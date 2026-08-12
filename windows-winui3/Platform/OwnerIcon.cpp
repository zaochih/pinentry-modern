#include "pch.h"
#include "OwnerIcon.h"

namespace PinentryWinUI3::Platform
{
    namespace
    {
        struct HandleCloser
        {
            void operator()(HANDLE handle) const noexcept
            {
                if (handle && handle != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(handle);
                }
            }
        };

        using UniqueHandle = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleCloser>;
    }

    std::wstring ResolveProcessImagePath(unsigned long processID)
    {
        if (processID <= 1)
        {
            return {};
        }

        UniqueHandle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(processID)));
        if (!process)
        {
            return {};
        }

        std::wstring path(MAX_PATH, L'\0');
        DWORD length = static_cast<DWORD>(path.size());
        if (!QueryFullProcessImageNameW(process.get(), 0, path.data(), &length))
        {
            return {};
        }

        path.resize(length);
        return path;
    }

    std::wstring ResolveIconPathForOwner(unsigned long processID)
    {
        // WinUI can display the executable file icon through Shell thumbnails only
        // with extra async plumbing. Returning the image path keeps the source
        // discoverable and lets the dialog fall back cleanly when loading fails.
        return ResolveProcessImagePath(processID);
    }
}
