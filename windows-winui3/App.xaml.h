#pragma once

#include <winrt/PinentryWinUI3.h>
#include "App.g.h"

namespace winrt::PinentryWinUI3::implementation
{
    struct App : AppT<App>
    {
        App();
        void OnLaunched(winrt::Microsoft::UI::Xaml::LaunchActivatedEventArgs const& args);
    };
}

namespace winrt::PinentryWinUI3::factory_implementation
{
    struct App : AppT<App, implementation::App>
    {
    };
}
