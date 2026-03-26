# EID Migration Tests

This folder contains test scripts and documentation for end-to-end testing of the EID credential migration tool.

## Quick Start

```powershell
# Navigate to tests folder
cd C:\Users\user\Documents\EIDAuthentication\tests

# Create test users
.\Create-TestUsers.ps1

# Run automated export test
.\Run-EndToEndTest.ps1 -OutputPath "C:\EIDTests" -Password "TestPassword123456!"

# Cleanup when done
.\Remove-TestUsers.ps1
```

## Files

| File | Description |
|------|-------------|
| `Create-TestUsers.ps1` | Creates 5 test users (eid_test1-3, eid_admin1, eid_user1) |
| `Remove-TestUsers.ps1` | Removes test users and their credentials |
| `Run-EndToEndTest.ps1` | Automated export + validation script |
| `TEST-PLAN.md` | Comprehensive test plan with scenarios |
| `TEST-QUICK-REFERENCE.md` | Quick reference card for testing |

## Test Workflow

### Source Machine
1. **Create test users** - `.\Create-TestUsers.ps1`
2. **Enroll EID credentials** - Use EIDConfigurationWizard for each user
3. **Export credentials** - `.\Run-EndToEndTest.ps1`
4. **Validate export** - Script automatically validates the export file

### Transfer Files
Copy to destination VM:
- Export file (e.g., `C:\EIDTests\eid_export_all_*.eid`)
- EIDMigrateUI.exe
- EID installer

### Destination VM
1. **Install EID components** - Run the installer
2. **Import credentials** - Use EIDMigrateUI.exe
3. **Test login** - Verify each user can log in with smart card
4. **Cleanup** - `.\Remove-TestUsers.ps1`

## Test Users

| Username | Password | Description |
|----------|----------|-------------|
| eid_test1 | Test@12345! | Basic test user |
| eid_test2 | Test@12345! | Second test user |
| eid_test3 | Test@12345! | Third test user |
| eid_admin1 | Test@12345! | Admin test user |
| eid_user1 | Test@12345! | Regular user |

## Notes

- All scripts require Administrator privileges
- Export passwords must be at least 16 characters
- Test users have a standard password for local login fallback
- Smart card enrollment must be done separately for each user

## Cleanup

To remove all test users and data:

```powershell
# On both source and destination machines
.\Remove-TestUsers.ps1

# Remove test output
Remove-Item -Path "C:\EIDTests" -Recurse -Force
```
