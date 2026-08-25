#include "pch.h"
#include "Localization.h"

namespace PinentryModern::Core::Localization
{
    namespace
    {
        enum class Language
        {
            English,
            ChineseSimplified
        };

        struct Translation
        {
            std::wstring_view english;
            std::wstring_view chinese;
        };

        std::wstring DecodeEnvironmentString(char const* value)
        {
            if (!value || !*value)
            {
                return {};
            }

            int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value, -1, nullptr, 0);
            UINT codePage = CP_UTF8;
            if (length <= 0)
            {
                length = MultiByteToWideChar(CP_ACP, 0, value, -1, nullptr, 0);
                codePage = CP_ACP;
            }

            if (length <= 0)
            {
                return {};
            }

            std::wstring output(static_cast<size_t>(length - 1), L'\0');
            MultiByteToWideChar(codePage, codePage == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0, value, -1, output.data(), length);
            return output;
        }

        std::wstring ReadLanguageOverride()
        {
            char* value = nullptr;
            size_t length = 0;
            if (_dupenv_s(&value, &length, "PINENTRY_WINUI3_LANG") != 0 || !value)
            {
                return {};
            }

            std::unique_ptr<char, decltype(&std::free)> owned(value, std::free);
            return DecodeEnvironmentString(value);
        }

