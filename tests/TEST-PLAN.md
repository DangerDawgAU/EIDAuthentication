# EID Migration - End-to-End Test Plan

## Objective
Validate the complete EID credential migration workflow from source machine to destination VM.

## Test Scenarios

### Scenario 1: Single User Migration
- Create 1 test user with EID credential
- Export to .eid file
- Import on destination VM
- Verify login works

### Scenario 2: Multiple Users Migration
- Create 5 test users with EID credentials
- Export all to .eid file
- Import on destination VM
- Verify all users can log in

### Scenario 3: Selective Export
- Create 5 test users
- Export only selected users via "Export Selected" feature
- Import on destination VM
- Verify only selected users were migrated

### Scenario 4: Cross-Machine RID Mapping
- Create users on source machine (will have specific RIDs)
- Export credentials
- Import on destination machine (users will have different RIDs)
- Verify RID remapping works correctly

## Test Data

### Test Users
| Username | Password | Description |
|----------|----------|-------------|
| eid_test1 | Test@12345! | Basic test user |
| eid_test2 | Test@12345! | Second test user |
| eid_test3 | Test@12345! | Third test user |
| eid_admin1 | Test@12345! | Admin test user |
| eid_user1 | Test@12345! | Regular user |

### Expected Results
- All users should be able to log in with smart card on destination VM
- Group memberships should be preserved
- Certificate mappings should be correct

## Pre-Test Checklist

### Source Machine
- [ ] EID Authentication Package installed
- [ ] EIDMigrateUI.exe available (x64/Release)
- [ ] Test users created
- [ ] Test users have EID credentials enrolled
- [ ] Smart card drivers installed

### Destination VM
- [ ] Fresh/clean Windows installation
- [ ] EID Authentication Package installed
- [ ] EIDMigrateUI.exe available (x64/Release)
- [ ] Smart card drivers installed
- [ ] No existing EID credentials for test users

## Test Procedure

### Phase 1: Source Machine Setup

1. **Create Test Users**
   ```powershell
   .\Create-TestUsers.ps1
   ```

2. **Enroll EID Credentials**
   - Use EIDConfigurationWizard to enroll cards for each test user
   - Verify credentials are stored in LSA

3. **Export Credentials**
   ```powershell
   # Option A: Use GUI
   .\EIDMigrateUI.exe
   # Select "Export Credentials" -> "This Machine"

   # Option B: Use CLI
   .\EIDMigrate.exe export -local -output test_export.eid -password "TestExportPassword123!" -v
   ```

4. **Validate Export File**
   ```powershell
   .\EIDMigrate.exe validate -input test_export.eid -password "TestExportPassword123!" -v
   ```

### Phase 2: Transfer to Destination VM

1. Copy files to destination:
   - `test_export.eid` (or multiple files)
   - `EIDAuthenticationPackage.dll`
   - `EIDCredentialProvider.dll`
   - `EIDMigrateUI.exe`
   - `EIDPasswordChangeNotification.dll`

### Phase 3: Destination VM Import

1. **Install EID Components**
   ```powershell
   # Run as Administrator
   .\Install-EidComponents.ps1
   ```

2. **Preview Import File**
   ```powershell
   .\EIDMigrateUI.exe
   # Select "Import Credentials" -> Browse to file -> Decrypt
   # Verify metadata shows correctly
   ```

3. **Import Credentials**
   - Select "Import" option
   - Choose appropriate import options
   - Wait for completion

4. **Verify Import**
   - Check that users appear in "View Credentials"
   - Verify count matches export

### Phase 4: Validation

1. **Login Test**
   - Insert smart card for each test user
   - Attempt Windows login
   - Verify successful authentication

2. **Credential Verification**
   ```powershell
   .\EIDMigrate.exe list -local -v
   ```

3. **Certificate Verification**
   - Open certmgr.msc
   - Check Personal certificates for each user
   - Verify EID certificates are present

## Success Criteria

### Must Pass
- [ ] All test users can log in with smart card on destination VM
- [ ] No error messages during import/export
- [ ] Credential count matches between export and import
- [ ] Export date shows UTC time correctly

### Should Pass
- [ ] Group memberships preserved
- [ ] Certificate details viewable in GUI
- [ ] Refresh button works without crash
- [ ] Export selected creates correct subset

### Nice to Have
- [ ] Performance metrics (export/import time)
- [ ] File size observations
- [ ] Error handling for bad passwords

## Cleanup

### On Destination VM
```powershell
# Remove test users and credentials
.\Remove-TestUsers.ps1
```

### On Source Machine
```powershell
# Remove test users (optional)
.\Remove-TestUsers.ps1
```

## Troubleshooting

### Export Fails
- Check LSA service is running
- Verify EID Authentication Package is registered
- Check event logs for errors

### Import Fails
- Verify password is correct (16+ characters)
- Check file integrity
- Ensure destination machine has EID components installed

### Login Fails After Import
- Verify credential provider is registered
- Check smart card drivers
- Ensure certificate is in user's MY store
- Check LSA secrets are present

## Test Log Template

```
Date: ____________________
Tester: __________________
Source Machine: __________________
Destination VM: __________________

Test Results:
[ ] Scenario 1: Single User - PASS / FAIL
[ ] Scenario 2: Multiple Users - PASS / FAIL
[ ] Scenario 3: Selective Export - PASS / FAIL
[ ] Scenario 4: RID Remapping - PASS / FAIL

Issues Found:
____________________
____________________

Notes:
____________________
____________________
```
