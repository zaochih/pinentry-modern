#pragma once

#include "../pch.h"

namespace PinentryModern::Platform
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

