#import "MacOSXSwiftAppDelegate.h"
#import "pinentry.h"

@implementation PinentryMacSwiftAppDelegate

// The app runs as an accessory without a menu bar, so key equivalents such as
// Cmd+V have no menu item to dispatch through and are silently dropped. Install
// a minimal main menu with a standard Edit menu so text fields get the usual
// Cut/Copy/Paste/Select All. Quit is deliberately omitted: terminating
// mid-request would break the calling gpg-agent.
- (void)installMainMenu {
    NSMenu *mainMenu = [[NSMenu alloc] init];

    NSMenuItem *appMenuItem = [[NSMenuItem alloc] init];
    appMenuItem.submenu = [[NSMenu alloc] init];
    [mainMenu addItem:appMenuItem];

    NSMenuItem *editMenuItem = [[NSMenuItem alloc] init];
    NSMenu *editMenu = [[NSMenu alloc] initWithTitle:@"Edit"];
    [editMenu addItemWithTitle:@"Cut" action:@selector(cut:) keyEquivalent:@"x"];
    [editMenu addItemWithTitle:@"Copy" action:@selector(copy:) keyEquivalent:@"c"];
    [editMenu addItemWithTitle:@"Paste" action:@selector(paste:) keyEquivalent:@"v"];
    [editMenu addItemWithTitle:@"Select All" action:@selector(selectAll:) keyEquivalent:@"a"];
    editMenuItem.submenu = editMenu;
    [mainMenu addItem:editMenuItem];

    NSApp.mainMenu = mainMenu;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    [self installMainMenu];
    [[NSUserDefaults standardUserDefaults] addSuiteNamed:@"org.gpgtools.common"];
    [[NSUserDefaults standardUserDefaults] registerDefaults:@{@"UseKeychain": @YES}];

    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0), ^{
        int result = pinentry_loop();
        dispatch_async(dispatch_get_main_queue(), ^{
            [NSApp terminate:nil];
            exit(result ? 1 : 0);
        });
    });
}

@end
