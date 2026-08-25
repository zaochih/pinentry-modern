#pragma once

#include "Core/PinentryTypes.h"
#include <winrt/PinentryModern.h>
#include "PinentryDialog.g.h"

namespace winrt::PinentryModern::implementation
{
    struct PinentryDialog :
        winrt::Microsoft::UI::Xaml::Markup::ComponentConnectorT<PinentryDialogT<PinentryDialog>>
    {
        PinentryDialog();
        PinentryDialog(
            ::PinentryModern::Core::DialogModel dialog,
            std::function<void(::PinentryModern::Core::DialogResponse)> onResolve);

        void OnPrimaryClick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void OnCancelClick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void OnNotOkClick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void OnPassphraseChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void OnRepeatPassphraseChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void OnVisiblePassphraseChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& args);
        void OnPassphraseKeyDown(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const& args);
        void OnCloseButtonPointerEntered(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args);
        void OnCloseButtonPointerExited(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args);
        void OnShowTypingChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);

    private:
        void InitializeDialogWindow();
        void ConfigureTitleBar();
        void PrepareInitialWindowForLayout();
        void ShowWindowAfterInitialLayout();
        void DisableResize();
        void HideFromTaskbar();
        void TuneWindowFrame();
        void ApplyWindows10DialogAdjustments();
        void RegisterWindows10ThemeListener();
        void UnregisterWindows10ThemeListener();
        void SetWindows10CloseButtonHover(bool hovering);
        bool UsesWindows10DialogStyle() const;
        void Populate();
        void Resolve(::PinentryModern::Core::DialogResponse response);
        void ConfirmPrimaryAction();
        void UpdatePassphraseMirror();
        void UpdateValidation();
        void UpdateQuality();
        void UpdateIcon();
        void UpdateActionButtonLayout();
        void StartTimeout();
        void FitWindowToContent();
        void ScheduleFitWindowToContent();
        void ShowWindowInFront();
        void FocusInitialControl();
        HWND NativeWindowHandle();
        std::wstring CurrentPassphrase();
        std::wstring CurrentRepeatPassphrase();

        ::PinentryModern::Core::DialogModel dialog_;
        std::function<void(::PinentryModern::Core::DialogResponse)> onResolve_;
        bool completed_ = false;
        bool updatingMirror_ = false;
        bool fittingWindow_ = false;
        bool fitWindowPending_ = false;
        bool initialLayoutComplete_ = false;
        bool windows10CloseButtonEventsRegistered_ = false;
        bool windows10ThemeListenerRegistered_ = false;
        bool windows10DarkTheme_ = false;
        int remainingSeconds_ = 0;
        winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer timeoutTimer_{ nullptr };
        winrt::Windows::UI::ViewManagement::UISettings windows10ThemeSettings_{ nullptr };
        winrt::event_token windows10ThemeChangedToken_{};
    };
}

namespace winrt::PinentryModern::factory_implementation
{
    struct PinentryDialog : PinentryDialogT<PinentryDialog, implementation::PinentryDialog>
    {
    };
}
