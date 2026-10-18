import Foundation

/// `pinentry-modern remark ...`: command line interface for the key remarks.
///
/// Invoked from `main` before any pinentry option parsing, so it never talks
/// to gpg-agent and never opens a window.
@objc(PinentryRemarkCommand)
final class PinentryRemarkCommand: NSObject {
    static let usage = """
    usage: pinentry-modern remark <command>

      list                  print every remark as KEYID<TAB>remark
      get <keyid>           print the remark of one key
      set <keyid> <text>    set or replace the remark (empty text removes it)
      remove <keyid>        delete the remark
      path                  print the file the remarks are stored in

    <keyid> may be a short ID, a long ID or a full fingerprint.
    The file location can be overridden with $PINENTRY_MODERN_REMARKS.
    """

    /// - Parameter arguments: everything after the `remark` word.
    /// - Returns: 0 on success, 1 on failure or when nothing matched, 2 on usage errors.
    @objc(runWithArguments:)
    static func run(arguments: [String]) -> Int32 {
        execute(arguments, store: FileKeyRemarkStore())
    }

    static func execute(_ arguments: [String], store: FileKeyRemarkStore) -> Int32 {
        guard let command = arguments.first else {
            writeError(usage)
            return 2
        }

        let rest = Array(arguments.dropFirst())

        do {
            switch (command, rest.count) {
            case ("list", 0):
                for (keyID, remark) in try store.allRemarks().sorted(by: { $0.key < $1.key }) {
                    print("\(keyID)\t\(remark)")
                }
                return 0
            case ("get", 1):
                guard KeyID.normalize(rest[0]) != nil else {
                    throw KeyRemarkError.invalidKeyID(rest[0])
                }
                guard let remark = store.remark(forKeyID: rest[0]) else {
                    return 1
                }
                print(remark)
                return 0
            case ("set", 2...):
                try store.setRemark(rest.dropFirst().joined(separator: " "), forKeyID: rest[0])
                return 0
            case ("remove", 1):
                return try store.removeRemark(forKeyID: rest[0]) ? 0 : 1
            case ("path", 0):
                print(store.url.path)
                return 0
            default:
                writeError(usage)
                return 2
            }
        } catch {
            writeError("pinentry-modern: \(error.localizedDescription)")
            return 1
        }
    }

    private static func writeError(_ message: String) {
        FileHandle.standardError.write(Data((message + "\n").utf8))
    }
}
