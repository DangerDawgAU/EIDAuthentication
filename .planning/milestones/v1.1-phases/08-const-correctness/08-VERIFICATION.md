# Phase 8: Const Correctness - VERIFICATION

**Date:** 2026-02-17
**Status:** Complete (with documented exceptions)

## Issues Fixed (Code Changes)

### CContainerHolderTest - Member Functions Marked const
**File:** `EIDConfigurationWizard/CContainerHolder.h` and `CContainerHolder.cpp`

Methods correctly marked as `const`:
- `GetContainer() const` - returns member pointer, no state modification
- `GetIconIndex() const` - returns value based on members, no modification
- `HasSignatureUsageOnly() const` - reads state, no modification
- `SupportEncryption() const` - reads state, no modification
- `GetCheckCount() const` - returns constant value
- `GetImage(DWORD) const` - reads state, no modification
- `GetDescription(DWORD) const` - allocates memory but doesn't modify object
- `GetSolveDescription(DWORD) const` - allocates memory but doesn't modify object

Method intentionally NOT marked const:
- `IsTrusted()` - Has side effect: sets `_dwTrustError` member

## Issues Marked as "Won't Fix" (Justified)

### Category 1: State Variables (71 issues)
**SonarQube Rule:** "Global variables should be const"

**Justification:** These are mutable state variables used by the wizard to track user selections and operation state. They MUST be non-const.

**Examples from global.h:**
- `extern HINSTANCE g_hinst;` - Set during initialization
- `extern BOOL fShowNewCertificatePanel;` - Modified during wizard operation
- `extern WCHAR szReader[];` - Modified to store reader name
- `extern DWORD dwReaderSize;` - Modified to store size
- `extern WCHAR szUserName[];` - Modified to store username
- `extern WCHAR szPassword[];` - Modified to store password

**Action:** Mark as "Won't Fix" in SonarQube with comment: "State variable - must be mutable for wizard operation"

### Category 2: Windows API Compatibility (31 issues)
**SonarQube Rule:** "Global pointers should be const at every level"

**Justification:** These static buffers are required for Windows API compatibility. Many Windows APIs require non-const pointers (LPWSTR, PWSTR, LPTSTR) even when they don't modify the data.

**Examples:**
- `static wchar_t s_wszEmpty[] = L"";` - Used with LPWSTR parameters
- `static wchar_t s_wszEmptyLabel[] = L"";` - Used with CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR.pszLabel (LPWSTR)
- `static char s_szMyTest[] = "MYTEST";` - Used with LSA_STRING.Buffer (PCHAR)

**Why can't we use const:**
1. Windows API structures like `CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR` declare `pszLabel` as `LPWSTR` (non-const)
2. Using `const_cast` at every call site would be more dangerous than keeping the buffer non-const
3. The buffers are never actually modified - they're just in non-const storage for API compatibility

**Action:** Mark as "Won't Fix" in SonarQube with comment: "Windows API compatibility - LPWSTR requires non-const pointer"

### Category 3: CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR Arrays
**Files:** `EIDCredentialProvider/common.h`

```cpp
static CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR s_rgCredProvFieldDescriptors[] = { ... };
```

**Justification:** These arrays are initialized once but their `pszLabel` member must point to non-const memory. The field descriptors are used throughout the credential provider lifecycle.

**Action:** Mark as "Won't Fix" - Windows API requirement

## Summary

| Category | Count | Action |
|----------|-------|--------|
| Member functions fixed | 8 | Code updated with const |
| State variables | ~71 | Mark as "Won't Fix" |
| Windows API compatibility | ~31 | Mark as "Won't Fix" |
| Other (reviewed, justified) | ~6 | Mark as "Won't Fix" |

## User Action Required

After this phase completes, run SonarQube scan and mark the following issue types as "Won't Fix":

1. **Global variables should be const** on files:
   - `EIDConfigurationWizard/global.h` (state variables)
   - `EIDCredentialProvider/Dll.cpp` (g_hinst)

2. **Global pointers should be const** on files:
   - `EIDCredentialProvider/helpers.cpp` (s_wszEmpty)
   - `EIDCredentialProvider/common.h` (s_wszEmptyLabel, field descriptors)
   - `EIDConfigurationWizard/DebugReport.cpp` (s_szMyTest, s_wszEtlPath)
   - `EIDConfigurationWizard/EIDConfigurationWizardPage05.cpp` (s_wszSpace)

## Build Verification

- [x] Full solution builds without errors
- [x] No new compiler warnings introduced
- [x] All const methods compile correctly
