#pragma once

#include "../pch.h"

namespace PinentryWinUI3::Platform
{
    enum class WindowsHelloResult
    {
        Verified,
        Unavailable,
        Canceled,
        Failed
    };

    WindowsHelloResult VerifyWindowsHello(std::wstring const& prompt);
}

