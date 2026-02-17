---
phase: 22-sonarqube-macro-issues
verified: 2026-02-17T12:00:00Z
status: passed
score: 3/3 must-haves verified

---

# Phase 22: SonarQube Macro Issues Verification Report

**Phase Goal:** Constants use `const`/`constexpr` instead of preprocessor macros where safe
**Verified:** 2026-02-17T12:00:00Z
**Status:** PASSED
**Re-verification:** No - initial verification

## Goal Achievement

### Observable Truths

| #   | Truth | Status | Evidence |
| --- | ----- | ------ | -------- |
| 1 | Constants that can use `constexpr` are converted | VERIFIED | 4 macros converted: REMOVALPOLICYKEY, BITLEN_TO_CHECK, WM_MYMESSAGE (Page04), WM_MYMESSAGE (Page05) |
| 2 | Build passes with no new warnings | VERIFIED | Build succeeded with 0 Error(s), 0 Warning(s) |
| 3 | Won't-fix categories are documented with Windows API justification | VERIFIED | 6 categories documented in 22-03-SUMMARY.md (~91+ macros) |

**Score:** 3/3 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
| -------- | -------- | ------ | ------- |
| `EIDCardLibrary/CContainer.cpp` | REMOVALPOLICYKEY as static constexpr TCHAR[] | VERIFIED | Line 34: `static constexpr TCHAR REMOVALPOLICYKEY[] = TEXT(...)` |
| `EIDCardLibrary/CertificateUtilities.cpp` | BITLEN_TO_CHECK as static constexpr DWORD | VERIFIED | Line 1271: `static constexpr DWORD BITLEN_TO_CHECK = 2048;` |
| `EIDConfigurationWizard/EIDConfigurationWizardPage04.cpp` | WM_MYMESSAGE as constexpr UINT | VERIFIED | Line 422: `constexpr UINT WM_MYMESSAGE = WM_USER + 10;` |
| `EIDConfigurationWizard/EIDConfigurationWizardPage05.cpp` | WM_MYMESSAGE as constexpr UINT | VERIFIED | Line 86: `constexpr UINT WM_MYMESSAGE = WM_USER + 10;` |
| `EIDCardLibrary/EIDAuthenticateVersion.h` | EIDAuthenticateVersionText as #define (reverted) | VERIFIED | Line 6: `#define EIDAuthenticateVersionText` (documented as won't-fix for resource compiler) |

### Key Link Verification

| From | To | Via | Status | Details |
| ---- | -- | --- | ------ | ------- |
| REMOVALPOLICYKEY constexpr | RegOpenKey/RegCreateKeyEx | Windows registry API | WIRED | Used at lines 295, 313 in CContainer.cpp |
| BITLEN_TO_CHECK constexpr | Array size declarations | Compile-time array sizing | WIRED | Used in RSAPRIVKEY struct for modulus, prime, exponent arrays |
| WM_MYMESSAGE constexpr (Page04) | switch case / PostMessage | Window message handling | WIRED | Used at lines 434 (switch), 551 (PostMessage) |
| WM_MYMESSAGE constexpr (Page05) | switch case / PostMessage | Window message handling | WIRED | Used at lines 97 (switch), 225 (PostMessage) |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| ----------- | ----------- | ----------- | ------ | -------- |
| SONAR-02 | 22-01, 22-02, 22-03 | Review and resolve macro issues (~111 "replace with const/constexpr") | SATISFIED | 4 convertible macros converted, ~91 documented as won't-fix with justifications |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| ---- | ---- | ------- | -------- | ------ |
| None found | - | - | - | - |

### Human Verification Required

**1. SonarQube Scan Verification**

**Test:** Run SonarQube scan to confirm issue resolution count
**Expected:** Macro issue count reduced by 4 (or 5 if EIDAuthenticateVersionText is excluded from count)
**Why human:** Requires access to SonarQube server and running analysis pipeline

**2. Functional Testing of Configuration Wizard**

**Test:** Run EIDConfigurationWizard and test dialog page navigation
**Expected:** Custom message handling (WM_MYMESSAGE) works correctly for page transitions
**Why human:** Requires GUI interaction and runtime behavior verification

### Phase 22 Conversions Summary

**Successfully converted (4 macros):**
1. `REMOVALPOLICYKEY` - `static constexpr TCHAR[]` in CContainer.cpp (registry key path)
2. `BITLEN_TO_CHECK` - `static constexpr DWORD` in CertificateUtilities.cpp (RSA key bit length)
3. `WM_MYMESSAGE` (Page04) - `constexpr UINT` in EIDConfigurationWizardPage04.cpp (custom window message)
4. `WM_MYMESSAGE` (Page05) - `constexpr UINT` in EIDConfigurationWizardPage05.cpp (custom window message)

**Reverted (documented as won't-fix):**
- `EIDAuthenticateVersionText` - Must remain as `#define` for Windows resource compiler (RC.exe cannot process C++ constexpr)

**Won't-fix categories documented for Phase 30 SonarQube resolution (~91+ macros):**
1. Windows Header Configuration Macros (~25) - Control SDK header behavior
2. Function-Like Macros with Preprocessor Features (~10) - Use `__FILE__`, `__LINE__`, `##`, `#`
3. Resource ID Macros (~50) - Required by resource compiler
4. TCHAR-Dependent Macros (~1) - Uses `TEXT()` macro
5. Include Guards (~10) - Standard C++ pattern
6. Windows SDK Macros (~60) - External header file

### Commits Verified

All task commits present in git history:
- `a2f4a0f` - refactor(22-01): convert REMOVALPOLICYKEY macro to static constexpr
- `61cbc9d` - refactor(22-01): convert EIDAuthenticateVersionText macro to constexpr
- `e9ddf4d` - fix(22-01): revert EIDAuthenticateVersionText to #define
- `240bfd6` - fix(22-02): convert BITLEN_TO_CHECK macro to constexpr
- `847e229` - fix(22-02): convert WM_MYMESSAGE macro to constexpr in Page04
- `51421a3` - fix(22-02): convert WM_MYMESSAGE macro to constexpr in Page05

### Build Verification

```
Build succeeded.
    0 Warning(s)
    0 Error(s)

Time Elapsed 00:00:00.90
```

All 7 projects in EIDCredentialProvider.sln built successfully:
- EIDCardLibrary (contains REMOVALPOLICYKEY, BITLEN_TO_CHECK changes)
- EIDConfigurationWizard (contains WM_MYMESSAGE changes)
- EIDConfigurationWizardElevated
- EIDCredentialProvider
- EIDAuthenticationPackage
- EIDPasswordChangeNotification
- EIDLogManager

---

_Verified: 2026-02-17T12:00:00Z_
_Verifier: Claude (gsd-verifier)_
