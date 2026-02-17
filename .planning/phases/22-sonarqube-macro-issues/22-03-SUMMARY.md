---
phase: 22-sonarqube-macro-issues
plan: 03
subsystem: code-quality
tags: [sonarqube, constexpr, verification, build, documentation]

# Dependency graph
requires:
  - phase: 22-sonarqube-macro-issues/22-01
    provides: REMOVALPOLICYKEY constexpr conversion
  - phase: 22-sonarqube-macro-issues/22-02
    provides: BITLEN_TO_CHECK, WM_MYMESSAGE constexpr conversions
provides:
  - Build verification confirming all constexpr conversions compile correctly
  - Compile-time context usage verification (switch statements, array sizes, registry APIs)
  - Won't-fix macro categories documented for SonarQube resolution
affects: [22-sonarqube-macro-issues, sonarqube-remediation, phase-30-final-scan]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "constexpr constants usable in switch case labels"
    - "constexpr constants usable in array size declarations"
    - "static constexpr TCHAR[] usable with Windows registry APIs"

key-files:
  created: []
  modified: []

key-decisions:
  - "EIDAuthenticateVersionText reverted to #define (commit e9ddf4d) - resource compiler cannot process constexpr"
  - "Won't-fix macro categories documented for Phase 30 SonarQube resolution"

patterns-established:
  - "Pattern: constexpr constants are usable in all compile-time contexts (switch, array sizes)"
  - "Pattern: Resource compiler (.rc files) requires #define, not constexpr"

requirements-completed: [SONAR-02]

# Metrics
duration: 4min
completed: 2026-02-17
---

# Phase 22 Plan 03: Build Verification Summary

**Verified all Phase 22 constexpr conversions compile correctly with zero errors, confirmed compile-time usability in switch statements and array sizes, and documented won't-fix macro categories for SonarQube resolution**

## Performance

- **Duration:** 4 min
- **Started:** 2026-02-17T11:47:03Z
- **Completed:** 2026-02-17T11:51:00Z
- **Tasks:** 3
- **Files modified:** 0 (verification only)

## Accomplishments
- Full solution build succeeded with 0 errors and 64 warnings (all pre-existing Windows SDK ntstatus.h macro redefinition warnings)
- Verified constexpr constants work in all compile-time contexts:
  - `case WM_MYMESSAGE:` switch case labels (Page04.cpp, Page05.cpp)
  - `BYTE modulus[BITLEN_TO_CHECK/8]` array size declarations (CertificateUtilities.cpp)
  - `RegOpenKey(..., REMOVALPOLICYKEY, ...)` Windows API calls (CContainer.cpp)
- Documented 6 won't-fix macro categories (~91+ macros) with justifications for Phase 30 SonarQube resolution

## Task Commits

This plan consists of verification and documentation tasks - no code changes were required.

1. **Task 1: Build all 7 projects with macro conversions** - Verification only (no commit needed)
2. **Task 2: Verify constexpr usage in compile-time contexts** - Verification only (no commit needed)
3. **Task 3: Document remaining won't-fix macros** - Documentation only (captured in this summary)

## Files Created/Modified
- None - this plan was verification and documentation only

## Decisions Made
- EIDAuthenticateVersionText must remain as `#define` because it is used in .rc resource files which the resource compiler (RC.exe) processes - the resource compiler cannot handle C++ constexpr
- All 64 warnings in the build are pre-existing C4005 macro redefinition warnings from Windows SDK ntstatus.h - not caused by our constexpr conversions

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None - all verification checks passed successfully.

## Won't-Fix Macro Categories (for Phase 30 SonarQube Resolution)

1. **Windows Header Configuration Macros** (~25 occurrences)
   - `WIN32_NO_STATUS`, `SECURITY_WIN32`, `_CRTDBG_MAP_ALLOC`, `_SEC_WINNT_AUTH_TYPES`
   - These MUST remain as macros; they configure Windows SDK header behavior before includes

2. **Function-Like Macros with Preprocessor Features** (~10 occurrences)
   - `EIDCardLibraryTrace(...)`, `EIDAlloc(...)`, `EIDFree(...)`, `CHECK_DWORD(...)`, `ERRORTOTEXT(ERROR)`
   - These use preprocessor-only features (`__FILE__`, `__LINE__`, `##`, `#`) with no C++ equivalent

3. **Resource ID Macros** (~50 occurrences)
   - `IDD_*`, `IDC_*`, `IDS_*`, `IDB_*`, `_APS_NEXT_*` in resource.h files
   - Resource compiler (.rc files) requires `#define`, not C++ constants

4. **TCHAR-Dependent Macros** (~1 occurrence)
   - `AUTHENTICATIONPACKAGENAMET` - Uses `TEXT()` macro for TCHAR compatibility

5. **Include Guards** (~10 occurrences)
   - Standard C++ header protection pattern

6. **Windows SDK Macros** (~60 occurrences in include/cardmod.h)
   - External Windows SDK header file - do not modify

**Total Won't Fix: ~91+ macros**

## Phase 22 Complete Summary

**Successfully converted macros:**
1. `REMOVALPOLICYKEY` - `static constexpr TCHAR[]` in CContainer.cpp
2. `BITLEN_TO_CHECK` - `static constexpr DWORD` in CertificateUtilities.cpp (RSAPRIVKEY struct)
3. `WM_MYMESSAGE` (Page04) - `constexpr UINT` in EIDConfigurationWizardPage04.cpp
4. `WM_MYMESSAGE` (Page05) - `constexpr UINT` in EIDConfigurationWizardPage05.cpp

**Reverted (won't fix):**
- `EIDAuthenticateVersionText` - Must remain as `#define` for resource compiler (commit e9ddf4d)

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Phase 22 (SonarQube Macro Issues) complete
- All convertible macros have been converted to constexpr
- Won't-fix categories documented for Phase 30 Final SonarQube Scan
- Ready for Phase 23 (SonarQube Const Issues) or next phase in sequence

---
*Phase: 22-sonarqube-macro-issues*
*Completed: 2026-02-17*
## Self-Check: PASSED
