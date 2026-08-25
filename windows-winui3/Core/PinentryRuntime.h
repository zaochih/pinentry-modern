#pragma once

#include "../pch.h"

namespace PinentryModern::Core
{
    void SetUIDispatcher(winrt::Microsoft::UI::Dispatching::DispatcherQueue const& dispatcher);
    winrt::Microsoft::UI::Dispatching::DispatcherQueue UIDispatcher();
    void ExitApplicationFromPinentryThread(int loopResult);
}
