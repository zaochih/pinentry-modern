#include "pch.h"
#include "PinentryDialog.xaml.h"

#include "Core/Localization.h"
#include "Core/PinentryQuality.h"
#include "Platform/OwnerIcon.h"
#include "Platform/SecureString.h"

#include <dwmapi.h>
#include <microsoft.ui.xaml.window.h>

using namespace winrt;
using namespace winrt::Microsoft::UI::Windowing;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;

namespace winrt::PinentryWinUI3::implementation
{
    namespace
    {
        hstring H(std::wstring const& value)
        {
            return hstring(value);
        }

        Visibility VisibleIf(bool condition)
        {
            return condition ? Visibility::Visible : Visibility::Collapsed;
        }

        bool IsChecked(CheckBox const& checkBox)
        {
            auto value = checkBox.IsChecked();
            return value && value.Value();
        }

        std::wstring QualityLabel(double value)
        {
            if (value >= 80)
            {
                return ::PinentryWinUI3::Core::Localization::QualityStrong();
            }
            if (value >= 55)
            {
                return ::PinentryWinUI3::Core::Localization::QualityGood();
            }
            if (value >= 30)
            {
                return ::PinentryWinUI3::Core::Localization::QualityFair();
            }
            return ::PinentryWinUI3::Core::Localization::QualityWeak();
        }

        winrt::Windows::Foundation::Uri FileUri(std::wstring path)
        {
            std::replace(path.begin(), path.end(), L'\\', L'/');
            if (!path.empty() && path.front() != L'/')
            {
                path = L"/" + path;
            }
            return winrt::Windows::Foundation::Uri(L"file://" + path);
        }

        int DipsToPhysicalPixels(double dips, UINT dpi)
        {
            UINT const effectiveDpi = dpi == 0 ? USER_DEFAULT_SCREEN_DPI : dpi;
            return static_cast<int>(std::ceil(dips * static_cast<double>(effectiveDpi) / 96.0));
        }
    }

    PinentryDialog::PinentryDialog()
    {
        InitializeComponent();
        InitializeDialogWindow();
    }

    PinentryDialog::PinentryDialog(
        ::PinentryWinUI3::Core::DialogModel dialog,
        std::function<void(::PinentryWinUI3::Core::DialogResponse)> onResolve)
        : dialog_(std::move(dialog)), onResolve_(std::move(onResolve))
    {
        InitializeComponent();
        InitializeDialogWindow();
        Closed([weak = get_weak()](winrt::Windows::Foundation::IInspectable const&, WindowEventArgs const&)
        {
            if (auto self = weak.get())
            {
                ::PinentryWinUI3::Core::DialogResponse response;
                response.canceled = true;
                self->Resolve(std::move(response));
            }
        });
        Populate();
        StartTimeout();
    }

