import Foundation

@MainActor
final class WindowPresentationCoordinator: ObservableObject, PinentryPresenter {
    @Published private(set) var activeViewModel: DialogViewModel?

    private let iconResolver: any IconResolving
    private let qualityEstimator: any PassphraseQualityEstimating
    private let timeoutController: any TimeoutControlling
    private var continuation: CheckedContinuation<DialogResponse, Never>?

    init(
        iconResolver: any IconResolving = IconResolver(),
        qualityEstimator: any PassphraseQualityEstimating = HeuristicQualityEstimator(),
        timeoutController: any TimeoutControlling = TimeoutController()
    ) {
        self.iconResolver = iconResolver
        self.qualityEstimator = qualityEstimator
        self.timeoutController = timeoutController
    }

    func present(dialog: DialogModel) async -> DialogResponse {
        show(dialog: dialog)

        return await waitForResponse()
    }

    @discardableResult
    func show(dialog: DialogModel) -> DialogViewModel {
        let viewModel = makeViewModel(for: dialog)
        activeViewModel = viewModel
        return viewModel
    }

    func waitForResponse() async -> DialogResponse {
        return await withCheckedContinuation { continuation in
            self.continuation = continuation
        }
    }

    /// Tries the system Touch ID sheet first. `onPasswordEntryNeeded` fires when
    /// there is nothing to unlock with Touch ID, or it was cancelled or failed,
    /// i.e. when the manual passphrase window has to be shown.
    func beginAutomaticTouchIDIfNeeded(onPasswordEntryNeeded: @escaping () -> Void) {
        guard let viewModel = activeViewModel else {
            onPasswordEntryNeeded()
            return
        }

        viewModel.onPasswordEntryNeeded = onPasswordEntryNeeded
        viewModel.beginAutomaticTouchIDIfNeeded()
    }

    private func makeViewModel(for dialog: DialogModel) -> DialogViewModel {
        DialogViewModel(
            dialog: dialog,
            iconResolver: iconResolver,
            qualityEstimator: qualityEstimator,
            timeoutController: timeoutController
        ) { [weak self] response in
            self?.finish(response)
        }
    }

    private func finish(_ response: DialogResponse) {
        continuation?.resume(returning: response)
        continuation = nil
    }
}
