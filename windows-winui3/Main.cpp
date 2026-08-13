#include "pch.h"
#include "App.xaml.h"
#include "Core/PinentryHandler.h"
#include "Platform/Trace.h"

extern "C"
{
#include "../pinentry/pinentry.h"
}

namespace
{
    void WriteStartupProbe()
    {
        wchar_t tempPath[MAX_PATH + 1] = {};
        DWORD const length = GetTempPathW(static_cast<DWORD>(std::size(tempPath)), tempPath);
        if (length == 0 || length >= std::size(tempPath))
        {
            return;
        }

        std::wstring path(tempPath);
        path += L"pinentry-winui3-startup-probe.txt";

        HANDLE file = CreateFileW(
            path.c_str(),
            GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (file == INVALID_HANDLE_VALUE)
        {
            return;
        }

        char const message[] = "wWinMain reached\n";
        DWORD bytesWritten = 0;
        WriteFile(file, message, static_cast<DWORD>(sizeof(message) - 1), &bytesWritten, nullptr);
        CloseHandle(file);
    }
}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    using ::PinentryWinUI3::Platform::ClearTraceLog;
    using ::PinentryWinUI3::Platform::Trace;

    WriteStartupProbe();
    ClearTraceLog();
    Trace(L"startup", L"process start");

    try
    {
        winrt::init_apartment(winrt::apartment_type::single_threaded);
        Trace(L"startup", L"winrt apartment initialized");

        pinentry_init("pinentry-winui3");
        pinentry_parse_opts(__argc, __argv);
        pinentry_cmd_handler = pinentry_winui3_cmd_handler;
        pinentry_set_flavor_flag("winui3");
        Trace(L"startup", L"pinentry core initialized");

        winrt::Microsoft::UI::Xaml::Application::Start([](auto&&)
        {
            Trace(L"startup", L"creating XAML application");
            try
            {
                winrt::make<winrt::PinentryWinUI3::implementation::App>();
                Trace(L"startup", L"XAML application created");
            }
            catch (winrt::hresult_error const& error)
            {
                Trace(
                    L"startup",
                    L"XAML application creation winrt exception, hresult=" +
                        std::to_wstring(static_cast<int32_t>(error.code())) +
                        L", message=" + std::wstring(error.message()));
                throw;
            }
            catch (...)
            {
                Trace(L"startup", L"XAML application creation exception");
                throw;
            }
        });

        Trace(L"startup", L"application exited");
        return 0;
    }
    catch (winrt::hresult_error const& error)
    {
        Trace(
            L"startup",
            L"winrt exception, hresult=" + std::to_wstring(static_cast<int32_t>(error.code())) +
                L", message=" + std::wstring(error.message()));
    }
    catch (...)
    {
        Trace(L"startup", L"unhandled startup exception");
    }

    return -1;
}
