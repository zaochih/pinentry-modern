import AppKit
import SwiftUI

/// Content placed in the accessory area of the system `NSAlert`: everything the
/// alert itself cannot express (error text, passphrase fields, strength bar,
/// options, countdown). Title, message, icon, buttons and the glass window are
/// drawn by the system.
///
/// A visible `NSAlert` does not grow when its accessory gets taller, so every
/// optional row (the "does not match" hint) reserves its space up front and is
/// only faded in.
struct DialogAccessoryView: View {
    @ObservedObject var viewModel: DialogViewModel
    let submit: () -> Void

    @FocusState private var focusedField: Field?

    private enum Field: Hashable {
        case passphrase
        case repeatedPassphrase
    }

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            if viewModel.state.showsErrorSection {
                errorSection
            }

            if viewModel.dialog.mode == .passphrase {
                passphraseSection
            }

            if let keyboardPrompt = viewModel.keyboardPrompt {
                Text(keyboardPrompt)
                    .font(.footnote)
                    .foregroundStyle(Color.secondary)
            }
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .onAppear {
            focusPassphraseField()
        }
        .onChange(of: viewModel.showTypedPassphrase) { _ in
            focusPassphraseField()
        }
    }

    private var errorSection: some View {
        Label {
            Text(viewModel.dialog.errorText ?? "")
                .fixedSize(horizontal: false, vertical: true)
        } icon: {
            Image(systemName: "exclamationmark.triangle.fill")
        }
        .font(.callout)
        .foregroundStyle(Color(nsColor: .systemRed))
    }

    private var passphraseSection: some View {
        VStack(alignment: .leading, spacing: 10) {
            passwordField(
                title: viewModel.promptText,
                text: $viewModel.passphrase,
                field: .passphrase
            )

            if viewModel.state.showsRepeatField {
                passwordField(
                    title: viewModel.confirmPromptText,
                    text: $viewModel.repeatedPassphrase,
                    field: .repeatedPassphrase
                )

                // Always laid out and only faded in: a visible NSAlert does not grow
                // when its accessory gets taller, so the line has to be reserved.
                Text(L10n.mismatchHint)
                    .font(.footnote.weight(.medium))
                    .foregroundStyle(Color(nsColor: .systemRed))
                    .opacity(viewModel.mismatchHint == nil ? 0 : 1)
            }

            if viewModel.dialog.showsQualityBar {
                qualitySection
            }

            if viewModel.state.showsOptionsSection {
                optionsSection
            }
        }
    }

    private var qualitySection: some View {
        let assessment = viewModel.qualityAssessment

        return VStack(alignment: .leading, spacing: 4) {
            HStack {
                Text(L10n.qualityTitle)
                    .font(.callout)
                    .foregroundStyle(Color.secondary)

                Spacer()

                Text(assessment?.label ?? " ")
                    .font(.callout.weight(.semibold))
                    .foregroundStyle(assessment.map(qualityColor) ?? Color.secondary)
            }

            QualityBar(
                score: assessment?.score ?? 0,
                color: assessment.map(qualityColor) ?? Color.secondary
            )
        }
    }

    private var optionsSection: some View {
        VStack(alignment: .leading, spacing: 6) {
            if viewModel.dialog.showTypingToggleAvailable {
                Toggle(L10n.showTyping, isOn: $viewModel.showTypedPassphrase)
                    .toggleStyle(.checkbox)
            }

            if viewModel.dialog.canUseKeychain {
                Toggle(L10n.saveInKeychain, isOn: $viewModel.saveInKeychain)
                    .toggleStyle(.checkbox)
            }
        }
        .font(.callout)
    }

    private func passwordField(title: String, text: Binding<String>, field: Field) -> some View {
        Group {
            if viewModel.showTypedPassphrase {
                TextField(title, text: text)
            } else {
                SecureField(title, text: text)
            }
        }
        .textFieldStyle(.roundedBorder)
        .focused($focusedField, equals: field)
        .onSubmit {
            // Return in the first of two fields goes on to the second one instead of
            // submitting a passphrase that was not repeated yet.
            if field == .passphrase, viewModel.state.showsRepeatField {
                focusedField = .repeatedPassphrase
            } else {
                submit()
            }
        }
    }

    private func focusPassphraseField() {
        guard viewModel.dialog.mode == .passphrase else {
            return
        }

        // The field only exists once the alert has put the view into its window.
        for delay in [0.0, 0.08, 0.2] {
            DispatchQueue.main.asyncAfter(deadline: .now() + delay) {
                focusedField = .passphrase
            }
        }
    }

    private func qualityColor(for assessment: PassphraseQualityAssessment) -> Color {
        switch assessment.grade {
        case .strong:
            return Color(nsColor: .systemGreen)
        case .good:
            return Color(nsColor: .systemYellow)
        case .fair:
            return Color(nsColor: .systemOrange)
        case .weak:
            return Color(nsColor: .systemRed)
        }
    }
}

/// A plain SwiftUI strength bar. `ProgressView` is backed by `NSProgressIndicator`,
/// whose fill animation restarted on every keystroke inside the alert's modal loop
/// and left the bar short of the value its label already reported.
private struct QualityBar: View {
    let score: Double
    let color: Color

    var body: some View {
        GeometryReader { proxy in
            ZStack(alignment: .leading) {
                Capsule()
                    .fill(Color.primary.opacity(0.14))

                Capsule()
                    .fill(color)
                    .frame(width: proxy.size.width * CGFloat(min(max(score, 0), 1)))
            }
            // Eases from the previous value to the new one, colour included.
            .animation(.easeOut(duration: 0.22), value: score)
        }
        .frame(height: 6)
    }
}
