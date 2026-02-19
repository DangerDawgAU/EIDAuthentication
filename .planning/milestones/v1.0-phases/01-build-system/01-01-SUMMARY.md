---
phase: 01-build-system
plan: 01
subsystem: build
tags: [cpp23, msvc, vcxproj, language-standard, e-idcardlibrary]

# Dependency graph
requires: []
provides:
  - C++23 language standard configuration in EIDCardLibrary.vcxproj
  - Build configuration pattern for remaining 6 projects
affects: [02-e-idcredentialprovider, 03-e-idauthenticate, 04-e-idtest, 05-e-idtestcredential]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "LanguageStandard element placement: first child of ClCompile in ItemDefinitionGroup"
    - "stdcpp23 enum value for /std:c++23preview in VS 2026"

key-files:
  created: []
  modified:
    - EIDCardLibrary/EIDCardLibrary.vcxproj

key-decisions:
  - "Used stdcpp23 instead of stdcpp23preview (VS 2026 uses this enum value for /std:c++23preview flag)"
  - "42 compile errors expected - const-correctness issues from /Zc:strictStrings to be fixed in subsequent plans"

patterns-established:
  - "Pattern: Insert LanguageStandard as first child of ClCompile in ItemDefinitionGroup"
  - "Pattern: Preserve RuntimeLibrary settings (MultiThreaded/MultiThreadedDebug) for static CRT linkage"

# Metrics
duration: 12min
completed: 2026-02-15
---

# Phase 1 Plan 1: C++23 Flag for EIDCardLibrary Summary

**Added C++23 language standard configuration to all 4 build configurations (Debug/Release x Win32/x64) in EIDCardLibrary.vcxproj, revealing 42 const-correctness compile errors from /Zc:strictStrings as expected per locked decision.**

## Performance

- **Duration:** 12 min
- **Started:** 2026-02-15T10:30:00Z
- **Completed:** 2026-02-15T10:42:00Z
- **Tasks:** 5 (4 auto + 1 checkpoint)
- **Files modified:** 1

## Accomplishments

- Added `<LanguageStandard>stdcpp23</LanguageStandard>` to Debug|Win32 configuration
- Added `<LanguageStandard>stdcpp23</LanguageStandard>` to Debug|x64 configuration
- Added `<LanguageStandard>stdcpp23</LanguageStandard>` to Release|Win32 configuration
- Added `<LanguageStandard>stdcpp23</LanguageStandard>` to Release|x64 configuration
- Verified build produces expected compile errors (42 const-correctness issues)

## Task Commits

Each task was committed atomically:

1. **Task 1: Add LanguageStandard to Debug|Win32** - `ed8b3c1` (feat)
2. **Task 2: Add LanguageStandard to Debug|x64** - `68b1d45` (feat)
3. **Task 3: Add LanguageStandard to Release|Win32** - `db4cfd7` (feat)
4. **Task 4: Add LanguageStandard to Release|x64** - `291993d` (feat)

**Task 5: Checkpoint (human-verify)** - User approved build result

## Files Created/Modified

- `EIDCardLibrary/EIDCardLibrary.vcxproj` - Added C++23 language standard to all 4 configurations

## Decisions Made

- **stdcpp23 vs stdcpp23preview:** Visual Studio 2026 uses the enum value `stdcpp23` to represent the `/std:c++23preview` compiler flag. This is the correct value for the current toolset.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Adjusted LanguageStandard enum value**
- **Found during:** Task 1 (Debug|Win32 configuration)
- **Issue:** Plan specified `stdcpp23preview` as the enum value, but VS 2026 MSBuild uses `stdcpp23` for the `/std:c++23preview` compiler flag
- **Fix:** Used `stdcpp23` enum value instead, which maps to `/std:c++23preview` in the compiler
- **Files modified:** EIDCardLibrary/EIDCardLibrary.vcxproj
- **Verification:** Build attempt confirmed C++23 flag is active (compile errors from stricter conformance)
- **Committed in:** ed8b3c1 (Task 1 commit, applied consistently to all 4 configurations)

---

**Total deviations:** 1 auto-fixed (blocking issue - enum value mismatch)
**Impact on plan:** Minimal - the effective compiler behavior is identical. The enum value difference is a VS 2026 implementation detail.

## Issues Encountered

### Expected Compile Errors (42 total)

The build produced 42 compile errors in EIDCardLibrary, all related to const-correctness. This is expected behavior:

- **Cause:** The C++23 flag enables `/Zc:strictStrings` by default, enforcing stricter string literal conversion rules
- **Examples:** Assigning string literals to non-const char* pointers, passing string literals to functions expecting non-const parameters
- **Resolution:** These will be fixed in subsequent plans (01-02 through 01-04) as part of the incremental modernization approach
- **Locked Decision:** "Incremental modernization - fix compile errors first, then refactor" from PROJECT.md

This is the expected outcome per the research phase and locked decisions. The compile errors identify all locations requiring const-correctness fixes.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- EIDCardLibrary.vcxproj is configured for C++23
- 42 compile errors identified for const-correctness fixes
- Pattern established for applying LanguageStandard to remaining projects
- Ready for plan 01-02 (apply C++23 flag to remaining projects and begin error fixes)

---
*Phase: 01-build-system*
*Completed: 2026-02-15*

## Self-Check: PASSED

- 01-01-SUMMARY.md: FOUND
- Final commit 9a72ef4: FOUND
- LanguageStandard elements: 4 (verified in EIDCardLibrary.vcxproj)
- All task commits (ed8b3c1, 68b1d45, db4cfd7, 291993d): VERIFIED
