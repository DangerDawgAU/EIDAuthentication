<#
.SYNOPSIS
    Stage (download + SHA-256 verify) the bundled smart-card minidrivers that the
    NSIS installer (Installerx64.nsi) embeds via `File "drivers\..."`.

.DESCRIPTION
    Single source of truth for the bundled-driver manifest (URL + SHA-256 + size).
    The driver binaries themselves are gitignored (see Installer\drivers\.gitignore),
    so every build that produces the installer must stage them first.

    Consumed by:
      - build.ps1                              (local + windows-ci.yaml builds)
      - .github/workflows/scan-artifacts.yml   (VirusTotal installer packaging)

    Run it directly to stage every driver into Installer\drivers\, or dot-source it
    with -ManifestOnly to reuse $BundledDrivers / Sync-BundledDriver without staging.

.PARAMETER DriversDir
    Target directory for the staged files. Defaults to the drivers folder next to
    this script (Installer\drivers).

.PARAMETER ManifestOnly
    Define $BundledDrivers and Sync-BundledDriver but do not stage anything. For
    callers that dot-source this file purely to reuse its definitions.

.NOTES
    To update a driver: change its Url below and re-run this script. On first run it
    downloads, prints the new SHA-256, and aborts on mismatch; copy the printed hash
    into the manifest. Never relax the hash check - it is what stops an unverified
    (or tampered) vendor driver from being shipped inside the installer.
#>
param(
    [string] $DriversDir = (Join-Path $PSScriptRoot 'drivers'),
    [switch] $ManifestOnly
)

$BundledDrivers = @(
    @{
        Name   = 'MyEID Minidriver (Aventra)'
        File   = 'MyEID_Minidriver.zip'
        Url    = 'https://aventra.fi/wp-content/uploads/2026/03/MyEID_Minidriver_3-0-1-2_Certified.zip'
        Sha256 = 'E9789B800A1201BC7A7F8B72FB3C9C8B10CAAFD4541C013F1383F5BA2CF3A510'
        Size   = 616068
    },
    @{
        Name   = 'YubiKey Minidriver (Yubico)'
        File   = 'YubiKey-Minidriver-5.0.4.273-x64.msi'
        Url    = 'https://downloads.yubico.com/support/YubiKey-Minidriver-5.0.4.273-x64.msi'
        Sha256 = '4C657C148C6DA60127094F8A64345317906A49DF25C3AAD0898C0FEFA6E07FB3'
        Size   = 2994176
    },
    @{
        Name   = 'IDOne PIV 2.4.3 Minidriver (Idemia / Windows Update)'
        File   = 'WindowsUpdate_Minidriver.cab'
        Url    = 'https://catalog.s.download.windowsupdate.com/d/msdownload/update/driver/drvs/2025/10/33c66d53-0881-47f8-9714-081138ca4d26_00cfc05dd035d167c8d86691958366fe061faed5.cab'
        Sha256 = 'F17279C3AA07497874D7DC8E3A4F1F500AA20E9BD273A8DD34BE7A37231356A9'
        Size   = 7838110
    }
)

function Sync-BundledDriver {
    param(
        [Parameter(Mandatory = $true)] [hashtable] $Driver,
        [Parameter(Mandatory = $true)] [string]    $TargetDir
    )

    $target = Join-Path $TargetDir $Driver.File

    if (Test-Path $target) {
        $actual = (Get-FileHash -Path $target -Algorithm SHA256).Hash
        if ($actual -eq $Driver.Sha256) {
            Write-Host ("  [OK]   {0}  -- verified ({1:N0} bytes)" -f $Driver.File, (Get-Item $target).Length) -ForegroundColor Gray
            return
        }
        Write-Host ("  [!!]   {0}  -- hash mismatch (expected {1}, got {2}); re-downloading" -f $Driver.File, $Driver.Sha256, $actual) -ForegroundColor Yellow
        Remove-Item -LiteralPath $target -Force
    }

    Write-Host ("  [DL]   {0}" -f $Driver.File) -ForegroundColor Cyan
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    try {
        Invoke-WebRequest -Uri $Driver.Url -OutFile $target -UseBasicParsing -TimeoutSec 300
    } catch {
        throw ("Could not fetch bundled driver '{0}' from {1}.`n`n" +
               "If this is an offline build host, pre-stage the file at:`n" +
               "  {2}`n`n" +
               "(see Installer\drivers\README.md for the expected SHA-256).`n`n" +
               "Underlying error: {3}") -f $Driver.Name, $Driver.Url, $target, $_.Exception.Message
    }

    $actual = (Get-FileHash -Path $target -Algorithm SHA256).Hash
    if ($actual -ne $Driver.Sha256) {
        Remove-Item -LiteralPath $target -Force -ErrorAction SilentlyContinue
        throw ("SHA-256 mismatch for '{0}' after download.`n" +
               "  expected: {1}`n  got:      {2}`n" +
               "Aborting build to avoid shipping an unverified driver.") -f $Driver.Name, $Driver.Sha256, $actual
    }

    Write-Host ("  [OK]   {0}  -- downloaded and verified" -f $Driver.File) -ForegroundColor Green
}

if ($ManifestOnly) {
    return
}

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "Staging bundled minidrivers" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan

if (-not (Test-Path $DriversDir)) {
    New-Item -ItemType Directory -Path $DriversDir -Force | Out-Null
}

foreach ($d in $BundledDrivers) {
    Sync-BundledDriver -Driver $d -TargetDir $DriversDir
}
Write-Host ""
