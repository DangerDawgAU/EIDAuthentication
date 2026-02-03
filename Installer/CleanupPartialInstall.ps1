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

$removed = 0
$skipped = 0

function Remove-RegistryKey($Path) {
    if (Test-Path $Path) {
        Remove-Item -Path $Path -Recurse -Force -ErrorAction SilentlyContinue
        Write-Host "  [OK] Removed $Path" -ForegroundColor Green
        $script:removed++
    } else {
        Write-Host "  [SKIP] Not found: $Path" -ForegroundColor Gray
        $script:skipped++
    }
}

function Remove-RegistryValue($Path, $Name) {
    if (Test-Path $Path) {
        $val = Get-ItemProperty -Path $Path -Name $Name -ErrorAction SilentlyContinue
        if ($null -ne $val.$Name) {
            Remove-ItemProperty -Path $Path -Name $Name -Force -ErrorAction SilentlyContinue
            Write-Host "  [OK] Removed $Path\$Name" -ForegroundColor Green
            $script:removed++
        } else {
            Write-Host "  [SKIP] Value not found: $Path\$Name" -ForegroundColor Gray
            $script:skipped++
        }
    } else {
        Write-Host "  [SKIP] Key not found: $Path" -ForegroundColor Gray
        $script:skipped++
    }
}

# --- Registry Cleanup ---
Write-Host "Removing registry entries..." -ForegroundColor Yellow

# Installation path
Remove-RegistryKey "HKLM:\Software\EIDAuthentication"

# Uninstall entry
Remove-RegistryKey "HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\EIDAuthentication"

# Credential Provider
Remove-RegistryKey "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\Credential Providers\{B4866A0A-DB08-4835-A26F-414B46F3244C}"
Remove-RegistryKey "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\Credential Provider Filters\{B4866A0A-DB08-4835-A26F-414B46F3244C}"

# CLSID entries (Credential Provider and Configuration Wizard)
Remove-RegistryKey "Registry::HKEY_CLASSES_ROOT\CLSID\{B4866A0A-DB08-4835-A26F-414B46F3244C}"
Remove-RegistryKey "Registry::HKEY_CLASSES_ROOT\CLSID\{F5D846B4-14B0-11DE-B23C-27A355D89593}"

# Configuration Wizard Control Panel namespace
Remove-RegistryKey "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\ControlPanel\NameSpace\{F5D846B4-14B0-11DE-B23C-27A355D89593}"

# WMI Autologger for ETW tracing
Remove-RegistryKey "HKLM:\SYSTEM\CurrentControlSet\Control\WMI\Autologger\EIDCredentialProvider"

# Crash dump configuration for lsass.exe
Remove-RegistryKey "HKLM:\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\lsass.exe"

# GPO policy values set by the Configuration Wizard
Remove-RegistryKey "HKLM:\SOFTWARE\Policies\Microsoft\Windows\SmartCardCredentialProvider"
Remove-RegistryValue "HKLM:\Software\Microsoft\Windows NT\CurrentVersion\Winlogon" "scremoveoption"
Remove-RegistryValue "HKLM:\Software\Microsoft\Windows\CurrentVersion\Policies\System" "scforceoption"

# --- LSA multi-string value cleanup ---
Write-Host ""
Write-Host "Removing LSA registration entries..." -ForegroundColor Yellow

function Remove-MultiStringEntry($Path, $ValueName, $EntryToRemove) {
    try {
        $current = (Get-ItemProperty -Path $Path -Name $ValueName -ErrorAction Stop).$ValueName
        if ($current -contains $EntryToRemove) {
            $updated = $current | Where-Object { $_ -ne $EntryToRemove }
            Set-ItemProperty -Path $Path -Name $ValueName -Value $updated
            Write-Host "  [OK] Removed '$EntryToRemove' from $ValueName" -ForegroundColor Green
            $script:removed++
        } else {
            Write-Host "  [SKIP] '$EntryToRemove' not found in $ValueName" -ForegroundColor Gray
            $script:skipped++
        }
    } catch {
        Write-Host "  [ERROR] Failed to update $ValueName : $_" -ForegroundColor Red
    }
}

