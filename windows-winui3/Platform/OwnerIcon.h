#pragma once

#include "../pch.h"

namespace PinentryWinUI3::Platform
{
    struct OwnerApplicationInfo
    {
        std::wstring imagePath;
        std::wstring iconPath;
        std::wstring displayName;
    };

    std::wstring ResolveProcessImagePath(unsigned long processID);
    OwnerApplicationInfo ResolveOwnerApplicationInfo(unsigned long processID);
}
