#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#import <sys/sysctl.h>
#import "pinentry.h"
#import "KeychainSupport.h"
#import "KeychainLabelUpdate.h"
#import "NSStringExtensions.h"
#import "pinentry_mac_swift-Swift.h"

static int pinentry_mac_swift_cmd_handler(pinentry_t pe);
pinentry_cmd_handler_t pinentry_cmd_handler = pinentry_mac_swift_cmd_handler;

int PinentryMacSwiftEvaluateQuality(void *pinentryPointer, const char *passphrase, size_t length) {
    if (!pinentryPointer || !passphrase) {
        return 0;
    }

    return pinentry_inq_quality((pinentry_t)pinentryPointer, passphrase, length);
}

int PinentryMacSwiftCopyParentProcessID(int processID) {
    if (processID <= 1) {
        return 0;
    }

    // sysctl works for any process; proc_pidinfo fails for root-owned ones such as
    // `login`, which sits between a terminal app and its shell and would cut the
    // walk up to the calling application short.
    struct kinfo_proc processInfo;
    size_t size = sizeof(processInfo);
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, processID };
    if (sysctl(mib, 4, &processInfo, &size, NULL, 0) != 0 || size == 0) {
        return 0;
    }

    return processInfo.kp_eproc.e_ppid;
}

static NSString *StringFromPinentryCString(char *string) {
    if (!string) {
        return nil;
    }

    return [NSString gpgStringWithCString:string];
}

static NSString *StringFromEnvironment(const char *name) {
    const char *value = getenv(name);
    if (!value || !*value) {
        return nil;
    }

    return [NSString gpgStringWithCString:value];
}

static NSString *CacheIdFromKeyInfo(NSString *keyInfo) {
    if (keyInfo.length <= 2) {
        return nil;
    }

    NSString *cacheMode = [keyInfo substringToIndex:1];
    if ([cacheMode isEqualToString:@"u"]) {
        return nil;
    }

    return [keyInfo substringFromIndex:2];
}

static NSString *KeyIDFromDescription(NSArray<NSString *> *lines);

static void FillIdentityFromDescription(CPinentryRequest *request, NSString *description) {
    NSArray *lines = [description componentsSeparatedByString:@"\n"];
    if (lines.count <= 2) {
        return;
    }

    NSString *userID = [[lines objectAtIndex:1] stringBetweenString:@"\"" andString:@"\"" needEnd:YES];
    if (userID.length > 0) {
        request.userID = userID;

        NSString *workString = userID;
        NSUInteger textLength = workString.length;
        NSRange range;

        if ([workString hasSuffix:@">"] && (range = [workString rangeOfString:@" <" options:NSBackwardsSearch]).length > 0) {
            range.location += 2;
            range.length = textLength - range.location - 1;
            request.email = [workString substringWithRange:range];
            workString = [workString substringToIndex:range.location - 2];
            textLength -= range.length + 3;
        }

        range = [workString rangeOfString:@" (" options:NSBackwardsSearch];
        if (range.length > 0 && range.location > 0 && [workString hasSuffix:@")"]) {
            range.location += 2;
            range.length = textLength - range.location - 1;
            request.comment = [workString substringWithRange:range];
            workString = [workString substringToIndex:range.location - 2];
        }

        request.name = workString;
    }

    request.keyID = KeyIDFromDescription(lines) ?: @"";
}

// The key line ("255-bit EDDSA key, ID 6713E2BCD4E25630,") follows the question and the
// quoted user ID. The user ID is skipped because it may itself contain "ID ", and a
// "0x" prefix (gpg --keyid-format 0xlong) is accepted.
static NSString *KeyIDFromDescription(NSArray<NSString *> *lines) {
    NSCharacterSet *nonHex = [[NSCharacterSet characterSetWithCharactersInString:@"0123456789abcdefABCDEF"] invertedSet];

    for (NSUInteger index = 2; index < lines.count; index++) {
        NSString *candidate = [[lines[index] stringBetweenString:@"ID " andString:@"," needEnd:YES]
                               stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet];
        if ([candidate hasPrefix:@"0x"] || [candidate hasPrefix:@"0X"]) {
            candidate = [candidate substringFromIndex:2];
        }

        if ((candidate.length == 8 || candidate.length == 16) &&
            [candidate rangeOfCharacterFromSet:nonHex].location == NSNotFound) {
            return candidate;
        }
    }

    return nil;
}

// The label of a saved item is "name <email> (key id)". Without a key id it would be
// shared by every key of the same user ID, so no label is built at all then.
static NSString *KeychainLabelForRequest(CPinentryRequest *request) {
    if (request.userID.length == 0 || request.keyID.length == 0) {
        return nil;
    }

    return [NSString stringWithFormat:@"%@ <%@> (%@)", request.name, request.email, request.keyID];
}

