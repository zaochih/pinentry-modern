#pragma once

#include <winrt/PinentryModern.h>
#include "App.g.h"

namespace winrt::PinentryModern::implementation
{
    struct App : AppT<App>
    {
        App();
        void OnLaunched(winrt::Microsoft::UI::Xaml::LaunchActivatedEventArgs const& args);

    private:
        void ApplyPlatformStyles();
        void ApplyWindows10ThemeResources(bool useDarkTheme);
    };
}

namespace winrt::PinentryModern::factory_implementation
{
    struct App : AppT<App, implementation::App>
    {
    };
}
