#pragma once

#include "../pch.h"
#include "SecureString.h"

namespace PinentryModern::Platform
{
    class CredentialStore
    {
    public:
        bool Has(std::wstring const& target) const;
        std::optional<SecureWString> Read(std::wstring const& target) const;
        bool Write(std::wstring const& target, std::wstring const& label, std::wstring const& secret) const;
        void Delete(std::wstring const& target) const;
    };
}

