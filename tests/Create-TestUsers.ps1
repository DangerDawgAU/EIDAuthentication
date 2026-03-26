<#
.SYNOPSIS
    Creates test users for EID migration testing

.DESCRIPTION
    Creates local user accounts for testing EID credential migration.
    Users are created with a standard password and added to the Users group.

.EXAMPLE
    .\Create-TestUsers.ps1
#>

#Requires -RunAsAdministrator

$ErrorActionPreference = "Stop"

# Test user configuration
$TestUsers = @(
    { Name = "eid_test1"; FullName = "EID Test User 1"; Description = "Test user for EID migration" },
    { Name = "eid_test2"; FullName = "EID Test User 2"; Description = "Test user for EID migration" },
    { Name = "eid_test3"; FullName = "EID Test User 3"; Description = "Test user for EID migration" },
    { Name = "eid_admin1"; FullName = "EID Admin Test"; Description = "Admin test user for EID migration" },
    { Name = "eid_user1"; FullName = "EID Regular User"; Description = "Regular test user for EID migration" }
)

$DefaultPassword = "Test@12345!"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "EID Migration - Test User Creation" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$CreatedUsers = @()
$ExistingUsers = @()
$FailedUsers = @()

foreach ($User in $TestUsers) {
    Write-Host "Processing user: $($User.Name)" -ForegroundColor Yellow

    try {
        # Check if user already exists
        $ExistingUser = Get-LocalUser -Name $User.Name -ErrorAction SilentlyContinue

        if ($ExistingUser) {
            Write-Host "  [SKIP] User already exists" -ForegroundColor DarkYellow
            $ExistingUsers += $User.Name
        }
        else {
            # Create the user
            $NewUser = New-LocalUser `
                -Name $User.Name `
                -Password (ConvertTo-SecureString $DefaultPassword -AsPlainText -Force) `
                -FullName $User.FullName `
                -Description $User.Description `
                -PasswordNeverRequired $false `
                -AccountNeverExpires

            if ($NewUser) {
                Write-Host "  [OK] User created successfully" -ForegroundColor Green

                # Add to Users group
                $UsersGroup = Get-LocalGroup -SID "S-1-5-32-545"
                Add-LocalGroupMember -SID $UsersGroup.SID -Member $NewUser -ErrorAction SilentlyContinue
                Write-Host "  [OK] Added to Users group" -ForegroundColor Green

                $CreatedUsers += $User.Name
            }
        }
    }
    catch {
        Write-Host "  [ERROR] $($_.Exception.Message)" -ForegroundColor Red
        $FailedUsers += $User.Name
    }
    Write-Host ""
}

# Summary
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Summary" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Created: $($CreatedUsers.Count) users" -ForegroundColor Green
Write-Host "Already existed: $($ExistingUsers.Count) users" -ForegroundColor Yellow
Write-Host "Failed: $($FailedUsers.Count) users" -ForegroundColor Red

if ($CreatedUsers.Count -gt 0) {
    Write-Host ""
    Write-Host "Created users:" -ForegroundColor Green
    $CreatedUsers | ForEach-Object { Write-Host "  - $_" -ForegroundColor Gray }
}

if ($ExistingUsers.Count -gt 0) {
    Write-Host ""
    Write-Host "Existing users:" -ForegroundColor Yellow
    $ExistingUsers | ForEach-Object { Write-Host "  - $_" -ForegroundColor Gray }
}

if ($FailedUsers.Count -gt 0) {
    Write-Host ""
    Write-Host "Failed users:" -ForegroundColor Red
    $FailedUsers | ForEach-Object { Write-Host "  - $_" -ForegroundColor Gray }
}

Write-Host ""
Write-Host "Default password for all users: $DefaultPassword" -ForegroundColor Magenta
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "1. Enroll EID smart cards for each user using EIDConfigurationWizard" -ForegroundColor Gray
Write-Host "2. Export credentials using: .\EIDMigrate.exe export -local -output test.eid -password `"YourPassword123!`" -v" -ForegroundColor Gray
Write-Host ""
