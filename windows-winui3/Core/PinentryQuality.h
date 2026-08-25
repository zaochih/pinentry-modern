#pragma once

#include "../pch.h"

namespace PinentryModern::Core
{
    int EvaluateQuality(void* pinentryPointer, std::wstring const& passphrase);
    double NormalizeQuality(int quality);
}