    void PinentryDialog::InitializeDialogWindow()
    {
        Title(H(::PinentryWinUI3::Core::Localization::DefaultDialogTitle()));
        ConfigureTitleBar();
        PrepareInitialWindowForLayout();

        RootGrid().Loaded([weak = get_weak()](winrt::Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
        {
            if (auto self = weak.get())
            {
                self->ConfigureTitleBar();
                self->HideFromTaskbar();
                self->FitWindowToContent();
                self->DisableResize();
                self->PreferRoundedCorners();
                self->TuneWindowFrame();
                self->FocusInitialControl();
                self->ShowWindowAfterInitialLayout();
            }
        });
        RootGrid().SizeChanged([weak = get_weak()](winrt::Windows::Foundation::IInspectable const&, SizeChangedEventArgs const&)
        {
            if (auto self = weak.get())
            {
                self->ScheduleFitWindowToContent();
            }
        });

        Activated([weak = get_weak()](winrt::Windows::Foundation::IInspectable const&, WindowActivatedEventArgs const&)
        {
            if (auto self = weak.get())
            {
                self->HideFromTaskbar();
                self->DisableResize();
                self->PreferRoundedCorners();
                self->TuneWindowFrame();
            }
        });
    }

    void PinentryDialog::ConfigureTitleBar()
    {
        ExtendsContentIntoTitleBar(true);
        SetTitleBar(TitleBarDragRegion());

        HWND hwnd = NativeWindowHandle();
        if (!hwnd)
        {
            return;
        }

        auto appWindow = AppWindow();
        if (auto presenter = appWindow.Presenter().try_as<OverlappedPresenter>())
        {
            presenter.SetBorderAndTitleBar(false, false);
            presenter.IsResizable(false);
            presenter.IsMaximizable(false);
            presenter.IsMinimizable(false);
        }
    }

    void PinentryDialog::PrepareInitialWindowForLayout()
    {
        HWND hwnd = NativeWindowHandle();
        if (!hwnd)
        {
            return;
        }

        LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        exStyle |= WS_EX_LAYERED;
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
        SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
        SetWindowPos(
            hwnd,
            nullptr,
            -32000,
            -32000,
            0,
            0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    void PinentryDialog::ShowWindowAfterInitialLayout()
    {
        HWND hwnd = NativeWindowHandle();
        if (!hwnd)
        {
            return;
        }

        initialLayoutComplete_ = true;
        SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
        ShowWindowInFront();
    }

    void PinentryDialog::DisableResize()
    {
        HWND hwnd = NativeWindowHandle();
        if (!hwnd)
        {
            return;
        }

        LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
        style &= ~WS_THICKFRAME;
        style &= ~WS_MAXIMIZEBOX;
        style &= ~WS_MINIMIZEBOX;
        SetWindowLongPtr(hwnd, GWL_STYLE, style);
        SetWindowPos(
            hwnd,
            nullptr,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }

    void PinentryDialog::HideFromTaskbar()
    {
        HWND hwnd = NativeWindowHandle();
        if (!hwnd)
        {
            return;
        }

        LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        exStyle &= ~WS_EX_APPWINDOW;
        exStyle |= WS_EX_TOOLWINDOW;
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
        SetWindowPos(
            hwnd,
            nullptr,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }

    void PinentryDialog::PreferRoundedCorners()
    {
        HWND hwnd = NativeWindowHandle();
        if (!hwnd)
        {
            return;
        }

        DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
        DwmSetWindowAttribute(
            hwnd,
            DWMWA_WINDOW_CORNER_PREFERENCE,
            &preference,
            sizeof(preference));
    }

    void PinentryDialog::TuneWindowFrame()
    {
        HWND hwnd = NativeWindowHandle();
        if (!hwnd)
        {
            return;
        }

        COLORREF const borderColor = RGB(0xD0, 0xD0, 0xD0);
        DwmSetWindowAttribute(
            hwnd,
            DWMWA_BORDER_COLOR,
            &borderColor,
            sizeof(borderColor));
    }

    void PinentryDialog::Populate()
    {
        Title(H(dialog_.title));
        WindowCaptionText().Text(H(dialog_.title.empty() ? ::PinentryWinUI3::Core::Localization::DefaultDialogTitle() : dialog_.title));
        TitleText().Text(H(dialog_.title));
        MessageText().Text(H(dialog_.message));
        MessageText().Visibility(VisibleIf(!dialog_.message.empty()));
        SourceNameText().Text(H(dialog_.sourceApplicationName));
        SourceNameText().Visibility(VisibleIf(!dialog_.sourceApplicationName.empty()));

        ErrorText().Text(H(dialog_.errorText));
        ErrorBorder().Visibility(VisibleIf(!dialog_.errorText.empty()));

        bool const isPassphrase = dialog_.mode == ::PinentryWinUI3::Core::DialogMode::Passphrase;
        PassphrasePanel().Visibility(VisibleIf(isPassphrase));
        QualityPanel().Visibility(VisibleIf(dialog_.showsQualityBar));
        ShowTypingCheckBox().Visibility(VisibleIf(isPassphrase && dialog_.showTypingToggleAvailable));
        SaveCredentialCheckBox().Visibility(VisibleIf(isPassphrase && dialog_.canUseCredentialManager));

        MismatchText().Text(H(::PinentryWinUI3::Core::Localization::MismatchHint()));
        QualityTitleText().Text(H(::PinentryWinUI3::Core::Localization::QualityTitle()));
        ShowTypingCheckBox().Content(box_value(H(::PinentryWinUI3::Core::Localization::ShowTyping())));
        SaveCredentialCheckBox().Content(box_value(H(::PinentryWinUI3::Core::Localization::SaveInCredentialManager())));

        auto const promptText = H(dialog_.promptText.empty() ? ::PinentryWinUI3::Core::Localization::DefaultPassphrasePrompt() : dialog_.promptText);
        RepeatPassphraseBox().PlaceholderText(H(::PinentryWinUI3::Core::Localization::ConfirmPassphrasePrompt()));
        RepeatPassphraseBox().Visibility(VisibleIf(dialog_.showsRepeatField));

        ShowTypingCheckBox().IsChecked(dialog_.showTypingDefault);
        SaveCredentialCheckBox().IsChecked(dialog_.saveInCredentialManagerDefault);

        NotOkButton().Content(box_value(H(dialog_.notOkText)));
        NotOkButton().Visibility(VisibleIf(!dialog_.notOkText.empty()));
        CancelButton().Content(box_value(H(dialog_.cancelText)));
        PrimaryButton().Content(box_value(H(dialog_.okText)));
        UpdateActionButtonLayout();

        PassphraseBox().PlaceholderText(promptText);
        VisiblePassphraseBox().PlaceholderText(promptText);
        UpdateIcon();
        UpdatePassphraseMirror();
        UpdateValidation();
        UpdateQuality();
    }

    void PinentryDialog::StartTimeout()
    {
        remainingSeconds_ = dialog_.timeoutSeconds;
        if (remainingSeconds_ <= 0)
        {
            TimeoutText().Visibility(Visibility::Collapsed);
            return;
        }

        TimeoutText().Visibility(Visibility::Visible);
        TimeoutText().Text(H(::PinentryWinUI3::Core::Localization::TimeoutHint(remainingSeconds_)));

        timeoutTimer_ = DispatcherQueue().CreateTimer();
        timeoutTimer_.Interval(std::chrono::seconds(1));
        timeoutTimer_.Tick([weak = get_weak()](
            winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer const& timer,
            Windows::Foundation::IInspectable const&)
        {
            if (auto self = weak.get())
            {
                if (--self->remainingSeconds_ <= 0)
                {
                    timer.Stop();
                    ::PinentryWinUI3::Core::DialogResponse response;
                    response.canceled = true;
                    self->Resolve(std::move(response));
                    return;
                }

                self->TimeoutText().Text(H(::PinentryWinUI3::Core::Localization::TimeoutHint(self->remainingSeconds_)));
            }
        });
        timeoutTimer_.Start();
    }

    void PinentryDialog::Resolve(::PinentryWinUI3::Core::DialogResponse response)
    {
        if (completed_)
        {
            return;
        }

        completed_ = true;
        if (timeoutTimer_)
        {
            timeoutTimer_.Stop();
        }

        auto callback = std::move(onResolve_);
        Close();
        if (callback)
        {
            callback(std::move(response));
        }
    }

    std::wstring PinentryDialog::CurrentPassphrase()
    {
        if (IsChecked(ShowTypingCheckBox()))
        {
            return std::wstring(VisiblePassphraseBox().Text());
        }

        return std::wstring(PassphraseBox().Password());
    }

    std::wstring PinentryDialog::CurrentRepeatPassphrase()
    {
        return std::wstring(RepeatPassphraseBox().Password());
    }

    void PinentryDialog::OnPrimaryClick(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        UpdateValidation();
        if (!PrimaryButton().IsEnabled())
        {
            return;
        }

        ::PinentryWinUI3::Core::DialogResponse response;
        response.confirmed = true;
        response.passphrase = CurrentPassphrase();
        response.repeatOkay = !dialog_.showsRepeatField || response.passphrase == CurrentRepeatPassphrase();
        response.saveInCredentialManager = IsChecked(SaveCredentialCheckBox());
        Resolve(std::move(response));
    }

    void PinentryDialog::OnCancelClick(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        ::PinentryWinUI3::Core::DialogResponse response;
        response.canceled = true;
        Resolve(std::move(response));
    }

    void PinentryDialog::OnNotOkClick(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        ::PinentryWinUI3::Core::DialogResponse response;
        response.declined = true;
        Resolve(std::move(response));
    }

    void PinentryDialog::OnPassphraseChanged(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        if (!updatingMirror_ && !IsChecked(ShowTypingCheckBox()))
        {
            updatingMirror_ = true;
            VisiblePassphraseBox().Text(PassphraseBox().Password());
            updatingMirror_ = false;
        }

        UpdateValidation();
        UpdateQuality();
    }

    void PinentryDialog::OnRepeatPassphraseChanged(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        UpdateValidation();
    }

    void PinentryDialog::OnVisiblePassphraseChanged(Windows::Foundation::IInspectable const&, TextChangedEventArgs const&)
    {
        if (!updatingMirror_ && IsChecked(ShowTypingCheckBox()))
        {
            updatingMirror_ = true;
            PassphraseBox().Password(VisiblePassphraseBox().Text());
            updatingMirror_ = false;
        }

        UpdateValidation();
        UpdateQuality();
    }

    void PinentryDialog::OnShowTypingChanged(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        UpdatePassphraseMirror();
        FocusInitialControl();
    }

    void PinentryDialog::UpdatePassphraseMirror()
    {
        if (updatingMirror_)
        {
            return;
        }

        updatingMirror_ = true;
        bool const showing = IsChecked(ShowTypingCheckBox());

        if (showing)
        {
            VisiblePassphraseBox().Text(PassphraseBox().Password());
        }
        else
        {
            PassphraseBox().Password(VisiblePassphraseBox().Text());
        }

        PassphraseBox().Visibility(VisibleIf(!showing));
        VisiblePassphraseBox().Visibility(VisibleIf(showing));
        updatingMirror_ = false;
    }

    void PinentryDialog::UpdateValidation()
    {
        if (dialog_.mode != ::PinentryWinUI3::Core::DialogMode::Passphrase || !dialog_.showsRepeatField)
        {
            MismatchText().Visibility(Visibility::Collapsed);
            PrimaryButton().IsEnabled(true);
            return;
        }

        auto const passphrase = CurrentPassphrase();
        auto const repeated = CurrentRepeatPassphrase();
        bool const mismatch = !repeated.empty() && passphrase != repeated;
        bool const visibilityChanged = MismatchText().Visibility() != VisibleIf(mismatch);
        MismatchText().Visibility(VisibleIf(mismatch));
        PrimaryButton().IsEnabled(!mismatch);
        if (visibilityChanged)
        {
            FitWindowToContent();
        }
    }

    void PinentryDialog::UpdateQuality()
    {
        if (!dialog_.showsQualityBar)
        {
            return;
        }

        int const rawQuality = ::PinentryWinUI3::Core::EvaluateQuality(dialog_.pinentryPointer, CurrentPassphrase());
        double const quality = ::PinentryWinUI3::Core::NormalizeQuality(rawQuality);
        QualityBar().Value(quality);
        QualityText().Text(H(QualityLabel(quality)));
    }

    void PinentryDialog::UpdateIcon()
    {
        std::wstring iconPath;
        ::PinentryWinUI3::Platform::OwnerApplicationInfo ownerInfo;
        OutputDebugStringW((L"[pinentry-winui3 owner] dialog owner pid: " + std::to_wstring(dialog_.iconSource.ownerPID) + L"\n").c_str());
        ownerInfo = ::PinentryWinUI3::Platform::ResolveOwnerApplicationInfo(dialog_.iconSource.ownerPID);
        if (!ownerInfo.displayName.empty())
        {
            dialog_.sourceApplicationName = ownerInfo.displayName;
            SourceNameText().Text(H(dialog_.sourceApplicationName));
            SourceNameText().Visibility(Visibility::Visible);
        }

        if (dialog_.iconSource.kind == ::PinentryWinUI3::Core::IconSourceKind::FilePath)
        {
            iconPath = dialog_.iconSource.filePath;
        }
        else if (dialog_.iconSource.kind == ::PinentryWinUI3::Core::IconSourceKind::OwnerProcessID || !ownerInfo.iconPath.empty())
        {
            iconPath = ownerInfo.iconPath;
        }

        if (iconPath.empty())
        {
            DefaultIcon().Visibility(Visibility::Visible);
            DialogIconImage().Visibility(Visibility::Collapsed);
            return;
        }

        auto image = winrt::Microsoft::UI::Xaml::Media::Imaging::BitmapImage();
        image.UriSource(FileUri(iconPath));
        DialogIconImage().Source(image);
        DialogIconImage().Visibility(Visibility::Visible);
        DefaultIcon().Visibility(Visibility::Collapsed);
    }

    void PinentryDialog::UpdateActionButtonLayout()
    {
        bool const hasNotOk = !dialog_.notOkText.empty();

        ActionButtonsGrid().RowSpacing(hasNotOk ? 10.0 : 0.0);

        Grid::SetRow(NotOkButton(), 0);
        Grid::SetColumn(NotOkButton(), 0);
        Grid::SetColumnSpan(NotOkButton(), 2);

        Grid::SetRow(CancelButton(), hasNotOk ? 1 : 0);
        Grid::SetColumn(CancelButton(), 0);
        Grid::SetColumnSpan(CancelButton(), 1);

        Grid::SetRow(PrimaryButton(), hasNotOk ? 1 : 0);
        Grid::SetColumn(PrimaryButton(), 1);
        Grid::SetColumnSpan(PrimaryButton(), 1);
    }

    void PinentryDialog::FitWindowToContent()
    {
        fitWindowPending_ = false;
        HWND hwnd = NativeWindowHandle();
        if (!hwnd)
        {
            return;
        }

        fittingWindow_ = true;
        RootGrid().ClearValue(FrameworkElement::HeightProperty());
        RootGrid().Measure({ static_cast<float>(RootGrid().Width()), 10000.0f });
        auto const desired = RootGrid().DesiredSize();
        RootGrid().Height(desired.Height);
        DisableResize();

        UINT const dpi = GetDpiForWindow(hwnd);
        int const clientWidth = DipsToPhysicalPixels(std::max(360.0, static_cast<double>(desired.Width)), dpi);
        int const clientHeight = DipsToPhysicalPixels(std::max(1.0, static_cast<double>(desired.Height)), dpi);

        auto appWindow = AppWindow();
        appWindow.ResizeClient({ clientWidth, clientHeight });

        winrt::Windows::Graphics::SizeInt32 const windowSize = appWindow.Size();

        MONITORINFO monitorInfo{ sizeof(monitorInfo) };
        HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        if (!GetMonitorInfo(monitor, &monitorInfo))
        {
            SetWindowPos(hwnd, nullptr, 0, 0, windowSize.Width, windowSize.Height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            fittingWindow_ = false;
            return;
        }

        RECT const work = monitorInfo.rcWork;
        int const workWidth = static_cast<int>(work.right - work.left);
        int const workHeight = static_cast<int>(work.bottom - work.top);
        int const x = static_cast<int>(work.left) + std::max<int>(0, (workWidth - windowSize.Width) / 2);
        int const y = static_cast<int>(work.top) + std::max<int>(0, (workHeight - windowSize.Height) / 2);
        appWindow.Move({ x, y });
        fittingWindow_ = false;
    }

    void PinentryDialog::ScheduleFitWindowToContent()
    {
        if (fittingWindow_ || fitWindowPending_)
        {
            return;
        }

        fitWindowPending_ = true;
        DispatcherQueue().TryEnqueue([weak = get_weak()]
        {
            if (auto self = weak.get())
            {
                self->FitWindowToContent();
            }
        });
    }

    void PinentryDialog::ShowWindowInFront()
    {
        HWND hwnd = NativeWindowHandle();
        if (!hwnd)
        {
            return;
        }

        SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        SetForegroundWindow(hwnd);
    }

    void PinentryDialog::FocusInitialControl()
    {
        if (dialog_.mode == ::PinentryWinUI3::Core::DialogMode::Passphrase)
        {
            if (IsChecked(ShowTypingCheckBox()))
            {
                VisiblePassphraseBox().Focus(FocusState::Programmatic);
            }
            else
            {
                PassphraseBox().Focus(FocusState::Programmatic);
            }
            return;
        }

        PrimaryButton().Focus(FocusState::Programmatic);
    }

    HWND PinentryDialog::NativeWindowHandle()
    {
        HWND hwnd = nullptr;
        if (auto nativeWindow = this->try_as<IWindowNative>())
        {
            nativeWindow->get_WindowHandle(&hwnd);
        }

        return hwnd;
    }

}