$lsaPath = "HKLM:\SYSTEM\CurrentControlSet\Control\Lsa"
Remove-MultiStringEntry $lsaPath "Security Packages" "EIDAuthenticationPackage"
Remove-MultiStringEntry $lsaPath "Notification Packages" "EIDPasswordChangeNotification"

# --- Service cleanup ---
Write-Host ""
Write-Host "Resetting Smart Card Removal Policy service..." -ForegroundColor Yellow
try {
    & sc.exe config ScPolicySvc start= demand 2>&1 | Out-Null
    & sc.exe stop ScPolicySvc 2>&1 | Out-Null
    Write-Host "  [OK] ScPolicySvc reset to demand-start" -ForegroundColor Green
} catch {
    Write-Host "  [SKIP] Could not reset ScPolicySvc: $_" -ForegroundColor Gray
}

# --- File cleanup ---
Write-Host ""
Write-Host "Checking System32 for leftover files..." -ForegroundColor Yellow

$system32Files = @(
    "$env:SystemRoot\System32\EIDAuthenticationPackage.dll",
    "$env:SystemRoot\System32\EIDCredentialProvider.dll",
    "$env:SystemRoot\System32\EIDPasswordChangeNotification.dll",
    "$env:SystemRoot\System32\EIDConfigurationWizard.exe",
    "$env:SystemRoot\System32\LogFiles\WMI\EIDCredentialProvider.etl"
)

foreach ($file in $system32Files) {
    if (Test-Path $file) {
        try {
            Remove-Item $file -Force -ErrorAction Stop
            Write-Host "  [OK] Deleted: $(Split-Path -Leaf $file)" -ForegroundColor Green
            $removed++
        } catch {
            Write-Host "  [LOCKED] $file - will be removed on reboot" -ForegroundColor Yellow
        }
    } else {
        Write-Host "  [OK] Not found: $(Split-Path -Leaf $file)" -ForegroundColor Green
    }
}

# Remove desktop shortcut (check both public and user desktop)
Write-Host ""
Write-Host "Removing shortcuts..." -ForegroundColor Yellow

$shortcuts = @(
    "$env:Public\Desktop\EID Authentication Configuration.lnk",
    "$env:USERPROFILE\Desktop\EID Authentication Configuration.lnk",
    "$env:ProgramData\Microsoft\Windows\Start Menu\Programs\EID Authentication"
)

foreach ($path in $shortcuts) {
    if (Test-Path $path) {
        Remove-Item $path -Recurse -Force -ErrorAction SilentlyContinue
        Write-Host "  [OK] Removed: $path" -ForegroundColor Green
        $removed++
    } else {
        Write-Host "  [SKIP] Not found: $(Split-Path -Leaf $path)" -ForegroundColor Gray
    }
}

# Remove Program Files installation directory
Write-Host ""
Write-Host "Checking Program Files installation..." -ForegroundColor Yellow
$installDir = "$env:ProgramFiles\EID Authentication"
if (Test-Path $installDir) {
    Remove-Item $installDir -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host "  [OK] Removed: $installDir" -ForegroundColor Green
    $removed++
} else {
    Write-Host "  [SKIP] Not found: $installDir" -ForegroundColor Gray
}

Write-Host ""
Write-Host "=================================================" -ForegroundColor Cyan
Write-Host "Cleanup Complete!" -ForegroundColor Green
Write-Host "Items removed: $removed" -ForegroundColor White
Write-Host "Items skipped: $skipped" -ForegroundColor Gray
Write-Host ""
Write-Host "Note: DLLs locked by LSA (if any) will be removed on next reboot." -ForegroundColor Yellow
Write-Host "A reboot is recommended to complete the cleanup." -ForegroundColor Yellow
Write-Host ""
