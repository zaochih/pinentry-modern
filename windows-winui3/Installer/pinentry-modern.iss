#define MyAppName "Pinentry Windows"
#define MyAppPublisher "Stapxs"
#define MyAppExeName "pinentry-modern.exe"
#ifndef MyAppVersion
#define MyAppVersion "1.0.1"
#endif
#ifndef SourceDir
#define SourceDir "..\bin\x64\Release"
#endif

[Setup]
AppId={{DD6D00FD-160A-4F52-BA7B-1B6E32B73F67}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={localappdata}\Programs\pinentry-modern
DisableProgramGroupPage=yes
DisableWelcomePage=no
OutputBaseFilename=pinentry-modern-setup-{#MyAppVersion}
OutputDir=..\..\dist
ArchitecturesAllowed=x64compatible or arm64
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest
SetupLogging=yes
SolidCompression=yes
Compression=lzma2
WizardStyle=modern dynamic windows11
LanguageDetectionMethod=uilanguage
ShowLanguageDialog=auto
UninstallDisplayIcon={app}\{#MyAppExeName}
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription={#MyAppName} Setup
VersionInfoProductName={#MyAppName}
VersionInfoProductVersion={#MyAppVersion}
VersionInfoCopyright=Copyright (C) 2026 Stapxs. Released under GPL - see COPYING

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "zh_cn"; MessagesFile: "Languages\ChineseSimplified.isl"

[CustomMessages]
en.GpgPageTitle=Gpg4win configuration
en.GpgPageDescription=Point gpg-agent to the installed pinentry executable.
en.GpgPageLead=After installation, edit your gpg-agent.conf file and add or update this line:
en.GpgPageTypicalPath=Typical gpg-agent.conf path:
en.GpgPageReload=Then reload gpg-agent:
en.GpgPageNote=The WinUI 3 app is installed as a folder. Keep the executable together with the files beside it.
zh_cn.GpgPageTitle=Gpg4win 配置
zh_cn.GpgPageDescription=将 gpg-agent 指向安装后的 pinentry 可执行文件。
zh_cn.GpgPageLead=安装完成后，编辑 gpg-agent.conf，新增或更新这一行：
zh_cn.GpgPageTypicalPath=gpg-agent.conf 常见路径：
zh_cn.GpgPageReload=然后重新加载 gpg-agent：
zh_cn.GpgPageNote=WinUI 3 应用会以文件夹形式安装。请保持可执行文件和旁边的依赖文件在一起。

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Excludes: "*.pdb,*.ilk,*.exp,*.lib"; Flags: ignoreversion recursesubdirs createallsubdirs

[Code]
var
  GpgConfigPage: TOutputMsgMemoWizardPage;

function GpgConfigText: String;
begin
  Result :=
    ExpandConstant('{cm:GpgPageLead}') + #13#10#13#10 +
    'pinentry-program "' + WizardDirValue + '\pinentry-modern.exe"' + #13#10#13#10 +
    ExpandConstant('{cm:GpgPageTypicalPath}') + #13#10 +
    '%APPDATA%\gnupg\gpg-agent.conf' + #13#10#13#10 +
    ExpandConstant('{cm:GpgPageReload}') + #13#10 +
    'gpgconf --kill gpg-agent' + #13#10#13#10 +
    ExpandConstant('{cm:GpgPageNote}');
end;

procedure InitializeWizard;
begin
  GpgConfigPage := CreateOutputMsgMemoPage(
    wpSelectDir,
    ExpandConstant('{cm:GpgPageTitle}'),
    ExpandConstant('{cm:GpgPageDescription}'),
    '',
    '');
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageID = GpgConfigPage.ID then
  begin
    GpgConfigPage.RichEditViewer.Lines.Text := GpgConfigText;
  end;
end;
