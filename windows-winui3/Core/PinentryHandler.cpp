#include "pch.h"
#include "PinentryHandler.h"
#include "Localization.h"
#include "PinentryMapper.h"
#include "PinentryRuntime.h"
#include "../Platform/CredentialStore.h"
#include "../Platform/SecureString.h"
#include "../Platform/Trace.h"
#include "../Platform/WindowsHello.h"
#include "../PinentryDialog.xaml.h"

using namespace PinentryWinUI3::Core;
using namespace PinentryWinUI3::Platform;

pinentry_cmd_handler_t pinentry_cmd_handler = pinentry_winui3_cmd_handler;

namespace
{
    struct DialogWaitState
    {
        std::mutex mutex;
        std::condition_variable condition;
        bool completed = false;
        DialogResponse response;
    };

    std::wstring CredentialLabelForRequest(PinentryRequestPayload const& request)
    {
        if (request.identity.userID.empty())
        {
            return L"pinentry-winui3";
        }

        if (!request.identity.name.empty() && !request.identity.email.empty() && !request.identity.keyID.empty())
        {
            return request.identity.name + L" <" + request.identity.email + L"> (" + request.identity.keyID + L")";
        }

        return request.identity.userID;
    }

    std::optional<DialogResponse> TryAutomaticCredential(PinentryRequestPayload const& request)
    {
        if (!request.requiresPassphrase)
        {
            Trace(L"credential", L"skip automatic credential: request does not require passphrase");
            return std::nullopt;
        }

        if (request.repeatPassphrase)
        {
            Trace(L"credential", L"skip automatic credential: repeat passphrase is active");
            return std::nullopt;
        }

        if (request.automaticCredentialTarget.empty())
        {
            Trace(L"credential", L"skip automatic credential: credential target is empty");
            return std::nullopt;
        }

        CredentialStore store;
        if (!store.Has(request.automaticCredentialTarget))
        {
            Trace(L"credential", L"credential not found: " + request.automaticCredentialTarget);
            return std::nullopt;
        }

        Trace(L"credential", L"credential found, requesting Windows Hello: " + request.automaticCredentialTarget);
        auto hello = VerifyWindowsHello(
            request.automaticWindowsHelloPrompt.empty()
                ? Localization::WindowsHelloUnlockPrompt()
                : request.automaticWindowsHelloPrompt);
        if (hello != WindowsHelloResult::Verified)
        {
            Trace(L"credential", L"Windows Hello did not verify, result=" + std::to_wstring(static_cast<int>(hello)));
            return std::nullopt;
        }

        auto secret = store.Read(request.automaticCredentialTarget);
        if (!secret || secret->Empty())
        {
            Trace(L"credential", L"credential read returned empty secret: " + request.automaticCredentialTarget);
            return std::nullopt;
        }

        Trace(L"credential", L"credential unlocked successfully: " + request.automaticCredentialTarget);
        DialogResponse response;
        response.confirmed = true;
        response.passphrase = secret->Take();
        response.pinFromCache = true;
        return response;
    }

    DialogResponse RunDialogOnUIThread(DialogModel const& dialog)
    {
        auto dispatcher = UIDispatcher();
        if (!dispatcher)
        {
            DialogResponse response;
            response.canceled = true;
            return response;
        }

        auto waitState = std::make_shared<DialogWaitState>();
        if (!dispatcher.TryEnqueue([dialog, waitState]()
        {
            auto windowHolder = std::make_shared<winrt::PinentryWinUI3::PinentryDialog>(nullptr);
            *windowHolder = winrt::make<winrt::PinentryWinUI3::implementation::PinentryDialog>(
                dialog,
                [waitState, windowHolder](DialogResponse response)
                {
                    std::scoped_lock lock(waitState->mutex);
                    waitState->response = std::move(response);
                    waitState->completed = true;
                    waitState->condition.notify_one();
                });
            windowHolder->Activate();
        }))
        {
            DialogResponse response;
            response.canceled = true;
            return response;
        }

        std::unique_lock lock(waitState->mutex);
        waitState->condition.wait(lock, [&waitState]() { return waitState->completed; });
        return std::move(waitState->response);
    }

    int WritePassphraseToPinentry(pinentry_t pe, std::wstring const& passphrase)
    {
        auto utf8 = ToUtf8(passphrase);
        if (utf8.empty() && !passphrase.empty())
        {
            return -1;
        }

        auto const length = static_cast<int>(utf8.size());
        pinentry_setbufferlen(pe, length + 1);
        if (!pe->pin)
        {
            SecureClear(utf8);
            return -1;
        }

        strcpy_s(pe->pin, static_cast<size_t>(pe->pin_len), utf8.c_str());
        SecureClear(utf8);
        return length;
    }
}

int pinentry_winui3_cmd_handler(pinentry_t pe)
{
    if (!pe)
    {
        return -1;
    }

    auto request = MakeRequestPayload(pe);
    auto const credentialTarget = request.automaticCredentialTarget;
    auto const credentialLabel = CredentialLabelForRequest(request);

    DialogResponse response;
    if (!pe->error)
    {
        if (auto automatic = TryAutomaticCredential(request))
        {
            response = std::move(*automatic);
        }
    }

    if (!response.confirmed)
    {
        auto dialog = MapRequestToDialog(request);
        response = RunDialogOnUIThread(dialog);
    }

    if (request.requiresPassphrase)
    {
        if (response.canceled || response.declined)
        {
            return -1;
        }

        int const length = WritePassphraseToPinentry(pe, response.passphrase);
        SecureClear(response.passphrase);
        if (length < 0)
        {
            return -1;
        }

        if (pe->repeat_passphrase && response.repeatOkay)
        {
            pe->repeat_okay = 1;
        }

        if (response.pinFromCache)
        {
            pe->pin_from_cache = 1;
        }

        if (!credentialTarget.empty() && response.saveInCredentialManager && length > 0)
        {
            bool const saved = CredentialStore().Write(credentialTarget, credentialLabel, FromPinentryString(pe->pin));
            Trace(L"credential", std::wstring(saved ? L"saved credential: " : L"failed to save credential: ") + credentialTarget);
        }

        return length;
    }

    if (response.confirmed)
    {
        return 1;
    }

    if (response.declined)
    {
        return 0;
    }

    pe->canceled = 1;
    return 0;
}
