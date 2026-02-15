# Configuration Wizard and Password Change Testing Checklist

**Phase:** 06-verification
**Plan:** 06-04
**Purpose:** Test Configuration Wizard and Password Change Notification functionality
**Date Created:** 2026-02-15

---

## Test Prerequisites

Before testing, ensure the following are completed:

- [ ] LSA package installed and tested (from 06-02)
- [ ] Credential Provider installed (from 06-03)
- [ ] Test user account exists on system
- [ ] Smart card reader connected
- [ ] Test smart card available (enrolled or ready for enrollment)
- [ ] Build artifacts available from x64\Release\ directory (verified in 06-01)

---

## Configuration Wizard Installation

### Step 1: Create Installation Directory

```cmd
mkdir "C:\Program Files\EID Authentication"
```

### Step 2: Copy Files

Copy the following files from `x64\Release\`:

- [ ] EIDConfigurationWizard.exe
- [ ] EIDConfigurationWizardElevated.exe

**Command:**
```cmd
copy x64\Release\EIDConfigurationWizard.exe "C:\Program Files\EID Authentication\"
copy x64\Release\EIDConfigurationWizardElevated.exe "C:\Program Files\EID Authentication\"
```

---

## Configuration Wizard Testing Steps

### Step 1: Launch

1. Navigate to `C:\Program Files\EID Authentication\`
2. Double-click `EIDConfigurationWizard.exe`
3. **Document:** Does wizard launch without crash? (Yes/No)
4. **Document:** Any error dialogs appear? (Yes/No - describe)

### Step 2: LSA Package Detection

1. Observe wizard startup screen
2. **Document:** Does wizard detect LSA package? (Yes/No)
3. Look for "Package Not Available" message
4. **Document:** Status message displayed: _____________

### Step 3: Smart Card Detection

1. Insert smart card into reader
2. Wait 2-3 seconds for detection
3. **Document:** Does wizard detect card? (Yes/No)
4. **Document:** Certificate information displays correctly? (Yes/No)
5. **Document:** Certificate details shown: _____________

### Step 4: Enrollment UI

1. Navigate to enrollment screen
2. **Document:** UI renders correctly? (Yes/No - note any rendering issues)
3. Check for C++23 std::format related rendering issues:
   - [ ] Text formatting appears correct
   - [ ] No garbled text
   - [ ] No missing text
   - [ ] Layout is proper
4. **Document:** Any issues observed: _____________

### Step 5: Elevation Helper

1. Trigger admin operation (if available in UI)
2. **Document:** UAC prompt appears? (Yes/No)
3. **Document:** EIDConfigurationWizardElevated.exe runs? (Yes/No)
4. **Document:** Elevation result: _____________

---

## Password Change Notification Testing Steps

### Step 1: Installation Verification

1. Verify DLL in System32:

```cmd
dir C:\Windows\System32\EIDPasswordChangeNotification.dll
```

**Document:** DLL exists? (Yes/No)

2. Verify registry entry:

```cmd
reg query "HKLM\SYSTEM\CurrentControlSet\Control\Lsa" /v "Notification Packages"
```

**Document:** "EIDPasswordChangeNotification" in list? (Yes/No)
**Document:** Full registry output: _____________

### Step 2: Password Change Test

1. Log in as test user (with password-based account)
2. Press Ctrl+Alt+Del
3. Select "Change a password"
4. Enter old password and new password
5. **Document:** Password change succeeds? (Yes/No)
6. **Document:** Any error messages? (describe if yes)
7. **Document:** Error details: _____________

### Step 3: Event Viewer Verification

1. Open Event Viewer (eventvwr.msc)
2. Navigate to Windows Logs -> System
3. Filter for "password" or filter by Error events
4. **Document:** Any errors related to EIDPasswordChangeNotification? (Yes/No - list if yes)
5. **Document:** Error events found: _____________

---

## Test Results Template

### Windows 7

**Tester:** _____________
**Build Number:** _____________
**Test Date:** _____________

#### Configuration Wizard

| Test Item | Result | Notes |
|-----------|--------|-------|
| Files copied successfully | [ ] Yes [ ] No | |
| Wizard launches without crash | [ ] Yes [ ] No | |
| LSA package detected | [ ] Yes [ ] No | |
| Smart card detection working | [ ] Yes [ ] No | |
| UI rendering correct (C++23) | [ ] Yes [ ] No | |
| Elevation helper working | [ ] Yes [ ] No | |
| **Overall wizard result** | [ ] PASS [ ] FAIL | |

#### Password Change Notification

| Test Item | Result | Notes |
|-----------|--------|-------|
| DLL exists in System32 | [ ] Yes [ ] No | |
| Registry entry exists | [ ] Yes [ ] No | |
| Password change succeeds | [ ] Yes [ ] No | |
| Event Viewer errors | [ ] None [ ] Yes (list below) | |

**Event Viewer Errors:**
```
[Paste any relevant errors here]
```

---

### Windows 10

**Tester:** _____________
**Build Number:** _____________
**Test Date:** _____________

#### Configuration Wizard

| Test Item | Result | Notes |
|-----------|--------|-------|
| Files copied successfully | [ ] Yes [ ] No | |
| Wizard launches without crash | [ ] Yes [ ] No | |
| LSA package detected | [ ] Yes [ ] No | |
| Smart card detection working | [ ] Yes [ ] No | |
| UI rendering correct (C++23) | [ ] Yes [ ] No | |
| Elevation helper working | [ ] Yes [ ] No | |
| **Overall wizard result** | [ ] PASS [ ] FAIL | |

#### Password Change Notification

| Test Item | Result | Notes |
|-----------|--------|-------|
| DLL exists in System32 | [ ] Yes [ ] No | |
| Registry entry exists | [ ] Yes [ ] No | |
| Password change succeeds | [ ] Yes [ ] No | |
| Event Viewer errors | [ ] None [ ] Yes (list below) | |

**Event Viewer Errors:**
```
[Paste any relevant errors here]
```

---

### Windows 11

**Tester:** _____________
**Build Number:** _____________
**Test Date:** _____________

#### Configuration Wizard

| Test Item | Result | Notes |
|-----------|--------|-------|
| Files copied successfully | [ ] Yes [ ] No | |
| Wizard launches without crash | [ ] Yes [ ] No | |
| LSA package detected | [ ] Yes [ ] No | |
| Smart card detection working | [ ] Yes [ ] No | |
| UI rendering correct (C++23) | [ ] Yes [ ] No | |
| Elevation helper working | [ ] Yes [ ] No | |
| **Overall wizard result** | [ ] PASS [ ] FAIL | |

#### Password Change Notification

| Test Item | Result | Notes |
|-----------|--------|-------|
| DLL exists in System32 | [ ] Yes [ ] No | |
| Registry entry exists | [ ] Yes [ ] No | |
| Password change succeeds | [ ] Yes [ ] No | |
| Event Viewer errors | [ ] None [ ] Yes (list below) | |

**Event Viewer Errors:**
```
[Paste any relevant errors here]
```

---

## Summary Format

After completing testing on all platforms, use this format to report results:

```
Windows 7: Config Wizard [PASS/FAIL] - Password Change [PASS/FAIL] - [notes]
Windows 10: Config Wizard [PASS/FAIL] - Password Change [PASS/FAIL] - [notes]
Windows 11: Config Wizard [PASS/FAIL] - Password Change [PASS/FAIL] - [notes]
```

---

## Critical Success Criteria

- Configuration Wizard MUST launch without crashes
- LSA package detection MUST work
- Password Change Notification MUST load (registry entry exists)
- Password changes MUST succeed
- No C++23 std::format related rendering issues

## Acceptable Issues

- Minor UI issues that don't prevent functionality
- Smart card enrollment not tested (if no enrolled card available)

## Potential Issues to Watch For

- std::format rendering problems (C++23 specific)
- C++ runtime library issues (should have been caught in 06-01)
- LSA communication failures (IPC issues)
- Password filter blocking password changes

---

## Dependency Information

This testing depends on:
- **06-01:** Build artifacts must exist in x64\Release\
- **06-02:** LSA package must be loaded (Configuration Wizard needs it)
- **06-03:** Credential Provider must be registered (full authentication flow)
