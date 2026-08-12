#include "pch.h"
#include "PinentryDialog.xaml.h"

#include "Core/PinentryQuality.h"
#include "Platform/OwnerIcon.h"
#include "Platform/SecureString.h"

using namespace winrt;
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
                return L"Strong";
            }
            if (value >= 55)
            {
                return L"Good";
            }
            if (value >= 30)
            {
                return L"Fair";
            }
            return L"Weak";
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
    }

    PinentryDialog::PinentryDialog()
    {
        InitializeComponent();
    }

    PinentryDialog::PinentryDialog(
        ::PinentryWinUI3::Core::DialogModel dialog,
        std::function<void(::PinentryWinUI3::Core::DialogResponse)> onResolve)
        : dialog_(std::move(dialog)), onResolve_(std::move(onResolve))
    {
        InitializeComponent();
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

    void PinentryDialog::Populate()
    {
        Title(H(dialog_.title));
        TitleText().Text(H(dialog_.title));
        MessageText().Text(H(dialog_.message));
        MessageText().Visibility(VisibleIf(!dialog_.message.empty()));

        ErrorText().Text(H(dialog_.errorText));
        ErrorBorder().Visibility(VisibleIf(!dialog_.errorText.empty()));

        bool const isPassphrase = dialog_.mode == ::PinentryWinUI3::Core::DialogMode::Passphrase;
        PassphrasePanel().Visibility(VisibleIf(isPassphrase));
        QualityPanel().Visibility(VisibleIf(dialog_.showsQualityBar));
        ShowTypingCheckBox().Visibility(VisibleIf(isPassphrase && dialog_.showTypingToggleAvailable));
        SaveCredentialCheckBox().Visibility(VisibleIf(isPassphrase && dialog_.canUseCredentialManager));

        PromptText().Text(H(dialog_.promptText.empty() ? L"Passphrase" : dialog_.promptText));
        RepeatPassphraseBox().PlaceholderText(L"Confirm passphrase");
        RepeatPassphraseBox().Visibility(VisibleIf(dialog_.showsRepeatField));

        ShowTypingCheckBox().IsChecked(dialog_.showTypingDefault);
        SaveCredentialCheckBox().IsChecked(dialog_.saveInCredentialManagerDefault);

        NotOkButton().Content(box_value(H(dialog_.notOkText)));
        NotOkButton().Visibility(VisibleIf(!dialog_.notOkText.empty()));
        CancelButton().Content(box_value(H(dialog_.cancelText)));
        PrimaryButton().Content(box_value(H(dialog_.okText)));

        PassphraseBox().PlaceholderText(PromptText().Text());
        VisiblePassphraseBox().PlaceholderText(PromptText().Text());
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
        TimeoutText().Text(H(L"This request times out in " + std::to_wstring(remainingSeconds_) + L"s."));

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

                self->TimeoutText().Text(H(L"This request times out in " + std::to_wstring(self->remainingSeconds_) + L"s."));
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
        UpdatePassphraseMirror();
        UpdateValidation();
        UpdateQuality();
    }

    void PinentryDialog::OnRepeatPassphraseChanged(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        UpdateValidation();
    }

    void PinentryDialog::OnVisiblePassphraseChanged(Windows::Foundation::IInspectable const&, TextChangedEventArgs const&)
    {
        UpdatePassphraseMirror();
        UpdateValidation();
        UpdateQuality();
    }

    void PinentryDialog::OnShowTypingChanged(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        UpdatePassphraseMirror();
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
        MismatchText().Visibility(VisibleIf(mismatch));
        PrimaryButton().IsEnabled(!mismatch);
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
        if (dialog_.iconSource.kind == ::PinentryWinUI3::Core::IconSourceKind::FilePath)
        {
            iconPath = dialog_.iconSource.filePath;
        }
        else if (dialog_.iconSource.kind == ::PinentryWinUI3::Core::IconSourceKind::OwnerProcessID)
        {
            iconPath = ::PinentryWinUI3::Platform::ResolveIconPathForOwner(dialog_.iconSource.ownerPID);
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
}
