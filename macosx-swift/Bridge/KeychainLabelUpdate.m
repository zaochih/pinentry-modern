#import <Security/Security.h>
#import "KeychainLabelUpdate.h"

#define GPG_SERVICE_NAME "GnuPG"

// The keychain the rest of the app reads from: the one named by the `KeychainPath`
// default, otherwise the default keychain. SecKeychainOpen/CopyDefault are deprecated,
// but they are what the upstream keychain support still uses, and the only way to name
// the same keychain here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
static SecKeychainRef CopyConfiguredKeychain(void) {
    SecKeychainRef keychainRef = NULL;

    NSString *keychainPath = [[NSUserDefaults standardUserDefaults] stringForKey:@"KeychainPath"];
    if (keychainPath.length > 0) {
        if (SecKeychainOpen(keychainPath.fileSystemRepresentation, &keychainRef) != errSecSuccess) {
            return NULL;
        }
    } else if (SecKeychainCopyDefault(&keychainRef) != errSecSuccess) {
        return NULL;
    }

    return keychainRef;
}
#pragma clang diagnostic pop

// Attributes (never the passphrase) of every item with this label, in the configured
// keychain only. `searchList` is returned for the follow-up update.
static NSArray *CopyItemsWithLabel(NSString *label, NSArray **searchList) {
    SecKeychainRef keychainRef = CopyConfiguredKeychain();
    if (!keychainRef) {
        return nil;
    }

    NSArray *list = @[(__bridge id)keychainRef];
    CFRelease(keychainRef);

    NSDictionary *findQuery = @{(__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
                                (__bridge id)kSecAttrService: @GPG_SERVICE_NAME,
                                (__bridge id)kSecAttrLabel: label,
                                (__bridge id)kSecMatchSearchList: list,
                                (__bridge id)kSecMatchLimit: (__bridge id)kSecMatchLimitAll,
                                (__bridge id)kSecReturnAttributes: @YES};

    CFTypeRef result = NULL;
    if (SecItemCopyMatching((__bridge CFDictionaryRef)findQuery, &result) != errSecSuccess || !result) {
        return nil;
    }

    if (searchList) {
        *searchList = list;
    }

    return CFBridgingRelease(result);
}

NSUInteger PinentryCountSavedPassphrasesWithLabel(NSString *label) {
    if (label.length == 0) {
        return 0;
    }

    return [CopyItemsWithLabel(label, NULL) count];
}

BOOL PinentryUpdateSavedPassphraseByLabel(NSString *label, NSString *passphrase) {
    if (label.length == 0 || passphrase.length == 0) {
        return NO;
    }

    NSData *encodedPassphrase = [passphrase dataUsingEncoding:NSUTF8StringEncoding];
    if (!encodedPassphrase) {
        return NO;
    }

    NSArray *searchList = nil;
    NSArray *items = CopyItemsWithLabel(label, &searchList);
    if (items.count != 1) {
        return NO;
    }

    NSString *account = items.firstObject[(__bridge id)kSecAttrAccount];
    if (![account isKindOfClass:[NSString class]] || account.length == 0) {
        return NO;
    }

    // Update exactly that item, addressed by its own account.
    NSDictionary *updateQuery = @{(__bridge id)kSecClass: (__bridge id)kSecClassGenericPassword,
                                  (__bridge id)kSecAttrService: @GPG_SERVICE_NAME,
                                  (__bridge id)kSecAttrAccount: account,
                                  (__bridge id)kSecMatchSearchList: searchList};
    NSDictionary *changes = @{(__bridge id)kSecValueData: encodedPassphrase};

    return SecItemUpdate((__bridge CFDictionaryRef)updateQuery, (__bridge CFDictionaryRef)changes) == errSecSuccess;
}
