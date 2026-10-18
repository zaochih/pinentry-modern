import AppKit
import Combine
import SwiftUI

/// Presents a request as a standard system `NSAlert`.
///
/// The system draws the window (glass frame, shadow, corner radius), the icon,
/// the title and message, the buttons and the Return/Escape handling. Only the
/// interactive parts live in `DialogAccessoryView`, placed in the alert's
/// accessory area.
@MainActor
final class AlertDialogRunner {
    private let viewModel: DialogViewModel
    private var cancellables = Set<AnyCancellable>()
    private var isRunning = false

    init(viewModel: DialogViewModel) {
        self.viewModel = viewModel
    }

    /// Shows the alert and blocks, in a modal session, until a response exists: given
    /// by the user, or by the view model on its own (timeout). The outcome is reported
    /// through the view model. The alert comes back if it was closed without a response
    /// (OK pressed while the two passphrases still differ); returning without one would
    /// leave gpg-agent waiting forever.
    func run() {
        while viewModel.response == nil {
            showAlert()
        }
    }

    private func showAlert() {
        let alert = NSAlert()
        alert.messageText = viewModel.titleText
        alert.informativeText = viewModel.dialog.message
        alert.icon = viewModel.alertIcon()

        // The first button is the default one (Return); the order mirrors the
        // OK / Cancel / Not-OK buttons of the pinentry protocol. Cancel is left out
        // for MESSAGE requests, so the actions are kept in button order.
        let primary = alert.addButton(withTitle: viewModel.primaryActionTitle)
        var actions: [() -> Void] = [{ [viewModel] in viewModel.confirm() }]

        if viewModel.dialog.showsCancelButton {
            let secondary = alert.addButton(withTitle: viewModel.secondaryActionTitle)
            secondary.keyEquivalent = "\u{1B}"
            actions.append { [viewModel] in viewModel.cancel() }
        }

        if let tertiaryTitle = viewModel.tertiaryActionTitle {
            alert.addButton(withTitle: tertiaryTitle)
            actions.append { [viewModel] in viewModel.decline() }
        }

        if viewModel.hasAccessoryContent {
            // An alert is 32pt wider than its accessory.
            let width = DialogLayout.width - 32
            let content = DialogAccessoryView(viewModel: viewModel) { [weak primary] in
                primary?.performClick(nil)
            }
            .frame(width: width)

            let hostingView = NSHostingView(rootView: content)
            hostingView.setFrameSize(NSSize(width: width, height: hostingView.fittingSize.height))
            alert.accessoryView = hostingView
        }

        // OK follows the fields synchronously, from the new values themselves; reading them
        // back later would leave a window in which Return still hits an enabled button.
        let needsRepeat = viewModel.state.showsRepeatField
        primary.isEnabled = viewModel.isPrimaryActionEnabled
        Publishers.CombineLatest(viewModel.$passphrase, viewModel.$repeatedPassphrase)
            .sink { [weak primary] passphrase, repeated in
                primary?.isEnabled = !needsRepeat || repeated == passphrase
            }
            .store(in: &cancellables)

        // The view model can finish without the user, e.g. on timeout.
        viewModel.$response
            .sink { [weak self] response in
                guard response != nil else {
                    return
                }

                // Not triggered by an event, and `stopModal` only works as the result of
                // one; the alert would stay up until the next mouse move or key press.
                DispatchQueue.main.async {
                    if self?.isRunning == true {
                        NSApp.abortModal()
                    }
                }
            }
            .store(in: &cancellables)

        Self.disableInteractiveGlass(of: alert.window)

        // The system turns the glass interactive again when the window becomes key,
        // which for this background app happens on the first click, so switch it off
        // again then, and just before every click is processed.
        NotificationCenter.default.publisher(for: NSWindow.didBecomeKeyNotification, object: alert.window)
            .merge(with: NotificationCenter.default.publisher(for: NSApplication.didBecomeActiveNotification))
            .sink { [weak alert] _ in
                if let alert {
                    Self.disableInteractiveGlass(of: alert.window)
                }
            }
            .store(in: &cancellables)

        let clickMonitor = NSEvent.addLocalMonitorForEvents(matching: [.leftMouseDown]) { [weak alert] event in
            if let alert {
                Self.disableInteractiveGlass(of: alert.window)
            }
            return event
        }
        defer {
            if let clickMonitor {
                NSEvent.removeMonitor(clickMonitor)
            }
        }

        for delay in [0.05, 0.25, 0.8] {
            DispatchQueue.main.asyncAfter(deadline: .now() + delay) { [weak alert] in
                if let alert {
                    // The glass view may only exist once the window has been shown.
                    Self.disableInteractiveGlass(of: alert.window)
                    Self.focusFirstTextField(of: alert)
                }
            }
        }

        NSApp.activate(ignoringOtherApps: true)
        isRunning = true
        let result = alert.runModal()
        isRunning = false
        cancellables.removeAll()

        guard viewModel.response == nil else {
            return
        }

        let index = result.rawValue - NSApplication.ModalResponse.alertFirstButtonReturn.rawValue
        if actions.indices.contains(index) {
            // `confirm()` refuses while the repeat field differs; `run()` then shows the alert again.
            actions[index]()
        } else {
            // Aborted or stopped by something other than a button: not an answer of "no".
            viewModel.cancel()
        }
    }

