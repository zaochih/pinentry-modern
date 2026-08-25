#include "pch.h"
#include "PinentryMapper.h"
#include "PinentryQuality.h"

extern "C"
{
#include "../../pinentry/pinentry.h"
}

namespace PinentryModern::Core
{
    int EvaluateQuality(void* pinentryPointer, std::wstring const& passphrase)
    {
        if (!pinentryPointer || passphrase.empty())
        {
            return 0;
        }

        auto utf8 = ToUtf8(passphrase);
        if (utf8.empty())
        {
            return 0;
        }

        return pinentry_inq_quality(
            static_cast<pinentry_t>(pinentryPointer),
            utf8.c_str(),
            utf8.size());
    }

    double NormalizeQuality(int quality)
    {
        if (quality <= 0)
        {
            return 0.0;
        }

        if (quality >= 100)
        {
            return 100.0;
        }

        return static_cast<double>(quality);
    }
}
