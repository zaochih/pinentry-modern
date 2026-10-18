import Foundation

/// Read access to human-readable remarks attached to OpenPGP key IDs.
///
/// A remark is free text such as "Git commit signing"; OpenPGP itself has no
/// place to label a single subkey, so the labels live next to the pinentry.
protocol KeyRemarkProviding {
    func remark(forKeyID keyID: String) -> String?
}

/// Read/write access to the remarks, used by the `remark` command line interface.
protocol KeyRemarkEditing: KeyRemarkProviding {
    /// Every stored remark, keyed by (normalized) key ID.
    func allRemarks() throws -> [String: String]

    /// Sets or replaces the remark. An empty remark removes the entry.
    func setRemark(_ remark: String, forKeyID keyID: String) throws

    /// Removes the remark. Returns `false` when there was nothing to remove.
    @discardableResult
    func removeRemark(forKeyID keyID: String) throws -> Bool
}

enum KeyRemarkError: Error, LocalizedError {
    case invalidKeyID(String)
    case unreadableFile(URL, underlying: Error)
    case malformedFile(URL)

    var errorDescription: String? {
        switch self {
        case .invalidKeyID(let value):
            return "'\(value)' is not a key ID: expected 8 or 16 hex digits, or a 40 digit fingerprint"
        case .unreadableFile(let url, let underlying):
            return "cannot read \(url.path): \(underlying.localizedDescription)"
        case .malformedFile(let url):
            return "\(url.path) is not a JSON object mapping key IDs to remark strings"
        }
    }
}

enum KeyID {
    /// Reduces a short ID, long ID or fingerprint to uppercase hex, ignoring
    /// whitespace and a leading `0x`. A fingerprint becomes its 16 digit long
    /// ID. Returns `nil` unless the input is 8, 16 or 40 hex digits.
    static func normalize(_ raw: String) -> String? {
        var text = raw.filter { !$0.isWhitespace }.uppercased()
        if text.hasPrefix("0X") {
            text.removeFirst(2)
        }

        guard !text.isEmpty, text.allSatisfy({ $0.isHexDigit }) else {
            return nil
        }

        switch text.count {
        case 8, 16:
            return text
        case 40:
            return String(text.suffix(16))
        default:
            return nil
        }
    }

    /// Equal IDs match; a short ID also matches a long ID that ends with it.
    static func matches(_ lhs: String, _ rhs: String) -> Bool {
        if lhs == rhs {
            return true
        }
        if lhs.count == 8 {
            return rhs.hasSuffix(lhs)
        }
        if rhs.count == 8 {
            return lhs.hasSuffix(rhs)
        }
        return false
    }
}

/// Remarks stored in a small hand-editable JSON file:
///
///     { "6713E2BCD4E25630": "Git commit signing" }
struct FileKeyRemarkStore: KeyRemarkEditing {
    static let environmentVariable = "PINENTRY_MODERN_REMARKS"

    let url: URL

    init(url: URL = FileKeyRemarkStore.defaultURL()) {
        self.url = url
    }

    static func defaultURL(
        environment: [String: String] = ProcessInfo.processInfo.environment,
        fileManager: FileManager = .default
    ) -> URL {
        if let override = environment[environmentVariable], !override.isEmpty {
            return URL(fileURLWithPath: (override as NSString).expandingTildeInPath)
        }

        let base = fileManager.urls(for: .applicationSupportDirectory, in: .userDomainMask).first
            ?? fileManager.homeDirectoryForCurrentUser.appendingPathComponent("Library/Application Support")
        return base.appendingPathComponent("pinentry-modern/remarks.json")
    }

    func remark(forKeyID keyID: String) -> String? {
        guard
            let normalized = KeyID.normalize(keyID),
            let remarks = try? allRemarks()
        else {
            return nil
        }

        if let exact = remarks[normalized] {
            return exact
        }

        // A remark is a trust cue in the prompt, so it must never be the wrong one: an
        // 8 digit short ID is only taken when it identifies exactly one entry.
        let candidates = remarks.filter { KeyID.matches($0.key, normalized) }
        return candidates.count == 1 ? candidates.first?.value : nil
    }

    func allRemarks() throws -> [String: String] {
        // Sorted so that two spellings of the same key in a hand-edited file resolve the
        // same way every time.
        var result: [String: String] = [:]
        for (key, value) in try load().sorted(by: { $0.key < $1.key }) {
            result[KeyID.normalize(key) ?? key] = value
        }
        return result
    }

    func setRemark(_ remark: String, forKeyID keyID: String) throws {
        guard let normalized = KeyID.normalize(keyID) else {
            throw KeyRemarkError.invalidKeyID(keyID)
        }

        var remarks = try load().filter { KeyID.normalize($0.key) != normalized }
        let trimmed = remark.trimmingCharacters(in: .whitespacesAndNewlines)
        if !trimmed.isEmpty {
            remarks[normalized] = trimmed
        }
        try save(remarks)
    }

    @discardableResult
    func removeRemark(forKeyID keyID: String) throws -> Bool {
        guard let normalized = KeyID.normalize(keyID) else {
            throw KeyRemarkError.invalidKeyID(keyID)
        }

        let existing = try load()
        let remaining = existing.filter { KeyID.normalize($0.key) != normalized }
        guard remaining.count != existing.count else {
            return false
        }

        try save(remaining)
        return true
    }

    // Entries are read and written with their original keys so that a hand-edited
    // file is never silently rewritten beyond the entry being changed.
    private func load() throws -> [String: String] {
        guard FileManager.default.fileExists(atPath: url.path) else {
            return [:]
        }

        let data: Data
        do {
            data = try Data(contentsOf: url)
        } catch {
            throw KeyRemarkError.unreadableFile(url, underlying: error)
        }

        if data.allSatisfy({ $0 == 0x20 || $0 == 0x0A || $0 == 0x0D || $0 == 0x09 }) {
            return [:]
        }

        guard let object = try? JSONSerialization.jsonObject(with: data) as? [String: String] else {
            throw KeyRemarkError.malformedFile(url)
        }
        return object
    }

    private func save(_ remarks: [String: String]) throws {
        let directory = url.deletingLastPathComponent()
        let fileManager = FileManager.default
        try fileManager.createDirectory(
            at: directory,
            withIntermediateDirectories: true,
            attributes: [.posixPermissions: 0o700]
        )
        // Also for a directory that already existed.
        try fileManager.setAttributes([.posixPermissions: 0o700], ofItemAtPath: directory.path)

        var data = try JSONSerialization.data(
            withJSONObject: remarks,
            options: [.prettyPrinted, .sortedKeys, .withoutEscapingSlashes]
        )
        data.append(0x0A)

        // Written to a 0600 temporary file next to the target and moved into place, so the
        // content is never readable by others, not even briefly.
        let temporary = directory.appendingPathComponent(".remarks-\(UUID().uuidString).tmp")
        guard fileManager.createFile(atPath: temporary.path, contents: data, attributes: [.posixPermissions: 0o600]) else {
            throw KeyRemarkError.unreadableFile(temporary, underlying: CocoaError(.fileWriteUnknown))
        }
        do {
            _ = try fileManager.replaceItemAt(url, withItemAt: temporary)
        } catch {
            try? fileManager.removeItem(at: temporary)
            throw error
        }
    }
}
