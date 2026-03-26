<#
.SYNOPSIS
    Removes test users created for EID migration testing

.DESCRIPTION
    Removes local user accounts and their associated EID credentials.
    Use with caution - this will permanently delete users and their data.

.EXAMPLE
    .\Remove-TestUsers.ps1

.EXAMPLE
    .\Remove-TestUsers.ps1 -WhatIf
#>

#Requires -RunAsAdministrator

param(
    [switch]$WhatIf,
    [switch]$Force
)

$ErrorActionPreference = "Stop"

# Test user configuration (must match Create-TestUsers.ps1)
$TestUsers = @(
    "eid_test1",
    "eid_test2",
    "eid_test3",
    "eid_admin1",
    "eid_user1"
)

if (-not $Force) {
    Write-Host "WARNING: This will delete the following test users and all their data:" -ForegroundColor Red
    $TestUsers | ForEach-Object { Write-Host "  - $_" -ForegroundColor Yellow }
    Write-Host ""
    $Response = Read-Host "Continue? (yes/no)"
    if ($Response -ne "yes") {
        Write-Host "Cancelled." -ForegroundColor Yellow
        exit 0
    }
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "EID Migration - Test User Removal" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$RemovedUsers = @()
$NotFoundUsers = @()
$FailedUsers = @()

foreach ($UserName in $TestUsers) {
    Write-Host "Processing user: $UserName" -ForegroundColor Yellow

    try {
        # Check if user exists
        $ExistingUser = Get-LocalUser -Name $UserName -ErrorAction SilentlyContinue

        if (-not $ExistingUser) {
            Write-Host "  [SKIP] User not found" -ForegroundColor DarkYellow
            $NotFoundUsers += $UserName
        }
        else {
            if ($WhatIf) {
                Write-Host "  [WHATIF] Would remove user: $UserName" -ForegroundColor Cyan
                $RemovedUsers += $UserName
            }
            else {
                # Remove user (this will also remove their LSA secrets)
                Remove-LocalUser -Name $UserName -ErrorAction Stop
                Write-Host "  [OK] User removed" -ForegroundColor Green
                $RemovedUsers += $UserName
            }
        }
    }
    catch {
        Write-Host "  [ERROR] $($_.Exception.Message)" -ForegroundColor Red
        $FailedUsers += $UserName
    }
    Write-Host ""
}

# Summary
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Summary" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Removed: $($RemovedUsers.Count) users" -ForegroundColor Green
Write-Host "Not found: $($NotFoundUsers.Count) users" -ForegroundColor Yellow
Write-Host "Failed: $($FailedUsers.Count) users" -ForegroundColor Red

if ($RemovedUsers.Count -gt 0) {
    Write-Host ""
    Write-Host "Removed users:" -ForegroundColor Green
    $RemovedUsers | ForEach-Object { Write-Host "  - $_" -ForegroundColor Gray }
}

if ($NotFoundUsers.Count -gt 0) {
    Write-Host ""
    Write-Host "Not found users:" -ForegroundColor Yellow
    $NotFoundUsers | ForEach-Object { Write-Host "  - $_" -ForegroundColor Gray }
}

if ($FailedUsers.Count -gt 0) {
    Write-Host ""
    Write-Host "Failed users:" -ForegroundColor Red
    $FailedUsers | ForEach-Object { Write-Host "  - $_" -ForegroundColor Gray }
}

Write-Host ""

if (-not $WhatIf -and $RemovedUsers.Count -gt 0) {
    Write-Host "Note: EID credentials in LSA have been removed along with the users." -ForegroundColor Cyan
    Write-Host "      You may want to restart for the changes to take full effect." -ForegroundColor Cyan
    Write-Host ""
}
