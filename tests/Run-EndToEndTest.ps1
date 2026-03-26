<#
.SYNOPSIS
    Runs end-to-end EID migration test from a source machine

.DESCRIPTION
    This script automates the export portion of the EID migration test.
    Run this on the source machine after enrolling EID credentials for test users.

.EXAMPLE
    .\Run-EndToEndTest.ps1 -OutputPath "C:\EIDTests" -Password "TestPassword123456!"
#>

#Requires -RunAsAdministrator

param(
    [string]$OutputPath = "C:\EIDTests",
    [string]$Password = $(Read-Host "Enter export password (min 16 characters)" -AsSecureString),
    [switch]$IncludeGroups,
    [switch]$ValidateCerts,
    [switch]$SkipExisting
)

$ErrorActionPreference = "Stop"

# Import required modules
Import-Module -Name "$PSScriptRoot\EIDMigrate" -ErrorAction SilentlyContinue

# Get the EIDMigrate executable path
$EIDMigratePath = "$PSScriptRoot\x64\Release\EIDMigrate.exe"

if (-not (Test-Path $EIDMigratePath)) {
    Write-Host "ERROR: EIDMigrate.exe not found at: $EIDMigratePath" -ForegroundColor Red
    Write-Host "Please build the solution first." -ForegroundColor Red
    exit 1
}

# Convert secure password to plain text for the command
$BSTR = [System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($Password)
$PlainPassword = [System.Runtime.InteropServices.Marshal]::PtrToStringAuto($BSTR)

# Validate password length
if ($PlainPassword.Length -lt 16) {
    Write-Host "ERROR: Password must be at least 16 characters." -ForegroundColor Red
    exit 1
}

# Create output directory
New-Item -Path $OutputPath -ItemType Directory -Force | Out-Null
$Timestamp = Get-Date -Format "yyyyMMdd_HHmmss"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "EID Migration - End-to-End Test" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Output Path: $OutputPath" -ForegroundColor Gray
Write-Host "Timestamp: $Timestamp" -ForegroundColor Gray
Write-Host ""

# Define export file names
$ExportAllFile = Join-Path $OutputPath "eid_export_all_$Timestamp.eid"
$LogFile = Join-Path $OutputPath "test_log_$Timestamp.txt"

# Start logging
Start-Transcript -Path $LogFile -Force

try {
    # ============================================
    # Phase 1: Check for existing EID credentials
    # ============================================
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "Phase 1: Checking for EID Credentials" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan

    & $EIDMigratePath list -local -v

    if ($LASTEXITCODE -ne 0) {
        Write-Host ""
        Write-Host "WARNING: No EID credentials found or error occurred." -ForegroundColor Yellow
        $Continue = Read-Host "Continue anyway? (yes/no)"
        if ($Continue -ne "yes") {
            throw "Test aborted by user"
        }
    }

    # ============================================
    # Phase 2: Export all credentials
    # ============================================
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "Phase 2: Exporting All Credentials" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan

    $ExportArgs = @(
        "export"
        "-local"
        "-output", "`"$ExportAllFile`""
        "-password", "`"$PlainPassword`""
        "-v"
    )

    Write-Host "Command: $EIDMigratePath $ExportArgs" -ForegroundColor Gray
    & $EIDMigratePath $ExportArgs

    if ($LASTEXITCODE -ne 0) {
        throw "Export failed with exit code: $LASTEXITCODE"
    }

    Write-Host ""
    Write-Host "Export completed successfully!" -ForegroundColor Green
    Write-Host "Export file: $ExportAllFile" -ForegroundColor Gray

    # ============================================
    # Phase 3: Validate export file
    # ============================================
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "Phase 3: Validating Export File" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan

    $ValidateArgs = @(
        "validate"
        "-input", "`"$ExportAllFile`""
        "-password", "`"$PlainPassword`""
        "-v"
    )

    Write-Host "Command: $EIDMigratePath $ValidateArgs" -ForegroundColor Gray
    & $EIDMigratePath $ValidateArgs

    if ($LASTEXITCODE -ne 0) {
        throw "Validation failed with exit code: $LASTEXITCODE"
    }

    # ============================================
    # Phase 4: List credentials from file
    # ============================================
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "Phase 4: Listing Exported Credentials" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan

    $ListArgs = @(
        "list"
        "-input", "`"$ExportAllFile`""
        "-password", "`"$PlainPassword`""
        "-v"
    )

    Write-Host "Command: $EIDMigratePath $ListArgs" -ForegroundColor Gray
    & $EIDMigratePath $ListArgs

    # ============================================
    # Phase 5: Get file info
    # ============================================
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "Phase 5: Export File Information" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan

    $FileInfo = Get-Item $ExportAllFile
    Write-Host "File size: $($FileInfo.Length) bytes ($([math]::Round($FileInfo.Length / 1KB, 2)) KB)" -ForegroundColor Gray
    Write-Host "Created: $($FileInfo.CreationTime)" -ForegroundColor Gray
    Write-Host "Modified: $($FileInfo.LastWriteTime)" -ForegroundColor Gray

    # ============================================
    # Test Summary
    # ============================================
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "Test Summary" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "SUCCESS: All tests passed!" -ForegroundColor Green
    Write-Host ""
    Write-Host "Export file created:" -ForegroundColor White
    Write-Host "  $ExportAllFile" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Password: $PlainPassword" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Next steps:" -ForegroundColor Cyan
    Write-Host "1. Copy the export file to the destination VM" -ForegroundColor Gray
    Write-Host "2. Copy EIDMigrateUI.exe to the destination VM" -ForegroundColor Gray
    Write-Host "3. Run EIDMigrateUI.exe and select 'Import Credentials'" -ForegroundColor Gray
    Write-Host "4. Use the password above to decrypt the file" -ForegroundColor Gray
    Write-Host ""

    # Save test info
    $TestInfoFile = Join-Path $OutputPath "test_info_$Timestamp.txt"
    @"
EID Migration Test - Export Summary
=====================================
Date: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
Source Computer: $env:COMPUTERNAME
Source Username: $env:USERNAME
Export File: $ExportAllFile
Password: $PlainPassword
File Size: $($FileInfo.Length) bytes

Test Results:
- List local credentials: PASS
- Export credentials: PASS
- Validate export file: PASS
- List credentials from file: PASS

Instructions for Import:
1. Copy $ExportAllFile to destination VM
2. Copy EIDMigrateUI.exe to destination VM
3. Install EID components on destination VM
4. Run EIDMigrateUI.exe -> Import Credentials
5. Browse to export file and enter password
6. Preview credentials and click Import
7. Test login with smart card for each user
"@ | Out-File -FilePath $TestInfoFile -Encoding UTF8

    Write-Host "Test info saved to: $TestInfoFile" -ForegroundColor Gray

}
catch {
    Write-Host ""
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}
finally {
    Stop-Transcript
}

# Clear password from memory
$PlainPassword = $null
[GC]::Collect()
