#pragma once

#include "../pch.h"

namespace PinentryModern::Core::Localization
{
    enum class TextRole
    {
        Title,
        Message,
        Prompt,
        Ok,
        Cancel,
        NotOk
    };

    std::wstring DefaultDialogTitle();
    std::wstring UnlockAction();
    std::wstring AllowAction();
    std::wstring CancelAction();
    std::wstring DefaultPassphrasePrompt();
    std::wstring ConfirmPassphrasePrompt();
    std::wstring MismatchHint();
    std::wstring TimeoutHint(int secondsRemaining);
    std::wstring QualityTitle();
    std::wstring QualityWeak();
    std::wstring QualityFair();
    std::wstring QualityGood();
    std::wstring QualityStrong();
    std::wstring ShowTyping();
    std::wstring SaveInCredentialManager();
    std::wstring WindowsHelloUnlockPrompt();
    std::wstring WindowsHelloUnlockPrompt(std::wstring const& userID);

    std::wstring LocalizeKnownPinentryText(std::wstring const& text, TextRole role);
}
