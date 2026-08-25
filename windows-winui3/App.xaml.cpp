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
using namespace winrt::Microsoft::UI::Xaml::Markup;
using namespace winrt::Microsoft::UI::Xaml::Media;

namespace winrt::PinentryModern::implementation
{
    namespace
    {
        using RtlGetVersionFn = LONG(WINAPI*)(OSVERSIONINFOW*);

        DWORD CurrentBuildNumber()
        {
            auto const ntdll = GetModuleHandleW(L"ntdll.dll");
            if (!ntdll)
            {
                return 0;
            }

            auto const rtlGetVersion = reinterpret_cast<RtlGetVersionFn>(
                GetProcAddress(ntdll, "RtlGetVersion"));
            if (!rtlGetVersion)
            {
                return 0;
            }

            OSVERSIONINFOW version = {};
            version.dwOSVersionInfoSize = sizeof(version);
            if (rtlGetVersion(&version) != 0)
            {
                return 0;
            }

            return version.dwBuildNumber;
        }

        bool ShouldUseWindows10Styles()
        {
            DWORD const buildNumber = CurrentBuildNumber();
            bool const useWindows10Styles = buildNumber >= 10240 && buildNumber < 21996;
            ::PinentryModern::Platform::Trace(
                L"startup",
                std::wstring(L"windows style check, build=") +
                    std::to_wstring(buildNumber) +
                    L", win10Styles=" +
                    (useWindows10Styles ? L"true" : L"false"));
            return useWindows10Styles;
        }

        bool ShouldUseDarkAppTheme()
        {
            DWORD appsUseLightTheme = 1;
            DWORD valueSize = sizeof(appsUseLightTheme);
            DWORD valueType = 0;
            LSTATUS const status = RegGetValueW(
                HKEY_CURRENT_USER,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                L"AppsUseLightTheme",
                RRF_RT_REG_DWORD,
                &valueType,
                &appsUseLightTheme,
                &valueSize);
            return status == ERROR_SUCCESS && appsUseLightTheme == 0;
        }

        SolidColorBrush Brush(BYTE red, BYTE green, BYTE blue)
        {
            return SolidColorBrush{ winrt::Windows::UI::ColorHelper::FromArgb(0xFF, red, green, blue) };
        }
    }

    App::App()
    {
        ::PinentryModern::Platform::Trace(L"startup", L"app constructor entered");
        UnhandledException([](winrt::Windows::Foundation::IInspectable const&, UnhandledExceptionEventArgs const& args)
        {
            ::PinentryModern::Platform::Trace(
                L"xaml",
                L"unhandled exception, message=" + std::wstring(args.Message()));
        });

        try
        {
            InitializeComponent();
            ::PinentryModern::Platform::Trace(L"startup", L"app InitializeComponent completed");
        }
        catch (winrt::hresult_error const& error)
        {
            ::PinentryModern::Platform::Trace(
                L"startup",
                L"app InitializeComponent winrt exception, hresult=" +
                    std::to_wstring(static_cast<int32_t>(error.code())) +
                    L", message=" + std::wstring(error.message()));
            throw;
        }
        catch (...)
        {
            ::PinentryModern::Platform::Trace(L"startup", L"app InitializeComponent exception");
            throw;
        }
    }

    void App::ApplyPlatformStyles()
    {
        if (!ShouldUseWindows10Styles())
        {
            return;
        }

        ResourceDictionary win10Styles;
        win10Styles.Source(winrt::Windows::Foundation::Uri(L"ms-appx:///Styles/Win10ControlStyles.xaml"));
        Resources().MergedDictionaries().Append(win10Styles);

        bool const useDarkTheme = ShouldUseDarkAppTheme();
        ApplyWindows10ThemeResources(useDarkTheme);

        ::PinentryModern::Platform::Trace(
            L"startup",
            std::wstring(L"windows 10 styles applied, dark=") + (useDarkTheme ? L"true" : L"false"));
    }