        std::wstring LowercaseAscii(std::wstring value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch)
            {
                if (ch >= L'A' && ch <= L'Z')
                {
                    return static_cast<wchar_t>(ch - L'A' + L'a');
                }
                return ch;
            });
            return value;
        }

        bool StartsWith(std::wstring_view value, std::wstring_view prefix)
        {
            return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
        }

        Language LanguageFromName(std::wstring languageName)
        {
            languageName = LowercaseAscii(languageName);
            std::replace(languageName.begin(), languageName.end(), L'_', L'-');

            if (StartsWith(languageName, L"zh"))
            {
                return Language::ChineseSimplified;
            }

            return Language::English;
        }

        Language CurrentLanguage()
        {
            static Language const language = []
            {
                auto overrideLanguage = ReadLanguageOverride();
                if (!overrideLanguage.empty())
                {
                    return LanguageFromName(overrideLanguage);
                }

                wchar_t localeName[LOCALE_NAME_MAX_LENGTH] = {};
                LANGID const languageId = GetUserDefaultUILanguage();
                if (LCIDToLocaleName(MAKELCID(languageId, SORT_DEFAULT), localeName, LOCALE_NAME_MAX_LENGTH, 0) > 0)
                {
                    return LanguageFromName(localeName);
                }

                return Language::English;
            }();

            return language;
        }

        std::wstring T(Translation translation)
        {
            return std::wstring(CurrentLanguage() == Language::ChineseSimplified ? translation.chinese : translation.english);
        }

        std::wstring Trim(std::wstring value)
        {
            auto const isSpace = [](wchar_t ch) { return iswspace(ch) != 0; };
            value.erase(value.begin(), std::find_if_not(value.begin(), value.end(), isSpace));
            value.erase(std::find_if_not(value.rbegin(), value.rend(), isSpace).base(), value.end());
            return value;
        }

        std::wstring CanonicalLookupKey(std::wstring text)
        {
            text = Trim(std::move(text));
            std::wstring output;
            output.reserve(text.size());
            bool previousWasSpace = false;

            for (wchar_t ch : text)
            {
                if (ch == L'&' || ch == L'_')
                {
                    continue;
                }

                if (iswspace(ch))
                {
                    if (!previousWasSpace && !output.empty())
                    {
                        output.push_back(L' ');
                        previousWasSpace = true;
                    }
                    continue;
                }

                output.push_back(static_cast<wchar_t>(towlower(ch)));
                previousWasSpace = false;
            }

            if (!output.empty() && output.back() == L' ')
            {
                output.pop_back();
            }

            return output;
        }

        std::wstring StrippedTrailingPunctuationLookupKey(std::wstring text)
        {
            text = Trim(std::move(text));
            while (!text.empty())
            {
                wchar_t const ch = text.back();
                if (ch == L'.' || ch == L':' || ch == L'!' || ch == L'?' || ch == L'\x2026' || ch == L'\xFF1A' || iswspace(ch))
                {
                    text.pop_back();
                    continue;
                }
                break;
            }

            return CanonicalLookupKey(std::move(text));
        }

        using Entry = std::pair<std::wstring_view, Translation>;

        std::optional<std::wstring> Lookup(std::wstring const& text, Entry const* entries, size_t count)
        {
            auto const normalized = CanonicalLookupKey(text);
            auto const stripped = StrippedTrailingPunctuationLookupKey(text);

            for (size_t index = 0; index < count; ++index)
            {
                auto const entryText = std::wstring(entries[index].first);
                auto const key = CanonicalLookupKey(entryText);
                auto const strippedKey = StrippedTrailingPunctuationLookupKey(entryText);
                if (normalized == key || normalized == strippedKey || stripped == key || stripped == strippedKey)
                {
                    return T(entries[index].second);
                }
            }

            return std::nullopt;
        }

        std::optional<std::wstring> LocalizedMatch(std::wstring const& text, TextRole role)
        {
            static Entry const titleTranslations[] = {
                { L"Unlock Secret Key", { L"Unlock Secret Key", L"解锁私钥" } },
                { L"Allow Key Operation", { L"Allow Key Operation", L"允许密钥操作" } },
                { L"Set New Passphrase", { L"Set New Passphrase", L"设置新口令" } },
                { L"Protect Secret Key", { L"Protect Secret Key", L"保护私钥" } },
                { L"Passphrase Incorrect", { L"Passphrase Incorrect", L"口令不正确" } },
                { L"Unlock Signing Key", { L"Unlock Signing Key", L"解锁签名密钥" } },
                { L"Passphrase Required", { L"Passphrase Required", L"需要口令" } },
                { L"Confirm Key Operation", { L"Confirm Key Operation", L"确认密钥操作" } }
            };

            static Entry const messageTranslations[] = {
                { L"Enter the passphrase for your signing key.", { L"Enter the passphrase for your signing key.", L"请输入签名密钥的口令。" } },
                { L"Enter the passphrase for your GPG key.", { L"Enter the passphrase for your GPG key.", L"请输入你的 GPG 密钥口令。" } },
                { L"Another app is requesting permission to use your private key.", { L"Another app is requesting permission to use your private key.", L"另一个应用正在请求使用你的私钥。" } },
                { L"Another app is requesting permission to use your private key for this operation.", { L"Another app is requesting permission to use your private key for this operation.", L"另一个应用正请求使用你的私钥来执行此操作。" } },
                { L"Create a new passphrase for the selected key.", { L"Create a new passphrase for the selected key.", L"请为所选密钥创建一个新口令。" } },
                { L"Create a new passphrase for the selected key. You will need to enter it twice.", { L"Create a new passphrase for the selected key. You will need to enter it twice.", L"请为所选密钥创建一个新口令。你需要输入两次。" } },
                { L"Choose a passphrase strong enough to secure your key material.", { L"Choose a passphrase strong enough to secure your key material.", L"请选择一个足够强的口令来保护你的密钥材料。" } },
                { L"The provided passphrase could not unlock your secret key.", { L"The provided passphrase could not unlock your secret key.", L"提供的口令无法解锁你的私钥。" } },
                { L"The provided passphrase could not unlock your secret key. Try again.", { L"The provided passphrase could not unlock your secret key. Try again.", L"提供的口令无法解锁你的私钥，请重试。" } },
                { L"pinentry needs the passphrase for your GPG key before it can continue.", { L"pinentry needs the passphrase for your GPG key before it can continue.", L"Pinentry 需要你的 GPG 密钥口令才能继续。" } },
                { L"Please enter the passphrase to unlock the OpenPGP secret key:", { L"Please enter the passphrase to unlock the OpenPGP secret key:", L"请输入口令以解锁 OpenPGP 私钥：" } }
            };

            static Entry const promptTranslations[] = {
                { L"Passphrase", { L"Passphrase", L"口令" } },
                { L"New Passphrase", { L"New Passphrase", L"新口令" } },
                { L"Confirm Passphrase", { L"Confirm Passphrase", L"再次输入口令" } },
                { L"Repeat Passphrase", { L"Repeat Passphrase", L"再次输入口令" } },
                { L"PIN", { L"PIN", L"PIN 码" } },
                { L"Confirm PIN", { L"Confirm PIN", L"再次输入 PIN 码" } },
                { L"Repeat PIN", { L"Repeat PIN", L"再次输入 PIN 码" } }
            };

            static Entry const okTranslations[] = {
                { L"Unlock", { L"Unlock", L"解锁" } },
                { L"Allow", { L"Allow", L"允许" } },
                { L"Continue", { L"Continue", L"继续" } },
                { L"Try Again", { L"Try Again", L"重试" } },
                { L"Save Passphrase", { L"Save Passphrase", L"保存口令" } },
                { L"Use Passphrase", { L"Use Passphrase", L"使用该口令" } },
                { L"OK", { L"OK", L"确定" } },
                { L"Confirm", { L"Confirm", L"确认" } },
                { L"Retry", { L"Retry", L"重试" } },
                { L"Save", { L"Save", L"保存" } },
                { L"Proceed", { L"Proceed", L"继续" } },
                { L"Yes", { L"Yes", L"是" } }
            };

            static Entry const cancelTranslations[] = {
                { L"Cancel", { L"Cancel", L"取消" } },
                { L"No", { L"No", L"否" } }
            };

            static Entry const notOkTranslations[] = {
                { L"Deny", { L"Deny", L"拒绝" } },
                { L"No", { L"No", L"否" } }
            };

            switch (role)
            {
            case TextRole::Title:
                return Lookup(text, titleTranslations, std::size(titleTranslations));
            case TextRole::Message:
                return Lookup(text, messageTranslations, std::size(messageTranslations));
            case TextRole::Prompt:
                return Lookup(text, promptTranslations, std::size(promptTranslations));
            case TextRole::Ok:
                return Lookup(text, okTranslations, std::size(okTranslations));
            case TextRole::Cancel:
                return Lookup(text, cancelTranslations, std::size(cancelTranslations));
            case TextRole::NotOk:
                return Lookup(text, notOkTranslations, std::size(notOkTranslations));
            }

            return std::nullopt;
        }
    }

    std::wstring DefaultDialogTitle()
    {
        return T({ L"Pinentry Windows", L"密码验证" });
    }

    std::wstring UnlockAction()
    {
        return T({ L"OK", L"确定" });
    }

    std::wstring AllowAction()
    {
        return T({ L"Allow", L"允许" });
    }

    std::wstring CancelAction()
    {
        return T({ L"Cancel", L"取消" });
    }

    std::wstring DefaultPassphrasePrompt()
    {
        return T({ L"Passphrase", L"口令" });
    }

    std::wstring ConfirmPassphrasePrompt()
    {
        return T({ L"Confirm passphrase", L"再次输入口令" });
    }

    std::wstring MismatchHint()
    {
        return T({ L"The two passphrases must match.", L"两次输入的口令必须一致。" });
    }

    std::wstring TimeoutHint(int secondsRemaining)
    {
        if (CurrentLanguage() == Language::ChineseSimplified)
        {
            return L"此请求将在 " + std::to_wstring(secondsRemaining) + L" 秒后超时。";
        }

        return L"This request times out in " + std::to_wstring(secondsRemaining) + L"s.";
    }

    std::wstring QualityTitle()
    {
        return T({ L"Passphrase strength", L"口令强度" });
    }

    std::wstring QualityWeak()
    {
        return T({ L"Weak", L"弱" });
    }

    std::wstring QualityFair()
    {
        return T({ L"Fair", L"一般" });
    }

    std::wstring QualityGood()
    {
        return T({ L"Good", L"良好" });
    }

    std::wstring QualityStrong()
    {
        return T({ L"Strong", L"强" });
    }

    std::wstring ShowTyping()
    {
        return T({ L"Show typing", L"显示输入内容" });
    }

    std::wstring SaveInCredentialManager()
    {
        return T({ L"Save in Windows Credential Manager", L"保存到 Windows 凭据管理器" });
    }

    std::wstring WindowsHelloUnlockPrompt()
    {
        return T({ L"Unlock your GPG passphrase.", L"解锁你的 GPG 口令。" });
    }

    std::wstring WindowsHelloUnlockPrompt(std::wstring const& userID)
    {
        if (userID.empty())
        {
            return WindowsHelloUnlockPrompt();
        }

        if (CurrentLanguage() == Language::ChineseSimplified)
        {
            return L"解锁 " + userID + L"。";
        }

        return L"Unlock " + userID + L".";
    }

    std::wstring LocalizeKnownPinentryText(std::wstring const& text, TextRole role)
    {
        auto const trimmed = Trim(text);
        if (trimmed.empty())
        {
            return text;
        }

        if (auto localized = LocalizedMatch(trimmed, role))
        {
            auto output = text;
            auto const start = output.find(trimmed);
            if (start != std::wstring::npos)
            {
                output.replace(start, trimmed.size(), *localized);
                return output;
            }
            return *localized;
        }

        if (trimmed.find(L'\n') != std::wstring::npos)
        {
            std::wstring localizedLines;
            size_t start = 0;
            bool changed = false;
            while (start <= trimmed.size())
            {
                auto const end = trimmed.find(L'\n', start);
                auto const line = trimmed.substr(start, end == std::wstring::npos ? std::wstring::npos : end - start);
                auto localized = LocalizedMatch(line, role);
                if (localized)
                {
                    localizedLines += *localized;
                    changed = true;
                }
                else
                {
                    localizedLines += line;
                }

                if (end == std::wstring::npos)
                {
                    break;
                }

                localizedLines += L'\n';
                start = end + 1;
            }

            if (changed)
            {
                auto output = text;
                auto const replaceStart = output.find(trimmed);
                if (replaceStart != std::wstring::npos)
                {
                    output.replace(replaceStart, trimmed.size(), localizedLines);
                    return output;
                }
                return localizedLines;
            }
        }

        return text;
    }
}
