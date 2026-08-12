#pragma once

#include "Core/PinentryTypes.h"
#include <winrt/PinentryWinUI3.h>
#include "PinentryDialog.g.h"

namespace winrt::PinentryWinUI3::implementation
{
    struct PinentryDialog :
        winrt::Microsoft::UI::Xaml::Markup::ComponentConnectorT<PinentryDialogT<PinentryDialog>>
    {
        PinentryDialog();
        PinentryDialog(
            ::PinentryWinUI3::Core::DialogModel dialog,
            std::function<void(::PinentryWinUI3::Core::DialogResponse)> onResolve);

        void OnPrimaryClick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void OnCancelClick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void OnNotOkClick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void OnPassphraseChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void OnRepeatPassphraseChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void OnVisiblePassphraseChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::TextChangedEventArgs const& args);
        void OnShowTypingChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);

    private:
        void Populate();
        void Resolve(::PinentryWinUI3::Core::DialogResponse response);
        void UpdatePassphraseMirror();
        void UpdateValidation();
        void UpdateQuality();
        void UpdateIcon();
        void StartTimeout();
        std::wstring CurrentPassphrase();
        std::wstring CurrentRepeatPassphrase();

        ::PinentryWinUI3::Core::DialogModel dialog_;
        std::function<void(::PinentryWinUI3::Core::DialogResponse)> onResolve_;
        bool completed_ = false;
        bool updatingMirror_ = false;
        int remainingSeconds_ = 0;
        winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer timeoutTimer_{ nullptr };
    };
}

namespace winrt::PinentryWinUI3::factory_implementation
{
    struct PinentryDialog : PinentryDialogT<PinentryDialog, implementation::PinentryDialog>
    {
    };
}
