# Pinentry Mac Swift

[简体中文](README.md)

This project rewrites the pinentry mac GUI with SwiftUI and supports macOS 13 and later. The SwiftUI implementation lives in the `macosx-swift` directory.

![Git signing prompt](README/git.png)
![VS Code prompt](README/vscode.png)

## Features

- Native SwiftUI interface
- Rewritten UI based on Pinentry Mac with full pinentry feature support
- Keychain caching with Touch ID authentication
- Best-effort source application icon detection
- Multi-language support

## Installation

- Download the app from the repository Releases.
- Move the app into Applications, or any directory you prefer.
- Edit `~/.gnupg/gpg-agent.conf` and point `pinentry-program` to `Contents/MacOS/pinentry-mac-swift` inside the app bundle.
- Reload the gpg-agent configuration:

```shell
gpgconf --kill gpg-agent
```

Do not separate the `pinentry-mac-swift` executable from the app bundle. Doing so will break multi-language support.

## Build

The build depends on tools and libraries that need to be installed. Make sure `libassuan`, `secmem`, and `gpg-error` are available on the system.

```shell
xcodebuild -project "macosx-swift/pinentry-mac-swift.xcodeproj" -scheme "pinentry-mac-swift" -c=/prionation Debug -destination 'platform=macOS' -derivedDataPath /private/tmp/pinentry-mac-modern-build build
```

## Windows Native Version

The WinUI 3 native version lives in `windows-winui3`. It uses C++/WinRT and reuses the shared `pinentry` and `secmem` C core. This version targets native Gpg4win/Windows usage and includes Credential Manager, Windows Hello, repeated passphrase, quality bar, and timeout dialog support.

Windows target machines need the latest supported Microsoft Visual C++ Redistributable x64 in addition to the app-local Windows App SDK files from the full Release output directory. See `windows-winui3/README.md` for Visual Studio build and Gpg4win integration notes.

## Acknowledgements

- This project was assisted by GPT 5.5.
- Thanks to the [pinentry-touchid](https://github.com/jorgelbg/pinentry-touchid) project for inspiration on Touch ID related features.
