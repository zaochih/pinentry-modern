import AppKit
import Darwin
import Foundation

@objc(CPinentryRequest)
final class CPinentryRequest: NSObject {
    @objc var requiresPassphrase = false
    @objc var title: String?
    @objc var message: String?
    @objc var errorText: String?
    @objc var promptText: String?
    @objc var okText: String?
    @objc var cancelText: String?
    @objc var notOkText: String?
    @objc var repeatPassphrase = false
    @objc var qualityBarRequested = false
    @objc var timeoutSeconds = 0
    @objc var keyInfo: String?
    @objc var prefersSaveInKeychain = false
    /// There is no cache id, but an item for this key exists and can be updated by its label.
    @objc var canUpdateSavedPassphraseByLabel = false
    /// MESSAGE or CONFIRM --one-button: there is nothing to cancel.
    @objc var oneButton = false
    @objc var prefersShowTyping = false
    @objc var attemptsAutomaticTouchID = false
    @objc var automaticTouchIDCacheID: String?
    @objc var automaticTouchIDKeychainLabel: String?
    @objc var userData: String?
    @objc var ownerPID = 0
    @objc var userID = ""
    @objc var name = ""
    @objc var email = ""
    @objc var comment = ""
    @objc var keyID = ""
    @objc var pinentryPointer: UnsafeMutableRawPointer?

    var payload: PinentryRequestPayload {
        PinentryRequestPayload(
            requiresPassphrase: requiresPassphrase,
            title: title,
            message: message?.replacingOccurrences(of: "\\n", with: "\n"),
            errorText: errorText,
            promptText: promptText,
            okText: okText,
            cancelText: cancelText,
            notOkText: notOkText,
            repeatPassphrase: repeatPassphrase,
            qualityBarRequested: qualityBarRequested,
            timeoutSeconds: timeoutSeconds > 0 ? timeoutSeconds : nil,
            keyInfo: keyInfo,
            prefersSaveInKeychain: prefersSaveInKeychain,
            canUpdateSavedPassphraseByLabel: canUpdateSavedPassphraseByLabel,
            oneButton: oneButton,
            prefersShowTyping: prefersShowTyping,
            attemptsAutomaticTouchID: attemptsAutomaticTouchID,
            automaticTouchIDCacheID: automaticTouchIDCacheID,
            automaticTouchIDKeychainLabel: automaticTouchIDKeychainLabel,
            userData: userData,
            ownerPID: ownerPID > 0 ? ownerPID : nil,
            identity: PinentryIdentityContext(
                userID: userID,
                name: name,
                email: email,
                comment: comment,
                keyID: keyID
            )
        )
    }
}

@objc(CPinentryResponse)
final class CPinentryResponse: NSObject {
    @objc var confirmed = false
    @objc var canceled = false
    @objc var declined = false
    @objc var passphrase = ""
    @objc var repeatOkay = false
    @objc var saveInKeychain = false
    @objc var pinFromCache = false
    @objc var keychainUnusable = false

    convenience init(bridgeResult: PinentryBridgeResult) {
        self.init()
        confirmed = bridgeResult.confirmed
        canceled = bridgeResult.canceled
        declined = bridgeResult.declined
        passphrase = bridgeResult.passphrase
        repeatOkay = bridgeResult.repeatOkay
        saveInKeychain = bridgeResult.saveInKeychain
        pinFromCache = bridgeResult.pinFromCache
        keychainUnusable = bridgeResult.keychainUnusable
    }
}

@objc(PinentryMacSwiftRuntime)
final class PinentryMacSwiftRuntime: NSObject {
    @objc static func runRequest(_ request: CPinentryRequest) -> CPinentryResponse {
        let semaphore = DispatchSemaphore(value: 0)
        final class ResponseBox: @unchecked Sendable {
            var response = CPinentryResponse()
        }
        let responseBox = ResponseBox()

        DispatchQueue.main.async {
            Task { @MainActor in
                responseBox.response = await present(request)
                semaphore.signal()
            }
        }

        semaphore.wait()
        return responseBox.response
    }

    @MainActor
    private static func present(_ request: CPinentryRequest) async -> CPinentryResponse {
        let mapper = PinentryRequestMapper()
        let model = mapper.map(request.payload)
        // The agent is only asked for a quality value (INQUIRE QUALITY) when it
        // requested the quality bar itself; otherwise the local estimate is used.
        let qualityEstimator = PinentryInquiryQualityEstimator(
            pinentryPointer: request.qualityBarRequested ? request.pinentryPointer : nil
        )
        let coordinator = WindowPresentationCoordinator(
            qualityEstimator: qualityEstimator
        )
        let viewModel = coordinator.show(dialog: model)

        NSApp.setActivationPolicy(.accessory)

        // Nothing is shown while the system Touch ID sheet is up; the alert only
        // appears if Touch ID is unavailable, cancelled, or fails. It runs a modal
        // loop, so it has to start after `waitForResponse` has registered itself.
        //
        // The modal loop must not be entered from inside a block of the GCD main queue
        // (DispatchQueue.main.async): that queue is serial, so while such a block is
        // running nothing else is ever taken from it, which starves everything that
        // needs the main actor or the main queue (the countdown, the timeout that
        // cancels the dialog). A run loop block does not hold the queue.
        let runner = AlertDialogRunner(viewModel: viewModel)
        coordinator.beginAutomaticTouchIDIfNeeded {
            RunLoop.main.perform(inModes: [.common]) {
                MainActor.assumeIsolated {
                    runner.run()
                }
            }
        }

        let response = await coordinator.waitForResponse()

        let bridgeResult = PinentryResultWriter().makeResult(
            from: response,
            repeatExpected: request.repeatPassphrase
        )
        return CPinentryResponse(bridgeResult: bridgeResult)
    }
}

private struct PinentryInquiryQualityEstimator: PassphraseQualityEstimating {
    let pinentryPointer: UnsafeMutableRawPointer?
    private let fallback = HeuristicQualityEstimator()

    func evaluate(passphrase: String) -> PassphraseQualityAssessment? {
        guard !passphrase.isEmpty else {
            return nil
        }

        guard let pinentryPointer else {
            return fallback.evaluate(passphrase: passphrase)
        }

        let quality = passphrase.withCString { cString in
            PinentryMacSwiftEvaluateQuality(
                pinentryPointer,
                cString,
                strlen(cString)
            )
        }

        guard quality > 0 else {
            return fallback.evaluate(passphrase: passphrase)
        }

        let score = min(max(Double(quality) / 100.0, 0.0), 1.0)
        let grade: PassphraseQualityAssessment.Grade
        switch score {
        case 0.85...:
            grade = .strong
        case 0.55...:
            grade = .good
        case 0.3...:
            grade = .fair
        default:
            grade = .weak
        }

        return PassphraseQualityAssessment(score: score, grade: grade)
    }
}
