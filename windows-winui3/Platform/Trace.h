#pragma once

#include "../pch.h"

namespace PinentryWinUI3::Platform
{
    void ClearTraceLog();
    void Trace(std::wstring const& category, std::wstring const& message);
}
