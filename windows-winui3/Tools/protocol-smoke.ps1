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

    function Get-QualityScore {
        param([string]$Passphrase)

        if ([string]::IsNullOrEmpty($Passphrase)) {
            return 0
        }

        $score = [Math]::Min(100, $Passphrase.Length * 8)
        if ($Passphrase -match '[a-z]') {
            $score += 10
        }
        if ($Passphrase -match '[A-Z]') {
            $score += 10
        }
        if ($Passphrase -match '\d') {
            $score += 10
        }
        if ($Passphrase -match '[^a-zA-Z0-9]') {
            $score += 10
        }

        return [Math]::Min(100, $score)
    }

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
            if ($line -match '^INQUIRE QUALITY ?(.*)$') {
                $score = Get-QualityScore $Matches[1]
                Write-Host "client: D $score"
                $process.StandardInput.WriteLine("D $score")
                Write-Host "client: END"
                $process.StandardInput.WriteLine("END")
                $process.StandardInput.Flush()
            }
        } while ($line -notmatch "^(OK|ERR|D )")
    }
}
finally {
    if (-not $process.HasExited) {
        $process.Kill()
    }
    $process.Dispose()
}
