#pragma once

#include "PinentryTypes.h"

namespace PinentryModern::Core
{
    std::wstring FromPinentryString(char const* value);
    std::string ToUtf8(std::wstring const& value);
    std::wstring ReadEnvironmentString(char const* name);

    bool CanPersistCredential(std::wstring const& keyInfo);
    std::wstring CacheIdentifierFromKeyInfo(std::wstring const& keyInfo);
    std::wstring CredentialTargetForCacheIdentifier(std::wstring const& cacheIdentifier);

    PinentryRequestPayload MakeRequestPayload(void* pinentry);
    DialogModel MapRequestToDialog(PinentryRequestPayload const& payload);
}

