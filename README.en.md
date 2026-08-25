[简体中文](README.md)

# Pinentry Modern

Pinentry Modern provides a more modern Pinentry GUI implementation with interfaces and interaction patterns that better match each operating system.

## Supported Platforms

| Platform | Minimum Version | Screenshot |  |
| ----- | ----- | ----- | ----- |
| Windows | Windows 10 <small>10.0.17763.0</small> | <img src="README/Windows 11.png" style="max-width:256px;min-height:200px" /> | <img src="README/Windows 10.png" style="max-width:256px;min-height:200px" /> |
| macOS | macOS Ventura <small>13.0</small> | <img src="README/MacOS 27.png" style="max-width:256px;min-height:200px" /> | <img src="README/MacOS 16.png" style="max-width:256px;min-height:200px" /> |

## Installation

### Windows

- Download the installer from the repository Releases: ```pinentry-modern-setup-<version>.exe```
- Install Pinentry Modern. The default install location is ```%AppData%\Local\Programs\pinentry-modern```
- Edit the gpg configuration file `~/.gnupg/gpg-agent.conf`, and point `pinentry-program` to `pinentry-modern.exe` in the Pinentry Modern install directory.
- Reload the gpg-agent configuration: ```gpgconf --kill gpg-agent```

Example configuration:

```text
pinentry-program C:\Users\<username>\AppData\Local\Programs\pinentry-modern\pinentry-modern.exe
```

### macOS

- Download the app archive from the repository Releases: ```pinentry-modern-<version>.app.zip```
- Extract it, then move the app to `/Applications`, or any other location you prefer.
- Edit the gpg configuration file `~/.gnupg/gpg-agent.conf`, and point `pinentry-program` to `pinentry-modern.app/Contents/MacOS/pinentry-modern` inside the Pinentry Modern app bundle.

Example configuration:

```text
pinentry-program /Applications/pinentry-modern.app/Contents/MacOS/pinentry-modern
```

## Build

### macOS Swift (`macosx-swift`)

The Swift version lives in `macosx-swift` and targets macOS 13 or later. Before building, install:

- Xcode 14.1 or later, including the Command Line Tools.
- Headers and dynamic libraries for `libassuan` and `libgpg-error`. You can install [GPG Suite](https://gpgtools.org/), or use Homebrew:

```shell
brew install libassuan libgpg-error
```

The bundled `pinentry` and `secmem` C sources are built automatically as static libraries by `macosx-swift/Scripts/build-pinentry-core-libs.sh`; `secmem` does not need to be installed separately. The scripts search `/usr/local/MacGPG2`, `/opt/homebrew`, `/usr/local`, and Homebrew `opt` prefixes by default. If your dependencies are installed elsewhere, set:

```shell
export PINENTRY_MAC_SWIFT_ASSUAN_PREFIX="/path/to/libassuan/prefix"
export PINENTRY_MAC_SWIFT_GPG_ERROR_PREFIX="/path/to/libgpg-error/prefix"
```

Debug build:

```shell
xcodebuild \
  -project "macosx-swift/pinentry-mac-swift.xcodeproj" \
  -scheme "pinentry-mac-swift" \
  -configuration Debug \
  -destination "platform=macOS" \
  -derivedDataPath /private/tmp/pinentry-mac-modern-build \
  build
```

For a Release build, change `-configuration Debug` to `-configuration Release`. The app is produced under `DerivedData` at `Build/Products/<Configuration>/pinentry-modern.app`. The build phases bundle `libassuan.0.dylib` and `libgpg-error.0.dylib` into `Contents/Frameworks` inside the app. Do not run `Contents/MacOS/pinentry-modern` outside the app bundle, because localized resources and bundled dynamic libraries may no longer be found.

### Windows WinUI 3 (`windows-winui3`)

The WinUI 3 native version lives in `windows-winui3`. It uses C++/WinRT and reuses the shared `pinentry` and `secmem` C core. This version targets native Gpg4win/Windows usage and includes Credential Manager, Windows Hello, repeated passphrase, quality bar, and timeout dialog support.

Before building, install:

- Visual Studio 2022 with the "Desktop development with C++" and "Universal Windows Platform development" workloads, including the MSVC v143 toolset.
- Windows SDK 10.0.22621.0 or later.
- NuGet CLI, or use the NuGet restore support built into Visual Studio.
- Gpg4win. By default, the project searches `C:\Program Files\Gpg4win\include` and `C:\Program Files\Gpg4win\lib` for the `libgpg-assuan.imp` and `libgpg-error.imp` import libraries and their headers.

If Gpg4win or the GnuPG libraries are installed in a custom location, pass these properties to MSBuild:

```powershell
/p:AssuanIncludeDir="C:\Path\To\include" /p:AssuanLibDir="C:\Path\To\lib"
```

Restore the NuGet packages declared in `packages.config` before the first build. They include Windows App SDK, Windows App SDK WinUI, C++/WinRT, Windows SDK BuildTools, and WebView2:

```powershell
nuget restore .\windows-winui3\PinentryModern.sln
```

Then build from an "x64 Native Tools Command Prompt for VS 2022" or Visual Studio Developer PowerShell:

```powershell
msbuild .\windows-winui3\PinentryModern.sln /p:Configuration=Release /p:Platform=x64
```

You can also open `windows-winui3/PinentryModern.sln` in Visual Studio, restore NuGet packages, and build `Release|x64`. The Release output directory is `windows-winui3/bin/x64/Release`.

The project sets `WindowsAppSDKSelfContained=true`, so the Release directory contains the app-local WinUI/Windows App SDK dependencies. Keep the whole output directory together when distributing or installing the app; do not copy only `pinentry-modern.exe`. Target machines also need the Microsoft Visual C++ Redistributable x64. The Gpg4win `libgpg-assuan` and `libgpg-error` runtime DLLs must also be loadable: keep Gpg4win installed at its default location, or copy the required DLLs into the Release output directory.

Building the installer requires Inno Setup 7:

```powershell
winget install --id JRSoftware.InnoSetup.7 --exact --source winget --interactive
.\windows-winui3\Tools\build-installer.ps1 -Version "<version>"
```

## Acknowledgements

- This project was assisted by GPT 5.5.
- Thanks to the [pinentry-touchid](https://github.com/jorgelbg/pinentry-touchid) project for inspiration on Touch ID related features.
