---
phase: 22-sonarqube-macro-issues
plan: 01
subsystem: code-quality
tags: [sonarqube, constexpr, macro, type-safety, c++23]

# Dependency graph
requires: []
provides:
  - REMOVALPOLICYKEY as static constexpr TCHAR array in CContainer.cpp
  - EIDAuthenticateVersionText as constexpr const char* in EIDAuthenticateVersion.h
affects: []

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Replace simple value macros with constexpr for type safety"
    - "Use static constexpr for file-scope constants to ensure internal linkage"
    - "Use array syntax TCHAR[] for string literals instead of pointer types"

key-files:
  created: []
  modified:
    - EIDCardLibrary/CContainer.cpp
    - EIDCardLibrary/EIDAuthenticateVersion.h

key-decisions:
  - "Convert REMOVALPOLICYKEY to static constexpr TCHAR[] for type safety and internal linkage"
  - "Convert EIDAuthenticateVersionText to constexpr const char* for header visibility"
  - "Leave EIDAuthenticateVersionNumeric as macro (1.1.1.1 is not valid C++ numeric literal syntax)"

patterns-established:
  - "Pattern: Use static constexpr for file-scope constants to avoid ODR violations"
  - "Pattern: Use TCHAR[] array syntax for string literals with constexpr for efficiency"

requirements-completed:
  - SONAR-02

# Metrics
duration: 7min
completed: 2026-02-17
---

# Phase 22 Plan 01: Simple Value Macro Conversion Summary

**Converted two preprocessor macros to type-safe constexpr constants in EIDCardLibrary, improving type safety and debugger visibility while maintaining zero runtime overhead**

## Performance

- **Duration:** 7 min
- **Started:** 2026-02-17T11:28:26Z
- **Completed:** 2026-02-17T11:35:37Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments
- Converted REMOVALPOLICYKEY from `#define` to `static constexpr TCHAR[]` in CContainer.cpp
- Converted EIDAuthenticateVersionText from `#define` to `constexpr const char*` in EIDAuthenticateVersion.h
- Build verified successfully with no new warnings introduced
- Identified EIDAuthenticateVersionNumeric as non-convertible (invalid C++ literal syntax)

## Task Commits

Each task was committed atomically:

1. **Task 1: Convert REMOVALPOLICYKEY macro to static constexpr in CContainer.cpp** - `a2f4a0f` (refactor)
2. **Task 2: Convert EIDAuthenticateVersionText macro to constexpr** - `61cbc9d` (refactor)

## Files Created/Modified
- `EIDCardLibrary/CContainer.cpp` - Converted REMOVALPOLICYKEY from macro to static constexpr TCHAR[]
- `EIDCardLibrary/EIDAuthenticateVersion.h` - Converted EIDAuthenticateVersionText from macro to constexpr const char*

## Decisions Made
- Used `static constexpr TCHAR[]` for REMOVALPOLICYKEY to ensure internal linkage (avoid ODR violations) and efficient string literal storage
- Used `constexpr const char*` for EIDAuthenticateVersionText since it's a header file constant visible to all translation units
- Left EIDAuthenticateVersionNumeric as a macro because its value `1.1.1.1` is not a valid C++ numeric literal (appears to be placeholder or documentation value)

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

Build required using solution file context (EIDCredentialProvider.sln) rather than project file directly due to include path dependencies. MSBuild command required double-slash syntax for options on Windows bash.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Phase 22 Plan 01 complete
- Two macros converted to constexpr as specified
- Ready for remaining Phase 22 plans (22-02, 22-03)
- No blockers or concerns

---
*Phase: 22-sonarqube-macro-issues*
*Completed: 2026-02-17*
