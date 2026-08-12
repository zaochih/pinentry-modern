#include "pch.h"
#include "PinentryRuntime.h"

using namespace winrt;
using namespace winrt::Microsoft::UI::Dispatching;
using namespace winrt::Microsoft::UI::Xaml;

namespace PinentryWinUI3::Core
{
    namespace
    {
        std::mutex dispatcherMutex;
        DispatcherQueue dispatcherQueue{ nullptr };
    }

    void SetUIDispatcher(DispatcherQueue const& dispatcher)
    {
        std::scoped_lock lock(dispatcherMutex);
        dispatcherQueue = dispatcher;
    }

    DispatcherQueue UIDispatcher()
    {
        std::scoped_lock lock(dispatcherMutex);
        return dispatcherQueue;
    }

    void ExitApplicationFromPinentryThread(int loopResult)
    {
        auto dispatcher = UIDispatcher();
        if (!dispatcher)
        {
            ExitProcess(loopResult ? 1 : 0);
        }

        if (!dispatcher.TryEnqueue([loopResult]()
        {
            Application::Current().Exit();
            ExitProcess(loopResult ? 1 : 0);
        }))
        {
            ExitProcess(loopResult ? 1 : 0);
        }
    }
}
