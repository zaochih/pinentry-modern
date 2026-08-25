#include "pch.h"
#include "PinentryMapper.h"
#include "Localization.h"

extern "C"
{
#include "../../pinentry/pinentry.h"
}

namespace PinentryModern::Core
{
    namespace
    {
        std::wstring Trim(std::wstring value)
        {
            auto const isSpace = [](wchar_t ch) { return iswspace(ch) != 0; };
            value.erase(value.begin(), std::find_if_not(value.begin(), value.end(), isSpace));
            value.erase(std::find_if_not(value.rbegin(), value.rend(), isSpace).base(), value.end());
            return value;
        }

        std::wstring ValueOr(std::wstring const& value, std::wstring fallback)
        {
            auto trimmed = Trim(value);
            return trimmed.empty() ? std::move(fallback) : value;
        }

        std::wstring DecodeWithCodePage(char const* value, UINT codePage)
        {
            if (!value || !*value)
            {
                return {};
            }

            int const length = MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, value, -1, nullptr, 0);
            if (length <= 0)
            {
                return {};
            }

            std::wstring output(static_cast<size_t>(length - 1), L'\0');
            MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, value, -1, output.data(), length);
            return output;
        }

        std::wstring Between(std::wstring const& value, std::wstring_view start, std::wstring_view end)
        {
            auto const startIndex = value.find(start);
            if (startIndex == std::wstring::npos)
            {
                return {};
            }

            auto const contentStart = startIndex + start.size();
            auto const endIndex = value.find(end, contentStart);
            if (endIndex == std::wstring::npos)
            {
                return {};
            }

            return value.substr(contentStart, endIndex - contentStart);
        }

        std::wstring UserDataValue(std::wstring const& rawUserData, std::wstring_view key)
        {
            std::wstring prefix(key);
            prefix += L"=";

            auto const start = rawUserData.find(prefix);
            if (start == std::wstring::npos)
            {
                return {};
            }

            auto const valueStart = start + prefix.size();
            auto const delimiter = rawUserData.find(L',', valueStart);
            if (delimiter == std::wstring::npos)
            {
                return rawUserData.substr(valueStart);
            }

            return rawUserData.substr(valueStart, delimiter - valueStart);
        }

        std::wstring PercentDecode(std::wstring const& value)
        {
            std::string bytes;
            bytes.reserve(value.size());

            for (size_t index = 0; index < value.size(); ++index)
            {
                if (value[index] == L'%' && index + 2 < value.size())
                {
                    wchar_t hex[3] = { value[index + 1], value[index + 2], 0 };
                    wchar_t* end = nullptr;
                    long const decoded = wcstol(hex, &end, 16);
                    if (end && *end == 0 && decoded >= 0 && decoded <= 255)
                    {
                        bytes.push_back(static_cast<char>(decoded));
                        index += 2;
                        continue;
                    }
                }

                if (value[index] <= 0x7f)
                {
                    bytes.push_back(static_cast<char>(value[index]));
                }
                else
                {
                    return value;
                }
            }

            return DecodeWithCodePage(bytes.c_str(), CP_UTF8);
        }

        void ReplaceAll(std::wstring& value, std::wstring_view token, std::wstring const& replacement)
        {
            size_t position = 0;
            while ((position = value.find(token, position)) != std::wstring::npos)
            {
                value.replace(position, token.size(), replacement);
                position += replacement.size();
            }
        }

        void FillIdentityFromDescription(PinentryRequestPayload& request, std::wstring const& description)
        {
            std::vector<std::wstring> lines;
            size_t start = 0;
            while (start <= description.size())
            {
                auto const end = description.find(L'\n', start);
                lines.push_back(description.substr(start, end == std::wstring::npos ? std::wstring::npos : end - start));
                if (end == std::wstring::npos)
                {
                    break;
                }
                start = end + 1;
            }

            if (lines.size() <= 2)
            {
                return;
            }

            auto userID = Between(lines[1], L"\"", L"\"");
            if (!userID.empty())
            {
                request.identity.userID = userID;
                std::wstring work = userID;

                auto const emailMarker = work.rfind(L" <");
                if (emailMarker != std::wstring::npos && !work.empty() && work.back() == L'>')
                {
                    request.identity.email = work.substr(emailMarker + 2, work.size() - emailMarker - 3);
                    work = work.substr(0, emailMarker);
                }

                auto const commentMarker = work.rfind(L" (");
                if (commentMarker != std::wstring::npos && !work.empty() && work.back() == L')')
                {
                    request.identity.comment = work.substr(commentMarker + 2, work.size() - commentMarker - 3);
                    work = work.substr(0, commentMarker);
                }

                request.identity.name = work;
            }

            auto const idMarker = description.find(L"ID ");
            if (idMarker != std::wstring::npos)
            {
                auto const idStart = idMarker + 3;
                auto const idEnd = description.find(L",", idStart);
                auto const keyID = description.substr(idStart, idEnd == std::wstring::npos ? std::wstring::npos : idEnd - idStart);
                if (keyID.size() == 8 || keyID.size() == 16)
                {
                    request.identity.keyID = keyID;
                }
            }
        }

        std::wstring ResolveDescriptionFromUserData(
            std::wstring const& rawUserData,
            std::wstring const& fallbackDescription,
            PinentryIdentityContext const& identity)
        {
            auto description = UserDataValue(rawUserData, L"DESCRIPTION");
            if (description.empty())
            {
                return fallbackDescription;
            }

            ReplaceAll(description, L"%USERID", identity.userID);
            ReplaceAll(description, L"%EMAIL", identity.email);
            ReplaceAll(description, L"%COMMENT", identity.comment);
            ReplaceAll(description, L"%NAME", identity.name);
            ReplaceAll(description, L"%KEYID", identity.keyID);

            auto decoded = PercentDecode(description);
            return decoded.empty() ? fallbackDescription : decoded;
        }

    }

    std::wstring FromPinentryString(char const* value)
    {
        auto utf8 = DecodeWithCodePage(value, CP_UTF8);
        if (!utf8.empty())
        {
            return utf8;
        }

        return DecodeWithCodePage(value, CP_ACP);
    }

    std::string ToUtf8(std::wstring const& value)
    {
        if (value.empty())
        {
            return {};
        }

        int const length = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (length <= 0)
        {
            return {};
        }

        std::string output(static_cast<size_t>(length - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, output.data(), length, nullptr, nullptr);
        return output;
    }

    std::wstring ReadEnvironmentString(char const* name)
    {
        char* value = nullptr;
        size_t length = 0;
        if (_dupenv_s(&value, &length, name) != 0 || !value)
        {
            return {};
        }

        std::unique_ptr<char, decltype(&std::free)> owned(value, std::free);
        return FromPinentryString(value);
    }

    bool CanPersistCredential(std::wstring const& keyInfo)
    {
        if (keyInfo.size() <= 2 || keyInfo[1] != L'/')
        {
            return false;
        }

        return keyInfo[0] != L'u' && !keyInfo.substr(2).empty();
    }

    std::wstring CacheIdentifierFromKeyInfo(std::wstring const& keyInfo)
    {
        if (!CanPersistCredential(keyInfo))
        {
            return {};
        }

        return keyInfo.substr(2);
    }

    std::wstring CredentialTargetForCacheIdentifier(std::wstring const& cacheIdentifier)
    {
        if (cacheIdentifier.empty())
        {
            return {};
        }

        return L"pinentry-modern:" + cacheIdentifier;
    }

    PinentryRequestPayload MakeRequestPayload(void* rawPinentry)
    {
        auto* pe = static_cast<pinentry_t>(rawPinentry);
        PinentryRequestPayload request;
        if (!pe)
        {
            return request;
        }

        request.requiresPassphrase = pe->pin != nullptr;
        request.title = FromPinentryString(pe->title);
        request.message = FromPinentryString(pe->description);
        request.errorText = FromPinentryString(pe->error);
        request.promptText = FromPinentryString(pe->prompt);
        request.okText = FromPinentryString(pe->ok);
        request.cancelText = FromPinentryString(pe->cancel);
        request.notOkText = FromPinentryString(pe->notok);
        request.repeatPassphrase = pe->repeat_passphrase != nullptr;
        request.qualityBarRequested = pe->quality_bar != nullptr;
        request.timeoutSeconds = pe->timeout;
        request.keyInfo = FromPinentryString(pe->keyinfo);
        request.userData = ReadEnvironmentString("PINENTRY_USER_DATA");
        request.ownerPID = pe->owner_pid;
        request.pinentryPointer = pe;

        FillIdentityFromDescription(request, request.message);

        auto const cacheIdentifier = CacheIdentifierFromKeyInfo(request.keyInfo);
        request.automaticCredentialTarget = CredentialTargetForCacheIdentifier(cacheIdentifier);
        if (!request.automaticCredentialTarget.empty())
        {
            request.automaticWindowsHelloPrompt = Localization::WindowsHelloUnlockPrompt();
            if (!request.identity.userID.empty())
            {
                request.automaticWindowsHelloPrompt = Localization::WindowsHelloUnlockPrompt(request.identity.userID);
            }
        }

        return request;
    }

    DialogModel MapRequestToDialog(PinentryRequestPayload const& payload)
    {
        auto const resolvedMessage = payload.userData.empty()
            ? payload.message
            : ResolveDescriptionFromUserData(payload.userData, payload.message, payload.identity);

        auto const iconPath = UserDataValue(payload.userData, L"ICON");

        DialogModel dialog;
        dialog.mode = payload.requiresPassphrase ? DialogMode::Passphrase : DialogMode::Confirm;
        dialog.title = ValueOr(
            Localization::LocalizeKnownPinentryText(payload.title, Localization::TextRole::Title),
            Localization::DefaultDialogTitle());
        dialog.message = Localization::LocalizeKnownPinentryText(resolvedMessage, Localization::TextRole::Message);
        dialog.errorText = Localization::LocalizeKnownPinentryText(payload.errorText, Localization::TextRole::Message);
        dialog.promptText = Localization::LocalizeKnownPinentryText(payload.promptText, Localization::TextRole::Prompt);
        dialog.okText = ValueOr(
            Localization::LocalizeKnownPinentryText(payload.okText, Localization::TextRole::Ok),
            payload.requiresPassphrase ? Localization::UnlockAction() : Localization::AllowAction());
        dialog.cancelText = ValueOr(
            Localization::LocalizeKnownPinentryText(payload.cancelText, Localization::TextRole::Cancel),
            Localization::CancelAction());
        dialog.notOkText = payload.requiresPassphrase ? L"" : Localization::LocalizeKnownPinentryText(payload.notOkText, Localization::TextRole::NotOk);
        dialog.showsRepeatField = payload.requiresPassphrase && payload.repeatPassphrase;
        dialog.showsQualityBar = payload.requiresPassphrase && payload.qualityBarRequested;
        dialog.canUseCredentialManager = payload.requiresPassphrase && CanPersistCredential(payload.keyInfo);
        dialog.showTypingToggleAvailable = payload.requiresPassphrase;
        dialog.saveInCredentialManagerDefault = payload.prefersSaveInCredentialManager;
        dialog.showTypingDefault = payload.prefersShowTyping;
        dialog.attemptsAutomaticWindowsHello = payload.attemptsAutomaticWindowsHello;
        dialog.automaticWindowsHelloPrompt = payload.automaticWindowsHelloPrompt;
        dialog.automaticCredentialTarget = payload.automaticCredentialTarget;
        dialog.timeoutSeconds = payload.timeoutSeconds;
        dialog.iconSource.ownerPID = payload.ownerPID;
        dialog.pinentryPointer = payload.pinentryPointer;

        if (!iconPath.empty())
        {
            dialog.iconSource.kind = IconSourceKind::FilePath;
            dialog.iconSource.filePath = iconPath;
        }
        else if (payload.ownerPID > 1)
        {
            dialog.iconSource.kind = IconSourceKind::OwnerProcessID;
        }

        return dialog;
    }
}
