#include "pch.h"
#include "App.xaml.h"
#include "Core/PinentryHandler.h"

extern "C"
{
#include "../pinentry/pinentry.h"
}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    winrt::init_apartment(winrt::apartment_type::single_threaded);

    pinentry_init("pinentry-winui3");
    pinentry_parse_opts(__argc, __argv);
    pinentry_cmd_handler = pinentry_winui3_cmd_handler;
    pinentry_set_flavor_flag("winui3");

    winrt::Microsoft::UI::Xaml::Application::Start([](auto&&)
    {
        winrt::make<winrt::PinentryWinUI3::implementation::App>();
    });

    return 0;
}

