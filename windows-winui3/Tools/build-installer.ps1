param(
    [string]$Version = "0.1.0",
    [string]$SourceDir,
    [string]$IsccPath
)

$ErrorActionPreference = "Stop"

$WindowsDir = Split-Path -Parent $PSScriptRoot
$InstallerScript = Join-Path $WindowsDir "Installer\pinentry-winui3.iss"

if (-not $SourceDir) {
    $SourceDir = Join-Path $WindowsDir "bin\x64\Release"
}

function Resolve-IsccPath {
    param([string]$ExplicitPath)

    if ($ExplicitPath) {
        if (Test-Path -LiteralPath $ExplicitPath) {
            return (Resolve-Path -LiteralPath $ExplicitPath).Path
        }

        throw "ISCC.exe was not found at '$ExplicitPath'."
    }

    $command = Get-Command "iscc.exe" -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    $candidateRoots = @(
        ${env:ProgramFiles},
        ${env:ProgramFiles(x86)},
        (Join-Path $env:LocalAppData "Programs")
    ) | Where-Object { $_ }

    $candidatePaths = foreach ($root in $candidateRoots) {
        Join-Path $root "Inno Setup 7\ISCC.exe"
        Join-Path $root "Inno Setup 6\ISCC.exe"
    }

    foreach ($path in $candidatePaths) {
        if (Test-Path -LiteralPath $path) {
            return (Resolve-Path -LiteralPath $path).Path
        }
    }

    throw @"
ISCC.exe was not found.

Install Inno Setup 7, then run this script again:
  winget install --id JRSoftware.InnoSetup.7 --exact --source winget --interactive

If ISCC.exe is installed in a custom directory, pass it explicitly:
  .\Tools\build-installer.ps1 -IsccPath "C:\Path\To\ISCC.exe"
"@
}

$ResolvedSourceDir = Resolve-Path -LiteralPath $SourceDir -ErrorAction SilentlyContinue
if (-not $ResolvedSourceDir) {
    throw "Release output directory was not found: '$SourceDir'. Build Release|x64 first, or pass -SourceDir."
}

$ExePath = Join-Path $ResolvedSourceDir.Path "pinentry-winui3.exe"
if (-not (Test-Path -LiteralPath $ExePath)) {
    throw "pinentry-winui3.exe was not found in '$($ResolvedSourceDir.Path)'. Build Release|x64 first, or pass -SourceDir."
}

$ResolvedIsccPath = Resolve-IsccPath -ExplicitPath $IsccPath

Write-Host "Using Inno Setup compiler: $ResolvedIsccPath"
Write-Host "Using Release output: $($ResolvedSourceDir.Path)"
Write-Host "Building installer version: $Version"

& $ResolvedIsccPath `
    $InstallerScript `
    "/DMyAppVersion=$Version" `
    "/DSourceDir=$($ResolvedSourceDir.Path)"

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
