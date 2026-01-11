# Cleanup Partial Installation Script
# Run as Administrator

Write-Host "EID Authentication - Cleanup Partial Installation" -ForegroundColor Cyan
Write-Host "=================================================" -ForegroundColor Cyan
Write-Host ""

# Check if running as administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "ERROR: This script must be run as Administrator" -ForegroundColor Red
    Write-Host "Right-click PowerShell and select 'Run as Administrator'" -ForegroundColor Yellow
    exit 1
}

Write-Host "Removing partial installation registry entries..." -ForegroundColor Yellow

# Remove installation path registry
try {
    Remove-Item -Path "HKLM:\Software\EIDAuthentication" -Force -ErrorAction SilentlyContinue
    Write-Host "  [OK] Removed HKLM:\Software\EIDAuthentication" -ForegroundColor Green
} catch {
    Write-Host "  [SKIP] Registry key not found" -ForegroundColor Gray
}

# Remove uninstall entry
try {
    Remove-Item -Path "HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\EIDAuthentication" -Force -ErrorAction SilentlyContinue
    Write-Host "  [OK] Removed uninstall registry entry" -ForegroundColor Green
} catch {
    Write-Host "  [SKIP] Uninstall entry not found" -ForegroundColor Gray
}

# Check for System32 DLLs
Write-Host ""
Write-Host "Checking System32 for DLL files..." -ForegroundColor Yellow

$system32Files = @(
    "$env:SystemRoot\System32\EIDAuthenticationPackage.dll",
    "$env:SystemRoot\System32\EIDCredentialProvider.dll",
    "$env:SystemRoot\System32\EIDPasswordChangeNotification.dll",
    "$env:SystemRoot\System32\EIDConfigurationWizard.exe"
)

foreach ($file in $system32Files) {
    if (Test-Path $file) {
        Write-Host "  [FOUND] $file" -ForegroundColor Red
        Write-Host "  NOTE: Cannot delete (locked by LSA) - will be removed on reboot" -ForegroundColor Yellow
    } else {
        Write-Host "  [OK] Not found: $(Split-Path -Leaf $file)" -ForegroundColor Green
    }
}

# Remove desktop shortcut
Write-Host ""
Write-Host "Removing desktop shortcut..." -ForegroundColor Yellow
$shortcut = "$env:Public\Desktop\EID Authentication Configuration.lnk"
if (Test-Path $shortcut) {
    Remove-Item $shortcut -Force
    Write-Host "  [OK] Removed desktop shortcut" -ForegroundColor Green
} else {
    Write-Host "  [SKIP] Shortcut not found" -ForegroundColor Gray
}

Write-Host ""
Write-Host "=================================================" -ForegroundColor Cyan
Write-Host "Cleanup Complete!" -ForegroundColor Green
Write-Host ""
Write-Host "You can now run the installer again." -ForegroundColor Cyan
Write-Host "Note: DLLs in System32 (if any) will be removed on next reboot." -ForegroundColor Yellow
Write-Host ""
