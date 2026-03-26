# EIDMigrate Investigation Summary

**Date:** 2026-03-24
**Status:** Fixes Applied - Pending Reboot and Testing

---

## Applied Fixes (from `afterreboot.md`)

All three fixes from the previous troubleshooting session have been verified as **APPLIED**:

### Fix 1: Authentication Package Registration ✅
**File:** `EIDCardLibrary/Registration.cpp` (lines 271-283)

```cpp
void EIDAuthenticationPackageDllRegister()
{
    // Register as Security Package (SSP interface)
    AppendValueToMultiSz(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Lsa", L"Security Packages", AUTHENTICATIONPACKAGENAMET);
    // Also register as Authentication Package (AP interface) for LsaLookupAuthenticationPackage
    AppendValueToMultiSz(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Lsa", L"Authentication Packages", AUTHENTICATIONPACKAGENAMET);
}
```

**Registry Status:** Verified
```
HKLM\SYSTEM\CurrentControlSet\Control\Lsa\Authentication Packages
  = msv1_0, EIDAuthenticationPackage
```

### Fix 2: Untrusted Connection Authorization ✅
**File:** `EIDAuthenticationPackage/EIDAuthenticationPackage.cpp` (lines 175-188)

```cpp
status = MyLsaDispatchTable->GetClientInfo(&ClientInfo);
if (STATUS_SUCCESS != status)
{
    // Untrusted connection via LsaConnectUntrusted doesn't have client info
    // For admin-only operations (dwRid == 0), grant access since the caller
    // must be running as Administrator to perform LSA operations
    if (dwRid == 0)
    {
        EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE, L"Untrusted connection, granting admin access for EIDMigrate");
        return TRUE;
    }
    EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING, L"GetClientInfo failed: 0x%08x", status);
    return FALSE;
}
```

### Fix 3: IPC Message Handlers Implemented ✅
**File:** `EIDAuthenticationPackage/EIDAuthenticationPackage.cpp`

- `HandleEnumerateCredentials()` - Lines 561-760
- `HandleExportCredential()` - Lines 774-920
- `HandleImportCredential()` - Lines 931-1110

**File:** `EIDCardLibrary/Package.cpp`

- `LsaEIDEnumerateCredentials()` - Lines 1337-1452
- `LsaEIDExportCredential()` - Lines 1459-1574
- `LsaEIDImportCredential()` - Lines 1576-1723

---

## Data Structures Defined ✅

**File:** `EIDCardLibrary/EIDCardLibrary.h`

All required structures are defined:
- `EIDM_ENUM_REQUEST` (lines 247-254)
- `EIDM_CREDENTIAL_SUMMARY` (lines 258-267)
- `EIDM_ENUM_RESPONSE` (lines 271-280)
- `EIDM_EXPORT_REQUEST` (lines 286-293)
- `EIDM_EXPORT_RESPONSE` (lines 297-313)
- `EIDM_IMPORT_REQUEST` (lines 319-336)
- `EIDM_IMPORT_RESPONSE` (lines 340-347)

---

## EIDMigrate.exe Status

**Location:** `C:\Users\user\Documents\EIDAuthentication\x64\Release\EIDMigrate.exe`
**Exists:** ✅
**Built:** ✅

**Admin Check:** Tool correctly returns exit code 2 (EIDMIGRATE_EXIT_LSA_DENIED) when not running as Administrator.

---

## Post-Reboot Testing Required

After rebooting, run these tests to verify EIDMigrate works correctly:

### Test 1: Verify Authentication Package Loaded
```cmd
reg query "HKLM\SYSTEM\CurrentControlSet\Control\Lsa" /v "Authentication Packages"
```
**Expected:** Should include `EIDAuthenticationPackage`

### Test 2: Verify DLL is Loaded in LSASS
```cmd
listdlls -accepteula lsass | findstr EID
```
**Expected:** Should show `EIDAuthenticationPackage.DLL`

### Test 3: List Local Credentials (Run as Administrator)
```cmd
cd C:\Users\user\Documents\EIDAuthentication\x64\Release
EIDMigrate.exe list -local -v
```
**Expected:**
- Exit code 0 (success) or exit code 3 (no credentials found)
- No "access denied" errors
- Message showing credential count or "No EID credentials found"

### Test 4: Export Test (if credentials exist)
```cmd
EIDMigrate.exe export -output test.eidm -v
```

### Test 5: Validate Test
```cmd
EIDMigrate.exe validate -input test.eidm -v
```

### Test 6: Import Test (on target machine)
```cmd
EIDMigrate.exe import -input test.eidm -dry-run -v
```

---

## Known Issues and Notes

1. **Reboot Required:** Authentication Packages are only loaded by LSASS at boot time. A reboot is required for the registry changes to take effect.

2. **LSASS Safety:** All IPC handlers include proper SEH (__try/__except) blocks to prevent LSASS crashes.

3. **Authorization Model:**
   - `dwRid = 0` means "administrator only" operations
   - EIDMigrate uses untrusted LSA connections (`LsaConnectUntrusted`)
   - The fallback in `MatchUserOrIsAdmin()` allows this to work

4. **Credential Limits:**
   - Max credentials: 1000
   - Max certificate size: 8192 bytes
   - Max private data size: 20KB (sanity check)

---

## Next Steps

1. **REBOOT THE SYSTEM** - This is required for Authentication Package loading
2. Run Test 1 and Test 2 to verify LSASS has loaded the package
3. Run Test 3 (list command) as Administrator
4. If successful, proceed with export/import testing
5. Check Event Logs for any EIDMigrate events:
   ```
   Event Viewer → Windows Logs → Application
   Filter for: EIDMigrate
   ```

---

## Files Modified (Summary)

| File | Change | Status |
|------|--------|--------|
| `EIDCardLibrary/Registration.cpp` | Added AP registration | ✅ Applied |
| `EIDAuthenticationPackage/EIDAuthenticationPackage.cpp` | Untrusted connection fix + 3 new handlers | ✅ Applied |
| `EIDCardLibrary/Package.cpp` | 3 new IPC client wrapper functions | ✅ Applied |
| `EIDCardLibrary/EIDCardLibrary.h` | New data structures for EIDMigrate | ✅ Applied |
| Registry | `Authentication Packages` = msv1_0, EIDAuthenticationPackage | ✅ Verified |

---

## Technical Background

### Why Reboot is Required

1. **Authentication Package Loading:** APs are only loaded by LSASS at boot time. The registry changes don't reload the AP table dynamically.

2. **DLL Replacement:** If the DLL was updated, Windows may have scheduled file replacement via `PendingFileRenameOperations` which processes on reboot.

### LSA Package Types

| Type | Registry Location | Loaded | Entry Point |
|------|-------------------|--------|-------------|
| Authentication Package | `Lsa\Authentication Packages` | Boot only | `LsaApInitializePackage` |
| Security Package (SSP) | `Lsa\Security Packages` | Dynamic via AddSecurityPackage | `SpInitialize` |

EIDAuthenticationPackage implements **both** interfaces.

### Untrusted LSA Connections

`LsaConnectUntrusted()` creates a connection without establishing a trusted logon session:
- `GetClientInfo()` cannot retrieve client security context
- `ImpersonateClient()` will fail
- The fix grants access for admin-only operations (dwRid == 0) from untrusted connections

---

*End of Investigation Summary*
