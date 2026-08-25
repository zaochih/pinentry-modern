#pragma once

#include "../pch.h"

namespace PinentryModern::Platform
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
