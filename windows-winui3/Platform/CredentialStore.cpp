#include "pch.h"
#include "CredentialStore.h"

namespace PinentryModern::Platform
{
    namespace
    {
        struct CredentialDeleter
        {
            void operator()(CREDENTIALW* credential) const noexcept
            {
                if (credential)
                {
                    CredFree(credential);
                }
            }
        };
    }

    bool CredentialStore::Has(std::wstring const& target) const
    {
        return Read(target).has_value();
    }

    std::optional<SecureWString> CredentialStore::Read(std::wstring const& target) const
    {
        if (target.empty())
        {
            return std::nullopt;
        }

        CREDENTIALW* rawCredential = nullptr;
        if (!CredReadW(target.c_str(), CRED_TYPE_GENERIC, 0, &rawCredential))
        {
            return std::nullopt;
        }

        std::unique_ptr<CREDENTIALW, CredentialDeleter> credential(rawCredential);
        if (!credential->CredentialBlob || credential->CredentialBlobSize == 0)
        {
            return std::nullopt;
        }

        auto const charCount = credential->CredentialBlobSize / sizeof(wchar_t);
        std::wstring secret(
            reinterpret_cast<wchar_t const*>(credential->CredentialBlob),
            reinterpret_cast<wchar_t const*>(credential->CredentialBlob) + charCount);

        if (!secret.empty() && secret.back() == L'\0')
        {
            secret.pop_back();
        }

        return SecureWString(std::move(secret));
    }

    bool CredentialStore::Write(std::wstring const& target, std::wstring const& label, std::wstring const& secret) const
    {
        if (target.empty() || secret.empty())
        {
            return false;
        }

        std::wstring persistedSecret = secret;
        persistedSecret.push_back(L'\0');

        CREDENTIALW credential{};
        credential.Type = CRED_TYPE_GENERIC;
        credential.TargetName = const_cast<LPWSTR>(target.c_str());
        credential.UserName = const_cast<LPWSTR>(label.empty() ? L"pinentry-modern" : label.c_str());
        credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
        credential.CredentialBlobSize = static_cast<DWORD>(persistedSecret.size() * sizeof(wchar_t));
        credential.CredentialBlob = reinterpret_cast<LPBYTE>(persistedSecret.data());

        bool const ok = CredWriteW(&credential, 0) != FALSE;
        SecureClear(persistedSecret);
        return ok;
    }

    void CredentialStore::Delete(std::wstring const& target) const
    {
        if (!target.empty())
        {
            CredDeleteW(target.c_str(), CRED_TYPE_GENERIC, 0);
        }
    }
}
