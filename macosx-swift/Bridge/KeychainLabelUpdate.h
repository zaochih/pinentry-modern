#import <Foundation/Foundation.h>

/// Number of saved GnuPG keychain items that carry `label` (no passphrase is read).
/// A label is only trusted to identify a key when exactly one item has it.
NSUInteger PinentryCountSavedPassphrasesWithLabel(NSString *label);

/// Replaces the passphrase of the saved GnuPG keychain item that carries `label`.
///
/// Used when gpg-agent sent no cache id (SETKEYINFO --clear: export, new subkey,
/// change passphrase), so the saved item can only be found through its label and a
/// saved passphrase that was just rejected has to be corrected.
///
/// Labels are not guaranteed to be unique, so nothing is changed unless exactly one
/// item matches; that item is then updated through its own account. Returns NO when
/// nothing was updated.
BOOL PinentryUpdateSavedPassphraseByLabel(NSString *label, NSString *passphrase);
