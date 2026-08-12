#include "pch.h"
#include "Trace.h"

namespace PinentryWinUI3::Platform
{
    namespace
    {
        bool TraceEnabled()
        {
            wchar_t value[16] = {};
            DWORD const length = GetEnvironmentVariableW(
                L"PINENTRY_WINUI3_OWNER_TRACE",
                value,
                static_cast<DWORD>(sizeof(value) / sizeof(value[0])));
            return length == 0 || wcscmp(value, L"0") != 0;
        }

        std::wstring TracePath()
        {
            wchar_t configuredPath[32768] = {};
            DWORD const configuredLength = GetEnvironmentVariableW(
                L"PINENTRY_WINUI3_OWNER_TRACE_LOG",
                configuredPath,
                static_cast<DWORD>(sizeof(configuredPath) / sizeof(configuredPath[0])));
            if (configuredLength > 0 && configuredLength < sizeof(configuredPath) / sizeof(configuredPath[0]))
            {
                return configuredPath;
            }

            std::wstring tempPath(MAX_PATH + 1, L'\0');
            DWORD const length = GetTempPathW(static_cast<DWORD>(tempPath.size()), tempPath.data());
            if (length == 0 || length >= tempPath.size())
            {
                return {};
            }

            tempPath.resize(length);
            return tempPath + L"pinentry-winui3-owner-trace.log";
        }
    }

    void ClearTraceLog()
    {
        auto const path = TracePath();
        if (!path.empty())
        {
            DeleteFileW(path.c_str());
        }
    }

    void Trace(std::wstring const& category, std::wstring const& message)
    {
        if (!TraceEnabled())
        {
            return;
        }

        auto line = L"[pinentry-winui3 " + category + L"] " + message + L"\n";
        OutputDebugStringW(line.c_str());

        auto const path = TracePath();
        if (path.empty())
        {
            return;
        }

        std::wofstream log(path, std::ios::app);
        if (log)
        {
            log << line;
        }
    }
}
