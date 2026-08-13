#include "pch.h"
#include "App.xaml.h"

#include "Core/PinentryRuntime.h"
#include "Platform/Trace.h"

extern "C"
{
#include "../pinentry/pinentry.h"
}

using namespace winrt;
using namespace winrt::Microsoft::UI::Dispatching;
using namespace winrt::Microsoft::UI::Xaml;

namespace winrt::PinentryWinUI3::implementation
{
    App::App()
    {
        ::PinentryWinUI3::Platform::Trace(L"startup", L"app constructor entered");
        UnhandledException([](winrt::Windows::Foundation::IInspectable const&, UnhandledExceptionEventArgs const& args)
        {
            ::PinentryWinUI3::Platform::Trace(
                L"xaml",
                L"unhandled exception, message=" + std::wstring(args.Message()));
        });

        try
        {
            InitializeComponent();
            ::PinentryWinUI3::Platform::Trace(L"startup", L"app InitializeComponent completed");
        }
        catch (winrt::hresult_error const& error)
        {
            ::PinentryWinUI3::Platform::Trace(
                L"startup",
                L"app InitializeComponent winrt exception, hresult=" +
                    std::to_wstring(static_cast<int32_t>(error.code())) +
                    L", message=" + std::wstring(error.message()));
            throw;
        }
        catch (...)
        {
            ::PinentryWinUI3::Platform::Trace(L"startup", L"app InitializeComponent exception");
            throw;
        }
    }

    void App::OnLaunched(LaunchActivatedEventArgs const&)
    {
        ::PinentryWinUI3::Platform::Trace(L"startup", L"app launched");
        ::PinentryWinUI3::Core::SetUIDispatcher(DispatcherQueue::GetForCurrentThread());

        std::thread([]()
        {
            ::PinentryWinUI3::Platform::Trace(L"startup", L"pinentry loop starting");
            winrt::init_apartment(winrt::apartment_type::multi_threaded);
            int const result = pinentry_loop();
            ::PinentryWinUI3::Platform::Trace(L"startup", L"pinentry loop exited, result=" + std::to_wstring(result));
            ::PinentryWinUI3::Core::ExitApplicationFromPinentryThread(result);
        }).detach();
    }
}