    /// On macOS 27 the system alert's glass reacts visibly to every click, which is
    /// wrong for a passphrase prompt (the system's own Touch ID sheet does not).
    /// The glass is made of plain `NSGlassEffectView`s with a public
    /// `effectIsInteractive` switch, but there are two of them and they live in the
    /// window's glass child window, which has no public accessor. Every glass view
    /// reachable from the window and from that child window is switched off; when the
    /// child window cannot be found nothing more happens.
    private static func disableInteractiveGlass(of window: NSWindow) {
        guard #available(macOS 27.0, *) else {
            return
        }

        var roots: [NSView] = []
        if let frameView = window.contentView?.superview {
            roots.append(frameView)
        }

        // Private accessor: called through the selector (KVC could raise an exception
        // that Swift cannot catch) and the result is type-checked.
        let childAccessor = NSSelectorFromString("_glassEffectChildWindow")
        if
            window.responds(to: childAccessor),
            let childWindow = window.perform(childAccessor)?.takeUnretainedValue() as? NSWindow,
            let content = childWindow.contentView
        {
            roots.append(content)
            if let childFrameView = content.superview {
                roots.append(childFrameView)
            }
        }

        for root in roots {
            disableInteractiveGlass(in: root)
        }
    }

    @available(macOS 27.0, *)
    private static func disableInteractiveGlass(in view: NSView) {
        if let glass = view as? NSGlassEffectView {
            glass.effectIsInteractive = false
        }

        for subview in view.subviews {
            disableInteractiveGlass(in: subview)
        }
    }

    /// SwiftUI focus is not reliable while the alert puts its accessory into the
    /// window, so make sure the passphrase field ends up with the keyboard.
    private static func focusFirstTextField(of alert: NSAlert) {
        guard
            !(alert.window.firstResponder is NSText),
            let accessory = alert.accessoryView,
            let field = firstEditableTextField(in: accessory)
        else {
            return
        }

        alert.window.makeFirstResponder(field)
    }

    private static func firstEditableTextField(in view: NSView) -> NSTextField? {
        for subview in view.subviews {
            if let field = subview as? NSTextField, field.isEditable {
                return field
            }

            if let field = firstEditableTextField(in: subview) {
                return field
            }
        }

        return nil
    }
}

enum AlertIcon {
    static let fallbackSymbolName = "lock.shield"

    /// An SF Symbol in the system accent color. It is kept well below the size of the
    /// alert's 64pt icon slot so it does not look heavier than a regular app icon.
    static func symbol(_ name: String) -> NSImage {
        let configuration = NSImage.SymbolConfiguration(pointSize: 40, weight: .light)
            .applying(NSImage.SymbolConfiguration(paletteColors: [.controlAccentColor]))

        return NSImage(systemSymbolName: name, accessibilityDescription: nil)?
            .withSymbolConfiguration(configuration)
            ?? NSApp.applicationIconImage
            ?? NSImage()
    }
}
