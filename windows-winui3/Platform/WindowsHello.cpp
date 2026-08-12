#include "pch.h"
#include "WindowsHello.h"

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
                return WindowsHelloResult::Unavailable;
            }

            auto result = UserConsentVerifier::RequestVerificationAsync(winrt::hstring(prompt)).get();
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
        catch (...)
        {
            return WindowsHelloResult::Unavailable;
        }
    }
}
