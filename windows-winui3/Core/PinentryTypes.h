#pragma once

#include "../pch.h"

namespace PinentryWinUI3::Core
{
    struct PinentryIdentityContext
    {
        std::wstring userID;
        std::wstring name;
        std::wstring email;
        std::wstring comment;
        std::wstring keyID;
    };

    struct PinentryRequestPayload
    {
        bool requiresPassphrase = false;
        std::wstring title;
        std::wstring message;
        std::wstring errorText;
        std::wstring promptText;
        std::wstring okText;
        std::wstring cancelText;
        std::wstring notOkText;
        bool repeatPassphrase = false;
        bool qualityBarRequested = false;
        int timeoutSeconds = 0;
        std::wstring keyInfo;
        bool prefersSaveInCredentialManager = true;
        bool prefersShowTyping = false;
        bool attemptsAutomaticWindowsHello = false;
        std::wstring automaticWindowsHelloPrompt;
        std::wstring automaticCredentialTarget;
        std::wstring userData;
        unsigned long ownerPID = 0;
        void* pinentryPointer = nullptr;
        PinentryIdentityContext identity;
    };

    enum class DialogMode
    {
        Passphrase,
        Confirm
    };

    enum class IconSourceKind
    {
        AppIcon,
        OwnerProcessID,
        FilePath
    };

    struct IconSource
    {
        IconSourceKind kind = IconSourceKind::AppIcon;
        unsigned long ownerPID = 0;
        std::wstring filePath;
    };

    struct DialogModel
    {
        DialogMode mode = DialogMode::Passphrase;
        std::wstring title;
        std::wstring message;
        std::wstring errorText;
        std::wstring promptText;
        std::wstring okText;
        std::wstring cancelText;
        std::wstring notOkText;
        bool showsRepeatField = false;
        bool showsQualityBar = false;
        bool canUseCredentialManager = false;
        bool showTypingToggleAvailable = true;
        bool saveInCredentialManagerDefault = true;
        bool showTypingDefault = false;
        bool attemptsAutomaticWindowsHello = false;
        std::wstring automaticWindowsHelloPrompt;
        std::wstring automaticCredentialTarget;
        int timeoutSeconds = 0;
        IconSource iconSource;
        void* pinentryPointer = nullptr;
    };

    struct DialogResponse
    {
        bool confirmed = false;
        bool canceled = false;
        bool declined = false;
        std::wstring passphrase;
        bool repeatOkay = false;
        bool saveInCredentialManager = false;
        bool pinFromCache = false;
        bool credentialStoreUnusable = false;
    };
}

