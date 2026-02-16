---
phase: 02-error-handling
plan: 01b
subsystem: build-system
tags: [cpp23, const-correctness, strictStrings, windows-api, static-buffers]

# Dependency graph
requires:
  - phase: 01-build-system
    provides: C++23 configuration in all projects
provides:
  - Fixed const-correctness errors in CompleteToken.cpp, Registration.cpp, smartcardmodule.cpp, TraceExport.cpp
  - Static buffer pattern for string literal to non-const pointer conversions
  - reinterpret_cast pattern for function pointer casts
affects: [02-02, 02-03]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Static wchar_t/WCHAR/TCHAR arrays for string literals passed to non-const Windows API parameters
    - const WCHAR* for function parameters that only read string data
    - reinterpret_cast for function pointer conversions

key-files:
  created: []
  modified:
    - EIDCardLibrary/CompleteToken.cpp
    - EIDCardLibrary/Registration.cpp
    - EIDCardLibrary/smartcardmodule.cpp
    - EIDCardLibrary/TraceExport.cpp

key-decisions:
  - "Prefer const-correct function signatures over static buffers when the function only reads the string"
  - "Use static arrays only when Windows API requires non-const pointers"

patterns-established:
  - "Pattern 1: Change function parameter from WCHAR*/PTSTR to const WCHAR*/LPCTSTR when function only reads the string"
  - "Pattern 2: Use static WCHAR/wchar_t/TCHAR arrays for string literals passed to Windows APIs requiring non-const pointers"
  - "Pattern 3: Use reinterpret_cast<TargetType> instead of C-style casts for function pointer conversions"

# Metrics
duration: 25min
completed: 2026-02-15
---

# Phase 02 Plan 01b: Const-Correctness Fixes (Non-Certificate Files) Summary

**Fixed 11 const-correctness compile errors in CompleteToken.cpp, Registration.cpp, smartcardmodule.cpp, and TraceExport.cpp using const-correct signatures and static buffer patterns for C++23 /Zc:strictStrings compatibility.**

## Performance

- **Duration:** 25 min
- **Started:** 2026-02-15T05:59:40Z
- **Completed:** 2026-02-15T06:24:52Z
- **Tasks:** 4
- **Files modified:** 4

## Accomplishments
- Fixed DebugPrintSid function signature to accept const WCHAR* (enables string literal arguments)
- Changed AppendValueToMultiSz and RemoveValueFromMultiSz helper functions to accept LPCTSTR parameters
- Created static buffers for Windows API calls requiring non-const strings (AddSecurityPackage, DeleteSecurityPackage, card module APIs)
- Replaced C-style function pointer cast with reinterpret_cast in TraceExport.cpp

## Task Commits

Each task was committed atomically:

1. **Task 1: Fix CompleteToken.cpp const-correctness** - `033e171` (fix)
2. **Task 2: Fix Registration.cpp const-correctness** - `a8155fe` (fix)
3. **Task 3: Fix smartcardmodule.cpp const-correctness** - `349b034` (fix)
4. **Task 4: Fix TraceExport.cpp const-correctness** - `8232d28` (fix)

**Plan metadata:** (pending final commit)

## Files Created/Modified
- `EIDCardLibrary/CompleteToken.cpp` - Changed DebugPrintSid parameter from WCHAR* to const WCHAR*
- `EIDCardLibrary/Registration.cpp` - Changed helper function parameters to LPCTSTR, added static buffer for AddSecurityPackage/DeleteSecurityPackage
- `EIDCardLibrary/smartcardmodule.cpp` - Added static buffer s_wszCardUserUser for card module API calls
- `EIDCardLibrary/TraceExport.cpp` - Added static buffer for LoggerName, changed function pointer cast to reinterpret_cast

## Decisions Made
- **Prefer const-correct signatures over static buffers:** When a function only reads string data, changing the parameter type to const is cleaner than creating static buffers for every call site
- **Use static arrays for Windows API compatibility:** When the API signature requires non-const (like AddSecurityPackageW, MgScCardAuthenticatePin), use static arrays to provide writable storage

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Fixed DebugPrintSid signature instead of using static buffer**
- **Found during:** Task 1 (CompleteToken.cpp const-correctness)
- **Issue:** Plan suggested using static WCHAR array, but changing the function signature to const WCHAR* is cleaner and benefits all callers
- **Fix:** Changed DebugPrintSid(WCHAR* Name, ...) to DebugPrintSid(const WCHAR* Name, ...)
- **Files modified:** EIDCardLibrary/CompleteToken.cpp
- **Verification:** Build passes, line 158 now accepts string literal L"PrimaryGroupId"
- **Committed in:** 033e171

**2. [Rule 1 - Bug] Fixed helper function signatures instead of using static buffers**
- **Found during:** Task 2 (Registration.cpp const-correctness)
- **Issue:** Plan suggested using static wchar_t arrays for RegSetKeyValue, but the actual errors were from AppendValueToMultiSz/RemoveValueFromMultiSz helper functions
- **Fix:** Changed helper function parameters from PTSTR to LPCTSTR for szKey, szValue, szData
- **Files modified:** EIDCardLibrary/Registration.cpp
- **Verification:** Build passes, all registry path string literals now accepted
- **Committed in:** a8155fe

**3. [Rule 3 - Blocking] Build required solution file instead of project file**
- **Found during:** Task 3 (smartcardmodule.cpp const-correctness)
- **Issue:** Building the vcxproj directly didn't resolve $(SolutionDir), causing cardmod.h include failure
- **Fix:** Built via solution file (.sln) to correctly resolve $(SolutionDir)include\ path
- **Verification:** cardmod.h found, const-correctness errors appeared as expected
- **Committed in:** N/A (process fix)

---

**Total deviations:** 3 (2 bug fixes, 1 blocking issue)
**Impact on plan:** All fixes improved code quality. Const-correct function signatures are more maintainable than static buffers at every call site.

## Issues Encountered
- **cardmod.h include path:** Building project directly didn't resolve $(SolutionDir). Workaround: build via solution file
- **Plan line numbers didn't match actual errors:** Errors were in helper function calls rather than RegSetKeyValue calls as documented. Fixed by analyzing actual build output.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- 11 const-correctness errors fixed in this plan (02-01b)
- Combined with 02-01a (12 errors in certificate files), all 23 const-correctness errors will be resolved
- Ready for 02-02 (define error types and Result<T> patterns) once 02-01a is complete

---
*Phase: 02-error-handling*
*Completed: 2026-02-15*

## Self-Check: PASSED

**Files verified:**
- EIDCardLibrary/CompleteToken.cpp: EXISTS
- EIDCardLibrary/Registration.cpp: EXISTS
- EIDCardLibrary/smartcardmodule.cpp: EXISTS
- EIDCardLibrary/TraceExport.cpp: EXISTS

**Commits verified:**
- 033e171: FOUND
- a8155fe: FOUND
- 349b034: FOUND
- 8232d28: FOUND
- af1915e: FOUND
