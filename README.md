# Pinentry Mac Swift

[English](README.en.md)

本项目基于 SwiftUI 重写了 pinentry mac 图形界面，支持 macOS 13 及以上系统。SwiftUI 版本代码位于 `macosx-swift` 目录。

![Git signing prompt](README/git.png)
![VS Code prompt](README/vscode.png)

## 特性

- 原生 SwiftUI 界面
- 基于 Pinentry Mac 重写 UI，支持完整 pinentry 功能
- Keychain 缓存支持 Touch ID 验证
- 尽可能获取发起应用的图标
- 多语言支持

## 安装

- 从仓库 Releases 下载 app。
- 将 app 拖入“应用程序”目录，或者任意你想放置的位置。
- 修改 gpg 配置文件 `~/.gnupg/gpg-agent.conf`，将 `pinentry-program` 指向 app 路径下的 `Contents/MacOS/pinentry-modern`。
- 重新加载 gpg-agent 配置：

```shell
gpgconf --kill gpg-agent
```

请不要将 `pinentry-modern` 可执行文件单独从 app 中分离出来使用，否则会导致多语言支持失效。

## 构建

构建依赖一些需要安装的工具，请确保 `libassuan`、`secmem` 和 `gpg-error` 已在系统中安装。

```shell
xcodebuild -project "macosx-swift/pinentry-mac-swift.xcodeproj" -scheme "pinentry-mac-swift" -c=/prionation Debug -destination 'platform=macOS' -derivedDataPath /private/tmp/pinentry-mac-modern-build build
```

## Windows 原生版本

WinUI 3 原生版本位于 `windows-winui3`，使用 C++/WinRT，并复用 `pinentry` 与 `secmem` C 核心。该版本面向 Gpg4win/Windows 原生使用场景，包含 Credential Manager、Windows Hello、重复口令、质量条和超时对话框支持。

Windows 目标机器除了完整 Release 输出目录中的 app-local Windows App SDK 文件外，还需要安装最新版 Microsoft Visual C++ Redistributable x64。Visual Studio 构建和 Gpg4win 集成说明见 `windows-winui3/README.md`。

## 感谢

- 本项目使用 GPT 5.5 辅助完成。
- 感谢 [pinentry-touchid](https://github.com/jorgelbg/pinentry-touchid) 项目对 Touch ID 相关功能的灵感。
