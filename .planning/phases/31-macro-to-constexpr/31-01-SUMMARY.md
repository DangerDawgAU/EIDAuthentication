---
phase: 31-macro-to-constexpr
plan: 01
subsystem: code-quality
tags: [constexpr, macro, modernization, type-safety, c++23]

# Dependency graph
requires:
  - phase: v1.3
    provides: Codebase with C++23 flag enabled
provides:
  - Simple value macros converted to constexpr (enables Phase 34 const correctness)
  - Documented won't-fix macro categories (MACRO-02, MACRO-03)
affects:
  - 34-const-correctness-globals

# Tech tracking
tech-stack:
  added: []
  patterns:
    - constexpr for compile-time type-safe constants
    - #undef/#define pattern for overriding SDK macros when required

key-files:
  created: []
  modified:
    - EIDCredentialProvider/common.h
    - EIDLogManager/EIDLogManager.cpp
    - EIDCardLibrary/EIDCardLibrary.h

key-decisions:
  - "CLSCTX_INPROC_SERVER renamed to CLSCTX_INPROC_SERVER_LOCAL to avoid confusion with Windows SDK definition"
  - "CERT_HASH_LENGTH documented as won't-fix because Windows SDK defines it as a macro"
  - "Resource compiler and flow-control macros remain as #define with documented justifications"

patterns-established:
  - "Pattern: Convert simple value macros to constexpr for type safety"
  - "Pattern: Document won't-fix macros with MACRO-XX category tags"

requirements-completed:
  - MACRO-01
  - MACRO-02
  - MACRO-03

# Metrics
duration: 15min
completed: 2026-02-18
---

# Phase 31 Plan 01: Macro to constexpr Summary

**Converted 2 simple value macros to constexpr and documented CERT_HASH_LENGTH as won't-fix due to Windows SDK conflict, enabling Phase 34 const correctness for global constants.**

## Performance

- **Duration:** 15 min
- **Started:** 2026-02-18T02:06:42Z
- **Completed:** 2026-02-18T02:21:00Z
- **Tasks:** 4
- **Files modified:** 3

## Accomplishments

- MAX_ULONG converted from `#define` to `constexpr ULONG` with type safety
- CLSCTX_INPROC_SERVER converted to `constexpr LONG CLSCTX_INPROC_SERVER_LOCAL` with descriptive rename
- CERT_HASH_LENGTH documented as won't-fix with MACRO-02 category (Windows SDK conflict)
- Full solution rebuild verified with zero build errors

## Task Commits

Each task was committed atomically:

1. **Task 1: Convert MAX_ULONG macro to constexpr** - `46f38c6` (feat)
2. **Task 2: Convert CLSCTX_INPROC_SERVER macro to constexpr** - `815c7e0` (feat)
3. **Task 3: Document CERT_HASH_LENGTH macro as won't-fix** - `19aa6d7` (docs)
4. **Task 4: Verify build passes after macro conversions** - No commit needed (verification only)

## Files Created/Modified

- `EIDCredentialProvider/common.h` - Converted MAX_ULONG from macro to constexpr ULONG
- `EIDLogManager/EIDLogManager.cpp` - Converted CLSCTX_INPROC_SERVER to constexpr and updated usage
- `EIDCardLibrary/EIDCardLibrary.h` - Added WON'T-FIX (MACRO-02) documentation for CERT_HASH_LENGTH

## Decisions Made

- **CLSCTX_INPROC_SERVER rename:** Renamed to `CLSCTX_INPROC_SERVER_LOCAL` to avoid confusion with Windows SDK's CLSCTX_INPROC_SERVER enum value. The project uses a custom local value (1) for EIDLogManager specifically.
- **CERT_HASH_LENGTH won't-fix:** Cannot convert to constexpr because Windows SDK (WinCred.h) defines CERT_HASH_LENGTH as a macro for SHA-1 (20 bytes). The codebase overrides it to SHA-256 (32 bytes) using #undef/#define pattern, which is required since the preprocessor runs before constexpr compilation.
- **No resource/flow-control macro changes:** Resource compiler macros (IDD_*, IDC_*, etc.) and flow-control macros (CHECK_*, EIDAlloc, etc.) remain as #define with documented justifications per STATE.md won't-fix categories.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None - all tasks completed without issues.

## User Setup Required

None - no external service configuration required.

## Won't-Fix Macro Categories Confirmed

| Category | Example Macros | Justification |
|----------|---------------|---------------|
| Resource compiler macros | IDD_*, IDC_*, IDS_*, IDB_*, _APS_* | RC.exe cannot process C++ constexpr (MACRO-02) |
| Flow-control macros | CHECK_DWORD, CHECK_BOOL, EIDAlloc, EIDFree | SEH/context requirements for __leave flow control (MACRO-03) |
| SDK configuration macros | WIN32_NO_STATUS, SECURITY_WIN32, _CRTDBG_MAP_ALLOC | Windows SDK compatibility (MACRO-03) |
| Third-party header macros | cardmod.h definitions | External file, not our code (MACRO-03) |
| Windows SDK overrides | CERT_HASH_LENGTH | Preprocessor #undef/#define required to override SDK value (MACRO-02) |

## Next Phase Readiness

- Phase 31 (Macro to constexpr) complete
- Phase 34 (Const Correctness - Globals) can now proceed - macros are constexpr-enabled
- No blockers or concerns

---
*Phase: 31-macro-to-constexpr*
*Completed: 2026-02-18*

## Self-Check: PASSED

- SUMMARY.md exists: True
- Task 1 commit (46f38c6): Found
- Task 2 commit (815c7e0): Found
- Task 3 commit (19aa6d7): Found
- Build verification: Zero errors
