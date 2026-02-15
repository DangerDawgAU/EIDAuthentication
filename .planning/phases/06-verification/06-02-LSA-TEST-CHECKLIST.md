# LSA Authentication Package Testing Checklist

**Plan:** 06-02
**Purpose:** Verify EIDAuthenticationPackage.dll loads and initializes correctly in LSASS on Windows 7, 10, and 11.
**Created:** 2026-02-15

---

## Test Prerequisites

Before starting LSA package testing, verify the following prerequisites are met:

- [ ] **Windows version verified** - Run `winver` command to confirm OS version
- [ ] **Administrator access confirmed** - Must have admin rights for DLL installation
- [ ] **Test Signing mode enabled** - Run `bcdedit /set testsigning on` and reboot
- [ ] **LSA Protection disabled** - Run `reg add "HKLM\SYSTEM\CurrentControlSet\Control\Lsa" /v RunAsPPL /t REG_DWORD /d 0 /f` and reboot
- [ ] **Build artifacts available** - EIDAuthenticationPackage.dll from 06-01 build verification (x64\Release\)
- [ ] **Process Explorer available** - Download from Microsoft Sysinternals for DLL verification

---

## Installation Steps

### Step 1: Copy DLL to System32
```
copy x64\Release\EIDAuthenticationPackage.dll C:\Windows\System32\
```

### Step 2: Register LSA Package
Add the package to the Authentication Packages registry key:

```
reg add "HKLM\SYSTEM\CurrentControlSet\Control\Lsa\Authentication Packages" /v "EIDAuthenticationPackage" /t REG_MULTI_SZ /d "EIDAuthenticationPackage" /f
```

**Note:** If using the installer (if available), run the installer instead of manual registration.

### Step 3: Reboot System
**CRITICAL:** LSA packages only load at system boot. Reboot is required.

```
shutdown /r /t 10
```

### Step 4: Verify Registration
After reboot, check the registry:

```
reg query "HKLM\SYSTEM\CurrentControlSet\Control\Lsa\Authentication Packages"
```

---

## Verification Steps

### Step 1: Check Event Viewer for LSA Events
1. Open Event Viewer: `eventvwr.msc`
2. Navigate to **Windows Logs** > **System**
3. Click **Filter Current Log**
4. Search for events containing "LSA"
5. Look for:
   - Successful package load events
   - Any error events related to LSA or authentication

### Step 2: Verify DLL Loaded in LSASS
1. Run **Process Explorer** as Administrator
2. Find the `lsass.exe` process
3. Double-click to view process properties
4. Go to the **Modules** tab
5. Search for `EIDAuthenticationPackage.dll`
6. Confirm it appears in the loaded modules list

### Step 3: Check for LSA Initialization Errors
1. In Event Viewer > Windows Logs > System
2. Look for Error events with source:
   - "LSA"
   - "Service Control Manager"
   - "Application Error"
3. Note any error codes or messages related to:
   - DLL load failures
   - Missing dependencies
   - Access denied errors

---

## Per-Version Testing

### Windows 7 Testing

| Step | Verification | Status |
|------|--------------|--------|
| Prerequisites complete | Test Signing + LSA Protection disabled | [ ] |
| DLL copied to System32 | File exists at C:\Windows\System32\EIDAuthenticationPackage.dll | [ ] |
| Registry registration | EIDAuthenticationPackage in Authentication Packages key | [ ] |
| System rebooted | Clean reboot, no errors | [ ] |
| DLL in lsass.exe | Process Explorer shows EIDAuthenticationPackage.dll loaded | [ ] |
| No LSA errors in Event Viewer | System log shows no LSA-related errors | [ ] |
| **Overall Result** | **PASS / FAIL** | [ ] |

**Windows 7 Build Number:** _______________
**Notes:** _______________________________________

---

### Windows 10 Testing

| Step | Verification | Status |
|------|--------------|--------|
| Prerequisites complete | Test Signing + LSA Protection disabled | [ ] |
| DLL copied to System32 | File exists at C:\Windows\System32\EIDAuthenticationPackage.dll | [ ] |
| Registry registration | EIDAuthenticationPackage in Authentication Packages key | [ ] |
| System rebooted | Clean reboot, no errors | [ ] |
| DLL in lsass.exe | Process Explorer shows EIDAuthenticationPackage.dll loaded | [ ] |
| No LSA errors in Event Viewer | System log shows no LSA-related errors | [ ] |
| **Overall Result** | **PASS / FAIL** | [ ] |

**Windows 10 Build Number:** _______________
**Notes:** _______________________________________

---

### Windows 11 Testing

| Step | Verification | Status |
|------|--------------|--------|
| Prerequisites complete | Test Signing + LSA Protection disabled | [ ] |
| DLL copied to System32 | File exists at C:\Windows\System32\EIDAuthenticationPackage.dll | [ ] |
| Registry registration | EIDAuthenticationPackage in Authentication Packages key | [ ] |
| System rebooted | Clean reboot, no errors | [ ] |
| DLL in lsass.exe | Process Explorer shows EIDAuthenticationPackage.dll loaded | [ ] |
| No LSA errors in Event Viewer | System log shows no LSA-related errors | [ ] |
| **Overall Result** | **PASS / FAIL** | [ ] |

**Windows 11 Build Number:** _______________
**Notes:** _______________________________________

---

## Expected Results

| Criteria | Expected |
|----------|----------|
| DLL appears in System32 | Yes |
| Registry key contains EIDAuthenticationPackage | Yes |
| System reboots successfully | Yes |
| EIDAuthenticationPackage.dll in lsass.exe modules | Yes |
| LSA load errors in Event Viewer | None |

---

## Failure Investigation

If testing fails on any Windows version, document the following:

### Error Codes
- [ ] Event Viewer error code: _______________
- [ ] Error source: _______________
- [ ] Error message: _______________

### DLL Load Status
- [ ] DLL present in System32: Yes / No
- [ ] Dependencies check (dumpbin /dependents): _______________

### Registry State
- [ ] Authentication Packages key contents: _______________

### Potential Causes
- [ ] Static CRT linkage issue
- [ ] Missing runtime dependencies
- [ ] Windows API incompatibility
- [ ] LSA Protection still enabled
- [ ] Test Signing not enabled
- [ ] Architecture mismatch (x86 vs x64)

---

## Test Summary

| Windows Version | Result | Notes |
|-----------------|--------|-------|
| Windows 7 | PASS / FAIL | |
| Windows 10 | PASS / FAIL | |
| Windows 11 | PASS / FAIL | |

**Tester:** _______________
**Date:** _______________
**Blocker Identified:** Yes / No
**Blocker Description (if any):** _______________________________________

---

*Checklist created for plan 06-02: LSA Authentication Package Testing*
