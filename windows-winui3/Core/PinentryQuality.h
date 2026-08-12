#pragma once

#include "../pch.h"

namespace PinentryWinUI3::Core
{
    int EvaluateQuality(void* pinentryPointer, std::wstring const& passphrase);
    double NormalizeQuality(int quality);
}

