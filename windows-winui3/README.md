# Pinentry WinUI 3

This directory contains a Windows-native C++/WinRT + WinUI 3 frontend for the
shared `pinentry/` and `secmem/` C core used by the macOS frontends.

The first target is an unpackaged, self-contained executable that can be used
directly from `gpg-agent.conf`:

```text
pinentry-program C:\path\to\pinentry-winui3.exe
```

## Build

Open `PinentryWinUI3.sln` in Visual Studio 2022 on Windows with these workloads:

- Desktop development with C++
- Windows application development
- Windows App SDK / C++ WinRT tooling

The project expects `libassuan` and `gpg-error` headers/libs to be available.
Set these MSBuild properties if they are not installed in the default location:

```powershell
msbuild PinentryWinUI3.sln `
  /p:AssuanIncludeDir=C:\gpg4win\include `
  /p:AssuanLibDir=C:\gpg4win\lib
```

The project is configured as:

- `WindowsPackageType=None`
- `WindowsAppSDKSelfContained=true`
- `UseWinUI=true`

## Test Checklist

- Protocol smoke test with `SETDESC`, `SETPROMPT`, `GETPIN`, `CONFIRM`,
  `SETREPEAT`, `SETQUALITYBAR`, and `SETTIMEOUT`.
- Gpg4win integration: configure `gpg-agent.conf`, restart `gpg-agent`, then
  test decrypt, sign, Git commit signing, and SSH unlock.
- Credential Manager: save, retrieve, Windows Hello verification, cache miss,
  and cache unusable paths.
- UI verification: English/Simplified Chinese strings, dark/light theme, long
  descriptions, repeated passphrase mismatch, timeout, Enter/Escape behavior,
  and owner icon fallback.