static CPinentryRequest *MakeRequest(pinentry_t pe) {
    CPinentryRequest *request = [[CPinentryRequest alloc] init];
    NSString *description = StringFromPinentryCString(pe->description);

    request.requiresPassphrase = pe->pin != NULL;
    request.title = StringFromPinentryCString(pe->title);
    request.message = description;
    request.errorText = StringFromPinentryCString(pe->error);
    request.promptText = StringFromPinentryCString(pe->prompt);
    request.okText = StringFromPinentryCString(pe->ok);
    request.cancelText = StringFromPinentryCString(pe->cancel);
    request.notOkText = StringFromPinentryCString(pe->notok);
    request.repeatPassphrase = pe->repeat_passphrase != NULL;
    request.qualityBarRequested = pe->quality_bar != NULL;
    request.timeoutSeconds = pe->timeout;
    request.keyInfo = StringFromPinentryCString(pe->keyinfo);
    request.prefersSaveInKeychain = [[NSUserDefaults standardUserDefaults] boolForKey:@"UseKeychain"];
    request.oneButton = pe->one_button != 0;
    request.prefersShowTyping = NO;
    request.userData = StringFromEnvironment("PINENTRY_USER_DATA");
    request.ownerPID = pe->owner_pid > 0 ? (NSInteger)pe->owner_pid : 0;
    request.pinentryPointer = pe;

    FillIdentityFromDescription(request, description);
    return request;
}

static int WritePassphraseToPinentry(pinentry_t pe, NSString *passphrase) {
    const char *utf8 = passphrase.UTF8String;
    if (!utf8) {
        return -1;
    }

    int length = (int)strlen(utf8);
    pinentry_setbufferlen(pe, length + 1);
    if (!pe->pin) {
        return -1;
    }

    strcpy(pe->pin, utf8);
    return length;
}

static int pinentry_mac_swift_cmd_handler(pinentry_t pe) {
    @autoreleasepool {
        static NSString *lastCacheIdUsed = nil;
        static BOOL doNotUseKeychain = NO;

        CPinentryRequest *request = MakeRequest(pe);
        NSString *cacheId = CacheIdFromKeyInfo(request.keyInfo);
        NSString *keychainLabel = KeychainLabelForRequest(request);
        BOOL lastTryWasKeychain = lastCacheIdUsed && [cacheId isEqualToString:lastCacheIdUsed];
        lastCacheIdUsed = nil;

        if (
            doNotUseKeychain
        ) {
            cacheId = nil;
        }

        BOOL canUseSavedPassphrase = request.requiresPassphrase && !request.repeatPassphrase;

        BOOL hasCacheItem =
            canUseSavedPassphrase &&
            cacheId &&
            !pe->error &&
            !lastTryWasKeychain &&
            hasPassphraseInKeychain(cacheId);

        // Without a cache id (SETKEYINFO --clear: export, new subkey, change passphrase),
        // or without an item for it, a saved item can only be reached by its label, and
        // only when exactly one item has it. The lookup is made at most once per request
        // and not at all on a retry that has a cache id, where its result would not be used.
        BOOL hasLabelItem = NO;
        if (
            canUseSavedPassphrase &&
            !hasCacheItem &&
            !doNotUseKeychain &&
            keychainLabel.length > 0 &&
            (!cacheId || !pe->error)
        ) {
            hasLabelItem = PinentryCountSavedPassphrasesWithLabel(keychainLabel) == 1;
        }

        if (hasCacheItem) {
            request.attemptsAutomaticTouchID = YES;
            request.automaticTouchIDCacheID = cacheId;
            request.automaticTouchIDKeychainLabel = keychainLabel;
        } else if (hasLabelItem && !pe->error) {
            request.attemptsAutomaticTouchID = YES;
            request.automaticTouchIDKeychainLabel = keychainLabel;
        }

        // The item filled in by label can be corrected after gpg-agent rejected it: only
        // that retry dialog offers to save, and only then is the item updated. A typo in a
        // first attempt must not replace a passphrase that still works, and without this
        // a stale passphrase would be filled in again on every request.
        BOOL canUpdateByLabel = hasLabelItem && !cacheId && pe->error != NULL;
        request.canUpdateSavedPassphraseByLabel = canUpdateByLabel;

        CPinentryResponse *response = [PinentryMacSwiftRuntime runRequest:request];

        if (request.requiresPassphrase) {
            if (response.canceled || response.declined) {
                return -1;
            }

            int length = WritePassphraseToPinentry(pe, response.passphrase ?: @"");
            if (length < 0) {
                return -1;
            }

            if (pe->repeat_passphrase && response.repeatOkay) {
                pe->repeat_okay = 1;
            }

            if (response.keychainUnusable) {
                doNotUseKeychain = YES;
                cacheId = nil;
            }

            if (response.pinFromCache) {
                pe->pin_from_cache = 1;
                if (cacheId.length > 0) {
                    lastCacheIdUsed = [cacheId copy];
                }
            }

            if (cacheId && response.saveInKeychain && response.passphrase.length > 0) {
                storePassphraseInKeychain(cacheId, response.passphrase, keychainLabel);
            } else if (
                canUpdateByLabel &&
                !response.keychainUnusable &&
                response.saveInKeychain &&
                response.passphrase.length > 0
            ) {
                PinentryUpdateSavedPassphraseByLabel(keychainLabel, response.passphrase);
            }

            return length;
        }

        if (response.confirmed) {
            return 1;
        }

        if (response.declined) {
            return 0;
        }

        pe->canceled = 1;
        return 0;
    }
}
