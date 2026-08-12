#pragma once

#include "../pch.h"

namespace PinentryWinUI3::Platform
{
    class SecureWString
    {
    public:
        SecureWString() = default;
        explicit SecureWString(std::wstring value) : value_(std::move(value)) {}

        SecureWString(SecureWString const&) = delete;
        SecureWString& operator=(SecureWString const&) = delete;

        SecureWString(SecureWString&& other) noexcept : value_(std::move(other.value_))
        {
            other.value_.clear();
        }

        SecureWString& operator=(SecureWString&& other) noexcept
        {
            if (this != &other)
            {
                Clear();
                value_ = std::move(other.value_);
                other.value_.clear();
            }
            return *this;
        }

        ~SecureWString()
        {
            Clear();
        }

        std::wstring const& Get() const noexcept
        {
            return value_;
        }

        std::wstring Take()
        {
            std::wstring output = std::move(value_);
            value_.clear();
            return output;
        }

        bool Empty() const noexcept
        {
            return value_.empty();
        }

        void Clear() noexcept
        {
            if (!value_.empty())
            {
                SecureZeroMemory(value_.data(), value_.size() * sizeof(wchar_t));
                value_.clear();
            }
        }

    private:
        std::wstring value_;
    };

    inline void SecureClear(std::wstring& value) noexcept
    {
        if (!value.empty())
        {
            SecureZeroMemory(value.data(), value.size() * sizeof(wchar_t));
            value.clear();
        }
    }

    inline void SecureClear(std::string& value) noexcept
    {
        if (!value.empty())
        {
            SecureZeroMemory(value.data(), value.size());
            value.clear();
        }
    }
}

