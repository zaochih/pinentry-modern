#pragma once

#include "../pch.h"

namespace PinentryModern::Platform
{
    void ClearTraceLog();
    void Trace(std::wstring const& category, std::wstring const& message);
}
