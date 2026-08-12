#include "pch.h"
#include "WindowsHello.h"
#include "Trace.h"

namespace PinentryWinUI3::Platform
{
    WindowsHelloResult VerifyWindowsHello(std::wstring const& prompt)
    {
        using namespace winrt::Windows::Security::Credentials::UI;

        try
        {
            auto availability = UserConsentVerifier::CheckAvailabilityAsync().get();
            if (availability != UserConsentVerifierAvailability::Available)
            {
                Trace(L"hello", L"unavailable, availability=" + std::to_wstring(static_cast<int>(availability)));
                return WindowsHelloResult::Unavailable;
            }

            auto result = UserConsentVerifier::RequestVerificationAsync(winrt::hstring(prompt)).get();
            Trace(L"hello", L"verification result=" + std::to_wstring(static_cast<int>(result)));
            switch (result)
            {
            case UserConsentVerificationResult::Verified:
                return WindowsHelloResult::Verified;
            case UserConsentVerificationResult::Canceled:
                return WindowsHelloResult::Canceled;
            default:
                return WindowsHelloResult::Failed;
            }
        }
        catch (winrt::hresult_error const& error)
        {
            Trace(L"hello", L"exception, hresult=" + std::to_wstring(static_cast<int32_t>(error.code())));
            return WindowsHelloResult::Unavailable;
        }
        catch (...)
        {
            Trace(L"hello", L"exception");
            return WindowsHelloResult::Unavailable;
        }
    }
}
