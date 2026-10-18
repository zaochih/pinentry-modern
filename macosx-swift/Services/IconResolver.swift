import AppKit

protocol IconResolving {
    func resolve(_ source: DialogModel.IconSource) -> NSImage?
}

protocol CallerNameResolving {
    /// Name of the application that (indirectly) started the process `ownerPID`.
    func callerName(ownerPID: Int) -> String?
}

struct IconResolver: IconResolving, CallerNameResolving {
    func resolve(_ source: DialogModel.IconSource) -> NSImage? {
        switch source {
        case .systemSymbol:
            return nil
        case .bundledImage(let name):
            return NSImage(named: name)
        case .filePath(let path):
            return NSImage(contentsOfFile: path)
        case .ownerProcessID(let processID):
            return callerApplication(ownerPID: processID).flatMap(icon(for:))
        case .appIcon:
            if let image = AppRuntimeBundle.bundle.image(forResource: NSImage.Name("Icon")) {
                return image
            }

            return NSApplication.shared.applicationIconImage
        }
    }

    func callerName(ownerPID: Int) -> String? {
        guard let application = callerApplication(ownerPID: ownerPID) else {
            return nil
        }

        // Named after the same bundle the icon comes from: inside Electron apps the
        // matched process is a helper ("Code Helper (Plugin)"), the app is "Code".
        if let bundleURL = preferredApplicationBundleURL(for: application.bundleURL) {
            let name = FileManager.default.displayName(atPath: bundleURL.path)
            let trimmed = name.hasSuffix(".app") ? String(name.dropLast(4)) : name
            if !trimmed.isEmpty {
                return trimmed
            }
        }

        return application.localizedName
    }

    /// Walks up from the requesting process (for gpg-agent that is the `gpg`
    /// process) until it reaches a real application, e.g. the terminal.
    private func callerApplication(ownerPID: Int) -> NSRunningApplication? {
        var currentPID = ownerPID
        var visited = Set<Int>()

        while currentPID > 1, visited.insert(currentPID).inserted, visited.count <= 12 {
            if
                let application = NSRunningApplication(processIdentifier: pid_t(currentPID)),
                application.bundleIdentifier != AppRuntimeBundle.bundle.bundleIdentifier
            {
                return application
            }

            let parentPID = Int(PinentryMacSwiftCopyParentProcessID(Int32(currentPID)))
            guard parentPID > 1, parentPID != currentPID else {
                break
            }

            currentPID = parentPID
        }

        return nil
    }

    private func icon(for application: NSRunningApplication) -> NSImage? {
        if
            let bundleURL = preferredApplicationBundleURL(for: application.bundleURL)
        {
            let icon = NSWorkspace.shared.icon(forFile: bundleURL.path)
            if icon.isValid {
                return icon
            }
        }

        return application.icon
    }

    private func preferredApplicationBundleURL(for bundleURL: URL?) -> URL? {
        guard let bundleURL else {
            return nil
        }

        let candidates = [
            bundleURL.standardizedFileURL,
            bundleURL.resolvingSymlinksInPath().standardizedFileURL
        ]

        for candidate in candidates {
            if let appBundleURL = outermostApplicationBundleURL(containing: candidate) {
                return appBundleURL
            }
        }

        return nil
    }

    private func outermostApplicationBundleURL(containing url: URL) -> URL? {
        var currentURL = url
        var resolvedBundleURL: URL?

        while currentURL.path != "/" {
            if currentURL.pathExtension == "app" {
                resolvedBundleURL = currentURL
            }

            currentURL.deleteLastPathComponent()
        }

        return resolvedBundleURL
    }
}
