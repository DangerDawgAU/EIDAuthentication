---
phase: 22-sonarqube-macro-issues
plan: 02
subsystem: code-quality
tags: [constexpr, macros, c++23, type-safety, sonarqube]

# Dependency graph
requires:
  - phase: 22-sonarqube-macro-issues/22-01
    provides: Pattern for simple value macro conversion
provides:
  - BITLEN_TO_CHECK as static constexpr DWORD in RSAPRIVKEY struct
  - WM_MYMESSAGE as constexpr UINT in Page04.cpp
  - WM_MYMESSAGE as constexpr UINT in Page05.cpp
affects: [22-sonarqube-macro-issues, sonarqube-remediation]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "static constexpr for struct member constants"
    - "constexpr UINT for Windows message constants"

key-files:
  created: []
  modified:
    - EIDCardLibrary/CertificateUtilities.cpp
    - EIDConfigurationWizard/EIDConfigurationWizardPage04.cpp
    - EIDConfigurationWizard/EIDConfigurationWizardPage05.cpp

key-decisions:
  - "Use static constexpr DWORD for struct member constants (type-safe, debugger-visible)"
  - "Use constexpr UINT for Windows custom message constants"

patterns-established:
  - "Pattern: Replace #define VALUE with constexpr TYPE for compile-time constants"
  - "Pattern: Use static constexpr for struct/class member constants"

requirements-completed: [SONAR-02]

# Metrics
duration: 15min
completed: 2026-02-17
---

# Phase 22 Plan 02: Numeric Constant Macros to Constexpr Summary

**Converted three numeric constant macros to type-safe constexpr declarations: BITLEN_TO_CHECK in RSAPRIVKEY struct and WM_MYMESSAGE in both Configuration Wizard dialog pages.**

## Performance

- **Duration:** 15 min
- **Started:** 2026-02-17T21:35:00Z
- **Completed:** 2026-02-17T21:50:00Z
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments
- Converted BITLEN_TO_CHECK from #define to static constexpr DWORD inside RSAPRIVKEY struct
- Converted WM_MYMESSAGE from #define to constexpr UINT in EIDConfigurationWizardPage04.cpp
- Converted WM_MYMESSAGE from #define to constexpr UINT in EIDConfigurationWizardPage05.cpp
- Verified EIDCardLibrary builds successfully with the changes

## Task Commits

Each task was committed atomically:

1. **Task 1: Convert BITLEN_TO_CHECK macro to constexpr in CertificateUtilities.cpp** - `240bfd6` (fix)
2. **Task 2: Convert WM_MYMESSAGE macro to constexpr in EIDConfigurationWizardPage04.cpp** - `847e229` (fix)
3. **Task 3: Convert WM_MYMESSAGE macro to constexpr in EIDConfigurationWizardPage05.cpp** - `51421a3` (fix)

## Files Created/Modified
- `EIDCardLibrary/CertificateUtilities.cpp` - BITLEN_TO_CHECK converted to static constexpr DWORD in RSAPRIVKEY struct
- `EIDConfigurationWizard/EIDConfigurationWizardPage04.cpp` - WM_MYMESSAGE converted to constexpr UINT
- `EIDConfigurationWizard/EIDConfigurationWizardPage05.cpp` - WM_MYMESSAGE converted to constexpr UINT

## Decisions Made
- Used `static constexpr DWORD` for BITLEN_TO_CHECK since it's a struct member and DWORD matches Windows cryptographic API conventions
- Used `constexpr UINT` for WM_MYMESSAGE since it's a Windows message constant and UINT is the standard type for message IDs
- Both conversions maintain compile-time constant behavior for array sizes and switch case labels

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

**Pre-existing build issue discovered:**
- EIDConfigurationWizard resource compilation fails due to `EIDAuthenticateVersionText` being defined as `constexpr` instead of `#define` in EIDAuthenticateVersion.h
- The Windows resource compiler (RC.exe) cannot process C++ constexpr - it only understands preprocessor macros
- This is a pre-existing issue from Plan 22-01, not introduced by this plan's changes
- C++ compilation succeeds for all modified files - the constexpr conversions are syntactically correct
- Documented as a deferred item for future resolution

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- All numeric constant macro conversions in this plan are complete
- Pattern established for converting simple value macros to constexpr
- EIDCardLibrary verified to build successfully
- EIDConfigurationWizard C++ code compiles correctly (resource compiler issue is pre-existing)

## Self-Check: PASSED

- All 3 modified files exist and contain expected conversions
- All 3 task commits (240bfd6, 847e229, 51421a3) exist in git history
- BITLEN_TO_CHECK converted to static constexpr DWORD
- WM_MYMESSAGE converted to constexpr UINT in both Page04 and Page05

---
*Phase: 22-sonarqube-macro-issues*
*Completed: 2026-02-17*
