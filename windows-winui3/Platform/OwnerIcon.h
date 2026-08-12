#pragma once

#include "../pch.h"

namespace PinentryWinUI3::Platform
{
    std::wstring ResolveProcessImagePath(unsigned long processID);
    std::wstring ResolveIconPathForOwner(unsigned long processID);
}

