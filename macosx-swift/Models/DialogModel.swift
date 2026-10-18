import Foundation

struct DialogModel: Equatable {
    enum Mode: Equatable {
        case passphrase
        case confirm
    }

    enum IconSource: Equatable {
        case appIcon
        case ownerProcessID(Int)
        case systemSymbol(String)
        case bundledImage(String)
        case filePath(String)
    }

    let mode: Mode
    let title: String
    let message: String
    let errorText: String?
    let promptText: String?
    let okText: String
    let cancelText: String
    /// False for MESSAGE requests, which only have an OK button.
    let showsCancelButton: Bool
    let notOkText: String?
    let showsRepeatField: Bool
    let showsQualityBar: Bool
    let canUseKeychain: Bool
    let showTypingToggleAvailable: Bool
    let saveInKeychainDefault: Bool
    let showTypingDefault: Bool
    let attemptsAutomaticTouchID: Bool
    let automaticTouchIDPrompt: String?
    let automaticTouchIDCacheID: String?
    let automaticTouchIDKeychainLabel: String?
    let timeoutSeconds: Int?
    let iconSource: IconSource
}

/// Layout settings that can be changed without rebuilding.
enum DialogLayout {
    static let defaultWidth: CGFloat = 300
    static let widthRange: ClosedRange<CGFloat> = 260...720

    /// Width of the dialog content in points. Override with
    /// `defaults write cn.stapxs.pinentry-modern WindowWidth -int 400`
    /// and reset with `defaults delete cn.stapxs.pinentry-modern WindowWidth`.
    static var width: CGFloat {
        let stored = UserDefaults.standard.integer(forKey: "WindowWidth")
        guard stored > 0 else {
            return defaultWidth
        }

        return min(max(CGFloat(stored), widthRange.lowerBound), widthRange.upperBound)
    }
}

struct DialogViewState: Equatable {
    let isExpanded: Bool
    let showsErrorSection: Bool
    let showsOptionsSection: Bool
    let showsRepeatField: Bool
    let showsQualityBar: Bool
}

struct DialogResponse: Equatable {
    let confirmed: Bool
    let canceled: Bool
    let declined: Bool
    let passphrase: String
    let saveInKeychain: Bool
    let pinFromCache: Bool
    let keychainUnusable: Bool
}
