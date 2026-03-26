# EID Migration Testing - Quick Reference Card

## Source Machine

### Step 1: Create Test Users
```powershell
# Run as Administrator
cd C:\Users\user\Documents\EIDAuthentication\tests
.\Create-TestUsers.ps1
```
Creates: eid_test1, eid_test2, eid_test3, eid_admin1, eid_user1
Default password: `Test@12345!`

### Step 2: Enroll EID Credentials
1. Open EIDConfigurationWizard.exe
2. For each test user:
   - Select "Enroll Smart Card"
   - Insert smart card
   - Enter user details
   - Complete enrollment

### Step 3: Export Credentials
```powershell
# Option A: Automated test
.\Run-EndToEndTest.ps1 -OutputPath "C:\EIDTests" -Password "TestPassword123456!"

# Option B: Manual export
cd ..\x64\Release
.\EIDMigrate.exe export -local -output "C:\EIDTests\test_export.eid" -password "TestPassword123456!" -v
```

### Step 4: Validate Export
```powershell
.\EIDMigrate.exe validate -input "C:\EIDTests\test_export.eid" -password "TestPassword123456!" -v
```

## Transfer Files

Copy these files to destination VM:
- `C:\EIDTests\test_export.eid` (the export file)
- `x64\Release\EIDMigrateUI.exe` (GUI wizard)
- `Installer\EIDInstallx64.exe` (installer)

## Destination VM

### Step 1: Install EID Components
```powershell
# Run as Administrator
.\EIDInstallx64.exe
```
Or manually:
1. Register EIDAuthenticationPackage.dll
2. Register EIDCredentialProvider.dll
3. Reboot

### Step 2: Import Credentials
```powershell
# Run as Administrator
.\EIDMigrateUI.exe
```

**Wizard Steps:**
1. Click "Import Credentials"
2. Click "Browse" → Select export file
3. Enter password: `TestPassword123456!`
4. Click "Decrypt"
5. Review metadata (source machine, export date, credential count)
6. Click "Next" to proceed
7. Review import options
8. Click "Next" to preview
9. Click "Import" to begin
10. Wait for completion

### Step 3: Verify Import
```powershell
# List imported credentials
.\EIDMigrate.exe list -local -v
```

### Step 4: Test Login
For each test user:
1. Insert their smart card
2. Press Ctrl+Alt+Del
3. Select "Smart Card" option
4. Enter PIN when prompted
5. Verify successful login

## Troubleshooting

### Export Shows 0 Credentials
- Verify EID credentials are enrolled
- Check LSA service is running
- Run as Administrator

### Import Fails
- Verify password is correct (16+ characters)
- Check file integrity (MD5 checksum match)
- Ensure EID components are installed

### Login Fails After Import
- Restart the destination VM
- Verify smart card drivers are installed
- Check certificate is in user's MY store
- Verify LSA secrets are created

## Test Checklist

- [ ] Test users created
- [ ] EID credentials enrolled for all users
- [ ] Export completed without errors
- [ ] Export file validated
- [ ] Files transferred to destination VM
- [ ] EID components installed on destination
- [ ] Import completed without errors
- [ ] All users can log in with smart card
- [ ] Group memberships preserved
- [ ] Export date shows UTC time correctly

## Cleanup

### On Destination VM
```powershell
.\Remove-TestUsers.ps1
```

### On Source VM
```powershell
.\Remove-TestUsers.ps1
Remove-Item -Path "C:\EIDTests" -Recurse -Force
```
