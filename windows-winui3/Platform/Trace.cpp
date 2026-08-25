#include "pch.h"
#include "Trace.h"

namespace PinentryModern::Platform
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
            return tempPath + L"pinentry-modern-owner-trace.log";
        }

        std::wstring ModuleDirectoryTracePath()
        {
            std::wstring modulePath(32768, L'\0');
            DWORD const length = GetModuleFileNameW(
                nullptr,
                modulePath.data(),
                static_cast<DWORD>(modulePath.size()));
            if (length == 0 || length >= modulePath.size())
            {
                return {};
            }

            modulePath.resize(length);
            auto const separator = modulePath.find_last_of(L"\\/");
            if (separator == std::wstring::npos)
            {
                return {};
            }

            return modulePath.substr(0, separator + 1) + L"pinentry-modern-owner-trace.log";
        }

        bool AppendTraceLine(std::wstring const& path, std::wstring const& line)
        {
            if (path.empty())
            {
                return false;
            }

            std::string utf8;
            int const utf8Length = WideCharToMultiByte(
                CP_UTF8,
                0,
                line.c_str(),
                static_cast<int>(line.size()),
                nullptr,
                0,
                nullptr,
                nullptr);
            if (utf8Length <= 0)
            {
                return false;
            }

            utf8.resize(static_cast<size_t>(utf8Length));
            int const writtenLength = WideCharToMultiByte(
                CP_UTF8,
                0,
                line.c_str(),
                static_cast<int>(line.size()),
                utf8.data(),
                utf8Length,
                nullptr,
                nullptr);
            if (writtenLength <= 0)
            {
                return false;
            }

            HANDLE file = CreateFileW(
                path.c_str(),
                FILE_APPEND_DATA,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                nullptr,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                nullptr);
            if (file == INVALID_HANDLE_VALUE)
            {
                return false;
            }

            DWORD bytesWritten = 0;
            BOOL const ok = WriteFile(
                file,
                utf8.data(),
                static_cast<DWORD>(utf8.size()),
                &bytesWritten,
                nullptr);
            CloseHandle(file);
            return ok && bytesWritten == utf8.size();
        }
    }

    void ClearTraceLog()
    {
        auto const path = TracePath();
        if (!path.empty())
        {
            DeleteFileW(path.c_str());
        }

        auto const modulePath = ModuleDirectoryTracePath();
        if (!modulePath.empty() && modulePath != path)
        {
            DeleteFileW(modulePath.c_str());
        }
    }

    void Trace(std::wstring const& category, std::wstring const& message)
    {
        if (!TraceEnabled())
        {
            return;
        }

        auto line = L"[pinentry-modern " + category + L"] " + message + L"\n";
        OutputDebugStringW(line.c_str());

        auto const path = TracePath();
        if (path.empty())
        {
            return;
        }

        if (!AppendTraceLine(path, line))
        {
            AppendTraceLine(ModuleDirectoryTracePath(), line);
        }
    }
}