    void App::ApplyWindows10ThemeResources(bool useDarkTheme)
    {
        auto resources = Resources();
        auto themeDictionaries = resources.ThemeDictionaries();

        auto apply = [useDarkTheme](ResourceDictionary const& dictionary)
        {
            if (useDarkTheme)
            {
                dictionary.Insert(box_value(L"PinentryDialogBackgroundBrush"), box_value(Brush(0x1F, 0x1F, 0x1F)));
                dictionary.Insert(box_value(L"PinentryDialogHeaderBackgroundBrush"), box_value(Brush(0x76, 0xB9, 0xED)));
                dictionary.Insert(box_value(L"PinentryDialogFooterBackgroundBrush"), box_value(Brush(0x1F, 0x1F, 0x1F)));
                dictionary.Insert(box_value(L"PinentryDialogHeaderTextBrush"), box_value(Brush(0x11, 0x11, 0x11)));
                dictionary.Insert(box_value(L"PinentryDialogPrimaryTextBrush"), box_value(Brush(0xFF, 0xFF, 0xFF)));
                dictionary.Insert(box_value(L"PinentryDialogSecondaryTextBrush"), box_value(Brush(0xCF, 0xCF, 0xCF)));
                dictionary.Insert(box_value(L"PinentryWin10ControlBorderBrush"), box_value(Brush(0x99, 0x99, 0x99)));
                dictionary.Insert(box_value(L"PinentryWin10ButtonBackgroundBrush"), box_value(Brush(0x4C, 0x4C, 0x4C)));
                dictionary.Insert(box_value(L"PinentryWin10ButtonBackgroundPointerOverBrush"), box_value(Brush(0x4C, 0x4C, 0x4C)));
                dictionary.Insert(box_value(L"PinentryWin10ButtonBackgroundPressedBrush"), box_value(Brush(0x40, 0x40, 0x40)));
                dictionary.Insert(box_value(L"PinentryWin10ButtonHoverBorderBrush"), box_value(Brush(0x94, 0x94, 0x94)));
                dictionary.Insert(box_value(L"PinentryWin10ButtonFocusBorderBrush"), box_value(Brush(0xFF, 0xFF, 0xFF)));
                dictionary.Insert(box_value(L"ButtonBackground"), box_value(Brush(0x4C, 0x4C, 0x4C)));
                dictionary.Insert(box_value(L"ButtonBackgroundPointerOver"), box_value(Brush(0x4C, 0x4C, 0x4C)));
                dictionary.Insert(box_value(L"ButtonBackgroundPressed"), box_value(Brush(0x40, 0x40, 0x40)));
                dictionary.Insert(box_value(L"ButtonBorderBrush"), box_value(Brush(0x4C, 0x4C, 0x4C)));
                dictionary.Insert(box_value(L"ButtonBorderBrushPointerOver"), box_value(Brush(0x94, 0x94, 0x94)));
                dictionary.Insert(box_value(L"ButtonBorderBrushPressed"), box_value(Brush(0x40, 0x40, 0x40)));
                dictionary.Insert(box_value(L"ButtonForeground"), box_value(Brush(0xFF, 0xFF, 0xFF)));
                dictionary.Insert(box_value(L"ButtonForegroundPointerOver"), box_value(Brush(0xFF, 0xFF, 0xFF)));
                dictionary.Insert(box_value(L"ButtonForegroundPressed"), box_value(Brush(0xFF, 0xFF, 0xFF)));
                dictionary.Insert(box_value(L"AccentButtonBackground"), box_value(Brush(0x4C, 0x4C, 0x4C)));
                dictionary.Insert(box_value(L"AccentButtonBackgroundPointerOver"), box_value(Brush(0x4C, 0x4C, 0x4C)));
                dictionary.Insert(box_value(L"AccentButtonBackgroundPressed"), box_value(Brush(0x40, 0x40, 0x40)));
                dictionary.Insert(box_value(L"AccentButtonBorderBrush"), box_value(Brush(0x4C, 0x4C, 0x4C)));
                dictionary.Insert(box_value(L"AccentButtonBorderBrushPointerOver"), box_value(Brush(0x94, 0x94, 0x94)));
                dictionary.Insert(box_value(L"AccentButtonBorderBrushPressed"), box_value(Brush(0x40, 0x40, 0x40)));
                dictionary.Insert(box_value(L"AccentButtonForeground"), box_value(Brush(0xFF, 0xFF, 0xFF)));
                dictionary.Insert(box_value(L"AccentButtonForegroundPointerOver"), box_value(Brush(0xFF, 0xFF, 0xFF)));
                dictionary.Insert(box_value(L"AccentButtonForegroundPressed"), box_value(Brush(0xFF, 0xFF, 0xFF)));
                dictionary.Insert(box_value(L"SystemControlFocusVisualPrimaryBrush"), box_value(Brush(0xFF, 0xFF, 0xFF)));
                dictionary.Insert(box_value(L"SystemControlFocusVisualSecondaryBrush"), box_value(Brush(0x1F, 0x1F, 0x1F)));
            }
            else
            {
                dictionary.Insert(box_value(L"PinentryDialogBackgroundBrush"), box_value(Brush(0xE6, 0xE6, 0xE6)));
                dictionary.Insert(box_value(L"PinentryDialogHeaderBackgroundBrush"), box_value(Brush(0x76, 0xB9, 0xED)));
                dictionary.Insert(box_value(L"PinentryDialogFooterBackgroundBrush"), box_value(Brush(0xE6, 0xE6, 0xE6)));
                dictionary.Insert(box_value(L"PinentryDialogHeaderTextBrush"), box_value(Brush(0x11, 0x11, 0x11)));
                dictionary.Insert(box_value(L"PinentryDialogPrimaryTextBrush"), box_value(Brush(0x11, 0x11, 0x11)));
                dictionary.Insert(box_value(L"PinentryDialogSecondaryTextBrush"), box_value(Brush(0x5E, 0x5E, 0x5E)));
                dictionary.Insert(box_value(L"PinentryWin10ControlBorderBrush"), box_value(Brush(0x99, 0x99, 0x99)));
                dictionary.Insert(box_value(L"PinentryWin10ButtonBackgroundBrush"), box_value(Brush(0xB8, 0xB8, 0xB8)));
                dictionary.Insert(box_value(L"PinentryWin10ButtonBackgroundPointerOverBrush"), box_value(Brush(0xB8, 0xB8, 0xB8)));
                dictionary.Insert(box_value(L"PinentryWin10ButtonBackgroundPressedBrush"), box_value(Brush(0xA8, 0xA8, 0xA8)));
                dictionary.Insert(box_value(L"PinentryWin10ButtonHoverBorderBrush"), box_value(Brush(0x6E, 0x6E, 0x6E)));
                dictionary.Insert(box_value(L"PinentryWin10ButtonFocusBorderBrush"), box_value(Brush(0x00, 0x00, 0x00)));
                dictionary.Insert(box_value(L"ButtonBackground"), box_value(Brush(0xB8, 0xB8, 0xB8)));
                dictionary.Insert(box_value(L"ButtonBackgroundPointerOver"), box_value(Brush(0xB8, 0xB8, 0xB8)));
                dictionary.Insert(box_value(L"ButtonBackgroundPressed"), box_value(Brush(0xA8, 0xA8, 0xA8)));
                dictionary.Insert(box_value(L"ButtonBorderBrush"), box_value(Brush(0xB8, 0xB8, 0xB8)));
                dictionary.Insert(box_value(L"ButtonBorderBrushPointerOver"), box_value(Brush(0x6E, 0x6E, 0x6E)));
                dictionary.Insert(box_value(L"ButtonBorderBrushPressed"), box_value(Brush(0xA8, 0xA8, 0xA8)));
                dictionary.Insert(box_value(L"ButtonForeground"), box_value(Brush(0x11, 0x11, 0x11)));
                dictionary.Insert(box_value(L"ButtonForegroundPointerOver"), box_value(Brush(0x11, 0x11, 0x11)));
                dictionary.Insert(box_value(L"ButtonForegroundPressed"), box_value(Brush(0x11, 0x11, 0x11)));
                dictionary.Insert(box_value(L"AccentButtonBackground"), box_value(Brush(0xB8, 0xB8, 0xB8)));
                dictionary.Insert(box_value(L"AccentButtonBackgroundPointerOver"), box_value(Brush(0xB8, 0xB8, 0xB8)));
                dictionary.Insert(box_value(L"AccentButtonBackgroundPressed"), box_value(Brush(0xA8, 0xA8, 0xA8)));
                dictionary.Insert(box_value(L"AccentButtonBorderBrush"), box_value(Brush(0xB8, 0xB8, 0xB8)));
                dictionary.Insert(box_value(L"AccentButtonBorderBrushPointerOver"), box_value(Brush(0x6E, 0x6E, 0x6E)));
                dictionary.Insert(box_value(L"AccentButtonBorderBrushPressed"), box_value(Brush(0xA8, 0xA8, 0xA8)));
                dictionary.Insert(box_value(L"AccentButtonForeground"), box_value(Brush(0x11, 0x11, 0x11)));
                dictionary.Insert(box_value(L"AccentButtonForegroundPointerOver"), box_value(Brush(0x11, 0x11, 0x11)));
                dictionary.Insert(box_value(L"AccentButtonForegroundPressed"), box_value(Brush(0x11, 0x11, 0x11)));
                dictionary.Insert(box_value(L"SystemControlFocusVisualPrimaryBrush"), box_value(Brush(0x00, 0x00, 0x00)));
                dictionary.Insert(box_value(L"SystemControlFocusVisualSecondaryBrush"), box_value(Brush(0xE6, 0xE6, 0xE6)));
            }
        };

        if (auto light = themeDictionaries.Lookup(box_value(L"Light")).try_as<ResourceDictionary>())
        {
            apply(light);
        }
        if (auto dark = themeDictionaries.Lookup(box_value(L"Dark")).try_as<ResourceDictionary>())
        {
            apply(dark);
        }

        apply(resources);
    }

    void App::OnLaunched(LaunchActivatedEventArgs const&)
    {
        ::PinentryModern::Platform::Trace(L"startup", L"app launched");
        ApplyPlatformStyles();
        ::PinentryModern::Core::SetUIDispatcher(DispatcherQueue::GetForCurrentThread());

        std::thread([]()
        {
            ::PinentryModern::Platform::Trace(L"startup", L"pinentry loop starting");
            winrt::init_apartment(winrt::apartment_type::multi_threaded);
            int const result = pinentry_loop();
            ::PinentryModern::Platform::Trace(L"startup", L"pinentry loop exited, result=" + std::to_wstring(result));
            ::PinentryModern::Core::ExitApplicationFromPinentryThread(result);
        }).detach();
    }
}
