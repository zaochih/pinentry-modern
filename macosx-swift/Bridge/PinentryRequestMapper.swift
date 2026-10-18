import Foundation

struct PinentryRequestMapper {
    private let parser: UserDataParser
    private let cachePolicy: CacheIdentifierPolicy
    private let remarkProvider: any KeyRemarkProviding
    private let callerResolver: any CallerNameResolving

    init(
        parser: UserDataParser = UserDataParser(),
        cachePolicy: CacheIdentifierPolicy = CacheIdentifierPolicy(),
        remarkProvider: any KeyRemarkProviding = FileKeyRemarkStore(),
        callerResolver: any CallerNameResolving = IconResolver()
    ) {
        self.parser = parser
        self.cachePolicy = cachePolicy
        self.remarkProvider = remarkProvider
        self.callerResolver = callerResolver
    }

    func map(_ payload: PinentryRequestPayload) -> DialogModel {
        let parsedUserData = parser.parse(
            rawUserData: payload.userData,
            fallbackDescription: payload.message,
            identity: payload.identity
        )

        let localizedTitle = payload.title?.nonEmpty.map {
            L10n.localizeKnownPinentryText($0, role: .title)
        }
        let localizedMessage = parsedUserData.resolvedDescription?.nonEmpty.map {
            L10n.localizeKnownPinentryText($0, role: .message)
        }
        let localizedErrorText = payload.errorText?.nonEmpty.map {
            L10n.localizeKnownPinentryText($0, role: .message)
        }
        let localizedPrompt = payload.promptText?.nonEmpty.map {
            L10n.localizeKnownPinentryText($0, role: .prompt)
        }
        let localizedOkText = payload.okText?.nonEmpty.map {
            L10n.localizeKnownPinentryText($0, role: .ok)
        }
        let localizedCancelText = payload.cancelText?.nonEmpty.map {
            L10n.localizeKnownPinentryText($0, role: .cancel)
        }
        let localizedNotOkText = payload.notOkText?.nonEmpty.map {
            L10n.localizeKnownPinentryText($0, role: .notOk)
        }

        let iconSource: DialogModel.IconSource
        if let iconPath = parsedUserData.iconPath, !iconPath.isEmpty {
            iconSource = .filePath(iconPath)
        } else if let ownerPID = payload.ownerPID, ownerPID > 1 {
            iconSource = .ownerProcessID(ownerPID)
        } else {
            iconSource = .appIcon
        }

        let keyRemark = payload.identity.keyID.nonEmpty
            .flatMap { remarkProvider.remark(forKeyID: $0) }?
            .nonEmpty

        let dialog = DialogModel(
            mode: payload.requiresPassphrase ? .passphrase : .confirm,
            title: localizedTitle ?? composedTitle(for: payload) ?? L10n.defaultDialogTitle,
            message: Self.message(localizedMessage, remark: keyRemark),
            errorText: localizedErrorText,
            promptText: localizedPrompt,
            okText: localizedOkText ?? (payload.requiresPassphrase ? L10n.unlockAction : L10n.allowAction),
            cancelText: localizedCancelText ?? L10n.cancelAction,
            showsCancelButton: !payload.oneButton,
            notOkText: payload.requiresPassphrase ? nil : localizedNotOkText,
            showsRepeatField: payload.requiresPassphrase && payload.repeatPassphrase,
            // gpg-agent only asks for the bar for some clients, never when creating
            // a key or changing a passphrase, so it is also shown whenever a new
            // passphrase is being set (repeat field). See PinentryInquiryQualityEstimator.
            showsQualityBar: payload.requiresPassphrase && (payload.qualityBarRequested || payload.repeatPassphrase),
            canUseKeychain: payload.requiresPassphrase
                && (cachePolicy.canPersistPassphrase(for: payload.keyInfo) || payload.canUpdateSavedPassphraseByLabel),
            showTypingToggleAvailable: payload.requiresPassphrase,
            saveInKeychainDefault: payload.prefersSaveInKeychain,
            showTypingDefault: payload.prefersShowTyping,
            attemptsAutomaticTouchID: payload.attemptsAutomaticTouchID,
            automaticTouchIDPrompt: payload.attemptsAutomaticTouchID
                ? Self.touchIDPrompt(
                    label: payload.automaticTouchIDKeychainLabel,
                    caller: callerName(for: payload),
                    remark: keyRemark
                )
                : nil,
            automaticTouchIDCacheID: payload.automaticTouchIDCacheID,
            automaticTouchIDKeychainLabel: payload.automaticTouchIDKeychainLabel,
            timeoutSeconds: payload.timeoutSeconds,
            iconSource: iconSource
        )

        return dialog
    }

    /// gpg-agent never sends a title, so one is composed here. Unlocking a key reads
    /// "<caller> is trying to use a GPG key"; asking for a new passphrase and any
    /// other passphrase request get a plain title. The kind of key comes from the
    /// cache identifier (`n/` GPG, `s/` SSH), not from the message text, which
    /// gpg-agent may have translated. Requests that are not passphrase prompts keep
    /// the default title.
    private func composedTitle(for payload: PinentryRequestPayload) -> String? {
        guard payload.requiresPassphrase else {
            return nil
        }

        if payload.repeatPassphrase {
            return L10n.setNewPassphraseTitle
        }

        let descriptor = cachePolicy.descriptor(for: payload.keyInfo)
        let isSSH: Bool
        switch descriptor.cacheMode {
        case .normal:
            isSSH = false
        case .ssh:
            isSSH = true
        case .user, .unknown:
            return L10n.enterPassphraseTitle
        }

        guard let caller = callerName(for: payload) else {
            return L10n.unlockKeyTitle(isSSH: isSSH)
        }

        return L10n.callerUsesKeyTitle(caller: caller, isSSH: isSSH)
    }

    private func callerName(for payload: PinentryRequestPayload) -> String? {
        payload.ownerPID.flatMap { callerResolver.callerName(ownerPID: $0)?.nonEmpty }
    }

    /// The key remark goes at the very end of the request message.
    private static func message(_ message: String?, remark: String?) -> String {
        guard let remark else {
            return message ?? ""
        }

        let note = L10n.keyRemarkNote(remark)
        guard let message, !message.isEmpty else {
            return note
        }

        return "\(message)\n\n\(note)"
    }

    /// The system sheet reads "<app> is trying to <reason>.", where <app> is always this
    /// process, so who is asking goes at the end of the first sentence and the remark in
    /// a second paragraph. The system appends its own final period, hence the one in the
    /// remark sentence is dropped while the first paragraph gets one.
    private static func touchIDPrompt(label: String?, caller: String?, remark: String?) -> String {
        var reason = label?.nonEmpty.map { L10n.touchIDReason(forKey: $0) } ?? L10n.touchIDReasonGeneric

        if let caller {
            reason += " \(L10n.touchIDForCaller(caller))"
        }

        guard let remark else {
            return reason
        }

        let note = L10n.keyRemarkNote(remark).trimmingCharacters(in: CharacterSet(charactersIn: ".。"))
        return "\(reason)\(L10n.touchIDSentenceEnd)\n\n\(note)"
    }
}

private extension String {
    var nonEmpty: String? {
        trimmingCharacters(in: .whitespacesAndNewlines).isEmpty ? nil : self
    }
}
