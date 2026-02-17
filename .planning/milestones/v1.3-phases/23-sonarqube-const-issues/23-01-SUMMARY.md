---
phase: 23-sonarqube-const-issues
plan: 01
subsystem: code-quality
tags: [sonarqube, const-correctness, global-variables, won't-fix, documentation]

# Dependency graph
requires:
  - phase: 16-const-correctness
    provides: Initial const conversions for immutable globals
  - phase: 22-sonarqube-macro-issues
    provides: Pattern for documenting won't-fix categories
provides:
  - Complete documentation of won't-fix global variable categories
  - Justification for each won't-fix category with Windows API interop explanations
  - Reference for Phase 30 Final SonarQube Scan resolution
affects: [phase-30-final-verification, sonarqube-resolution]

# Tech tracking
tech-stack:
  added: []
  patterns: [won't-fix documentation, Windows API interop const constraints]

key-files:
  created: [".planning/phases/23-sonarqube-const-issues/23-01-SUMMARY.md"]
  modified: []

key-decisions:
  - "All remaining ~32 global variables are legitimately mutable and cannot be marked const"
  - "Windows CryptoAPI requires non-const char arrays for CERT_ENHKEY_USAGE.rgpszUsageIdentifier"
  - "LSA function pointers must remain mutable as they are assigned at runtime during package initialization"
  - "UI state variables in EIDConfigurationWizard track user selections and navigation state"

patterns-established:
  - "Pattern: Won't-fix documentation with specific justification for each category"
  - "Pattern: Categorization by mutability reason (runtime assignment, API requirements, lifecycle state)"

requirements-completed: [SONAR-03]

# Metrics
duration: 8min
completed: 2026-02-17
---

# Phase 23 Plan 01: SonarQube Const Issues Summary

**Verified all remaining global variable const issues are legitimately mutable with documented won't-fix justifications for SonarQube resolution**

## Performance

- **Duration:** 8 min
- **Started:** 2026-02-17T12:10:27Z
- **Completed:** 2026-02-17T12:18:35Z
- **Tasks:** 3
- **Files modified:** 0 (documentation-only phase)

## Accomplishments
- Verified all 32 remaining global variables fall into won't-fix categories
- Confirmed build passes with 0 errors, 64 pre-existing warnings (ntstatus.h macro redefinitions)
- Documented 6 won't-fix categories with specific justifications for SonarQube resolution
- No code changes required - all globals are legitimately mutable

## Task Commits

Each task was committed atomically:

1. **Task 1: Verify remaining global variables are legitimately mutable** - No commit (documentation/verification only)
2. **Task 2: Build verification** - No commit (no code changes, verification only)
3. **Task 3: Document won't-fix categories** - `docs(23-01): complete won't-fix documentation`

**Plan metadata:** (to be committed)

## Files Created/Modified
- `.planning/phases/23-sonarqube-const-issues/23-01-SUMMARY.md` - Complete won't-fix category documentation

## Decisions Made
- All remaining global variables are legitimately mutable - no const conversions needed
- 6 won't-fix categories established with Windows API interop justifications
- Phase 30 can use this documentation to resolve SonarQube global variable const issues

## Won't-Fix Categories

### Category 1: LSA Function Pointers (3 globals in Package.cpp)

| Variable | Type | Justification |
|----------|------|---------------|
| `MyAllocateHeap` | PLSA_ALLOCATE_LSA_HEAP | Assigned at runtime by LSA via SetAlloc() |
| `MyFreeHeap` | PLSA_FREE_LSA_HEAP | Assigned at runtime by LSA via SetFree() |
| `MyImpersonate` | PLSA_IMPERSONATE_CLIENT | Assigned at runtime by LSA via SetImpersonate() |

**Won't Fix Reason:** These function pointers are initialized to nullptr at declaration and assigned their actual values during LSA package initialization. Marking them const would prevent this required runtime assignment.

### Category 2: Tracing State (4 globals in Tracing.cpp, TraceExport.cpp)

| Variable | Type | Justification |
|----------|------|---------------|
| `bFirst` | BOOL | Modified during tracing initialization |
| `g_csTraceInitialized` | BOOL | Set when critical section is initialized |
| `IsTracingEnabled` | BOOL | Modified by EnableCallback() at runtime |
| `g_hTraceOutputFile` | HANDLE | Set when trace file is opened |

**Won't Fix Reason:** Tracing state is enabled/disabled dynamically during program operation. File handles are opened on demand.

### Category 3: DLL State (2 globals in Dll.cpp)

| Variable | Type | Justification |
|----------|------|---------------|
| `g_cRef` | LONG | Modified by AddRef/Release for reference counting |
| `g_hinst` | HINSTANCE | Set by DllMain when DLL is loaded |

**Won't Fix Reason:** Standard COM DLL lifecycle state. Reference count must be mutable for proper object lifetime management. Instance handle is set by Windows during DLL loading.

### Category 4: UI State (6 globals in EIDConfigurationWizard*.cpp)

| Variable | Type | Location | Justification |
|----------|------|----------|---------------|
| `dwCurrentCredential` | DWORD | Page04.cpp | Tracks user's current credential selection |
| `fHasDeselected` | BOOL | Page04.cpp | Tracks if user has deselected a credential |
| `fGotoNewScreen` | BOOL | Wizard.cpp | Navigation state between wizard pages |
| `dwWizardError` | DWORD | Page05.cpp | Error code during wizard operation |
| `pRootCertificate` | PCCERT_CONTEXT | Page03.cpp | User-selected root certificate |
| `hwndInvalidPasswordBalloon` | HWND | Page05.cpp | Window handle for password validation UI |

**Won't Fix Reason:** These track user selections and navigation state during wizard operation. They must remain mutable to support interactive UI workflows.

### Category 5: Handle Variables (3 globals in various files)

| Variable | Type | Location | Justification |
|----------|------|----------|---------------|
| `hFile` | HANDLE | EIDLogManager.cpp | Set/opened during file operations |
| `hInternalLogWriteHandle` | HANDLE | DebugReport.cpp | Set when internal logging is enabled |
| `samsrvDll` | HMODULE | StoredCredentialManagement.cpp | Loaded at runtime via LoadLibrary |

**Won't Fix Reason:** File handles and module handles are opened/loaded during operation and cannot be const.

### Category 6: Windows API Buffers (14 globals in CertificateUtilities.cpp, CertificateValidation.cpp, CEIDCredential.cpp, helpers.cpp)

**Char arrays for CryptoAPI (CertificateUtilities.cpp, CertificateValidation.cpp):**

| Variable | Purpose |
|----------|---------|
| `s_szOidClientAuth[]` | Client authentication OID |
| `s_szOidServerAuth[]` | Server authentication OID |
| `s_szOidSmartCardLogon[]` | Smart card logon OID |
| `s_szOidEfs[]` | EFS OID |
| `s_szOidKeyUsage[]` | Key usage OID |
| `s_szOidBasicConstraints2[]` | Basic constraints OID |
| `s_szOidEnhancedKeyUsage[]` | Enhanced key usage OID |
| `s_szOidSubjectKeyId[]` | Subject key identifier OID |
| `s_szOidSha1RsaSign[]` | SHA1 RSA signature OID |

**Won't Fix Reason:** `CERT_ENHKEY_USAGE.rgpszUsageIdentifier` is `LPSTR*` (non-const char**). With `/Zc:strictStrings`, string literals are const and cannot convert to LPSTR. These MUST be writable arrays for Windows CryptoAPI compatibility.

**Wchar_t arrays for Credential Provider API:**

| Variable | Location | Purpose |
|----------|----------|---------|
| `s_wszUnknownError[]` | CEIDCredential.cpp | Default error message |
| `s_wszEmpty[]` | helpers.cpp | Empty string for field descriptors |
| `s_wszSpace[]` | Page05.cpp | Space character for UI |
| `s_wszEtlPath[]` | DebugReport.cpp | ETW trace file path |

**Won't Fix Reason:** `CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR.pszLabel` is `LPWSTR` (non-const). These buffers must be writable for Windows credential provider API compatibility.

## Already Const (No Action Needed)

The following globals were already marked const in previous phases:

| Variable | Type | Location | Phase Fixed |
|----------|------|----------|-------------|
| `TraceAllocation` | const BOOL | Package.cpp | Phase 16 |
| `dwReaderSize` | const DWORD | EIDConfigurationWizard.cpp | Phase 16 |
| `dwCardSize` | const DWORD | EIDConfigurationWizard.cpp | Phase 16 |
| `dwUserNameSize` | const DWORD | EIDConfigurationWizard.cpp | Phase 16 |
| `dwPasswordSize` | const DWORD | EIDConfigurationWizard.cpp | Phase 16 |

## Build Verification

Build completed successfully with:
- **0 Errors**
- **64 Warnings** (all pre-existing ntstatus.h macro redefinitions from Windows SDK - expected and documented)

## Summary Statistics

| Category | Count | Decision |
|----------|-------|----------|
| LSA Function Pointers | 3 | Won't Fix |
| Tracing State | 4 | Won't Fix |
| DLL State | 2 | Won't Fix |
| UI State | 6 | Won't Fix |
| Handle Variables | 3 | Won't Fix |
| Windows API Buffers | 14 | Won't Fix |
| Already Const | 5 | Already Fixed |
| **Total** | **37** | - |

## Deviations from Plan

None - plan executed exactly as written. All global variables verified as legitimately mutable with documented justifications.

## Issues Encountered

None - verification and documentation proceeded smoothly.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- SonarQube const issue analysis complete
- All won't-fix categories documented with technical justifications
- Ready for Phase 24 (SonarQube Nested Issues) or Phase 30 (Final SonarQube Scan)
- Phase 30 can reference this document to resolve global variable const issues in SonarQube dashboard

---
*Phase: 23-sonarqube-const-issues*
*Plan: 01*
*Completed: 2026-02-17*

## Self-Check: PASSED

- [x] SUMMARY.md exists (218 lines, required: 50)
- [x] Commit c649701 exists (won't-fix documentation)
- [x] Commit a82254f exists (STATE.md update)
- [x] Build passed with 0 errors
