#include "pch.h"
#include "App.xaml.h"

#include "Core/PinentryRuntime.h"

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
        InitializeComponent();
    }

    void App::OnLaunched(LaunchActivatedEventArgs const&)
    {
        ::PinentryWinUI3::Core::SetUIDispatcher(DispatcherQueue::GetForCurrentThread());

        std::thread([]()
        {
            int const result = pinentry_loop();
            ::PinentryWinUI3::Core::ExitApplicationFromPinentryThread(result);
        }).detach();
    }
}
