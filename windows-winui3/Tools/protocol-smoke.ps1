param(
    [Parameter(Mandatory = $true)]
    [string]$PinentryExe
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $PinentryExe)) {
    throw "pinentry executable not found: $PinentryExe"
}

$startInfo = [System.Diagnostics.ProcessStartInfo]::new()
$startInfo.FileName = (Resolve-Path $PinentryExe)
$startInfo.RedirectStandardInput = $true
$startInfo.RedirectStandardOutput = $true
$startInfo.RedirectStandardError = $true
$startInfo.UseShellExecute = $false
$startInfo.CreateNoWindow = $true

$process = [System.Diagnostics.Process]::Start($startInfo)

try {
    $banner = $process.StandardOutput.ReadLine()
    Write-Host "server: $banner"

    $commands = @(
        "OPTION allow-external-password-cache",
        "SETTITLE Unlock Secret Key",
        "SETDESC Please enter the passphrase to unlock the OpenPGP secret key:%0A%22Example User <user@example.com>%22%0A2048-bit RSA key, ID 1234ABCD, created 2026-08-11.",
        "SETPROMPT Passphrase",
        "SETKEYINFO n/1234ABCD1234ABCD",
        "SETQUALITYBAR Passphrase Strength",
        "SETTIMEOUT 60",
        "GETPIN",
        "CONFIRM",
        "BYE"
    )

    foreach ($command in $commands) {
        Write-Host "client: $command"
        $process.StandardInput.WriteLine($command)
        $process.StandardInput.Flush()

        do {
            $line = $process.StandardOutput.ReadLine()
            if ($null -eq $line) {
                break
            }
            Write-Host "server: $line"
        } while ($line -notmatch "^(OK|ERR|D )")
    }
}
finally {
    if (-not $process.HasExited) {
        $process.Kill()
    }
    $process.Dispose()
}

