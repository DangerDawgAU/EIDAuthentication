---
phase: 28-diagnostics-logging
plan: 01
subsystem: diagnostics
tags: [etw, tracing, stack-trace, error-logging, lsass-safe]

# Dependency graph
requires:
  - phase: 27-cpp23-advanced-features
    provides: CaptureStackBackTrace recommendation (over std::stacktrace)
provides:
  - EIDLogErrorWithContext macro for structured error logging with operation context
  - EIDLogStackTrace macro for LSASS-safe stack trace capture
  - Foundation helpers for DIAG-01 (enhanced error messages) and DIAG-03 (structured logging)
affects: [28-02, 28-03, 28-04, 28-05]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Structured logging prefixes ([ERROR_CONTEXT], [STACK_TRACE]) for filtering
    - Stack-allocated buffers only for LSASS safety
    - CaptureStackBackTrace Win32 API for stack traces (not std::stacktrace)

key-files:
  created: []
  modified:
    - EIDCardLibrary/Tracing.h
    - EIDCardLibrary/Tracing.cpp

key-decisions:
  - "Use CaptureStackBackTrace (not std::stacktrace) per Phase 27 finding - std::stacktrace not reliable in LSASS context"
  - "Stack-allocated buffers only (no std::string, new, malloc) for LSASS safety"
  - "Structured prefixes [ERROR_CONTEXT] and [STACK_TRACE] enable filtering in security monitoring tools"

patterns-established:
  - "Pattern: Diagnostic helpers use stack-allocated WCHAR buffers with swprintf_s for formatting"
  - "Pattern: Error logging includes operation name and HRESULT for troubleshooting"
  - "Pattern: Stack traces logged at ERROR level header, VERBOSE level for individual frames"

requirements-completed:
  - DIAG-01
  - DIAG-03

# Metrics
duration: 3min
completed: 2026-02-18
---

# Phase 28 Plan 01: Diagnostic Helper Functions Summary

**Enhanced error logging helper (EIDLogErrorWithContext) and LSASS-safe stack trace capture (EIDLogStackTrace) using CaptureStackBackTrace Win32 API**

## Performance

- **Duration:** 3 min
- **Started:** 2026-02-17T14:43:59Z
- **Completed:** 2026-02-17T14:47:56Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments
- Added EIDLogErrorWithContext macro and EIDLogErrorWithContextEx function for structured error logging with operation context and HRESULT formatting
- Added EIDLogStackTrace macro and EIDLogStackTraceEx function for LSASS-safe stack trace capture using CaptureStackBackTrace
- All functions use stack-allocated buffers only (no dynamic memory) for LSASS safety
- Build passes with no new warnings (only pre-existing Windows SDK header warnings)

## Task Commits

Each task was committed atomically:

1. **Task 1: Add EIDLogErrorWithContext helper function declarations to Tracing.h** - `755086b` (feat)
2. **Task 2: Implement EIDLogErrorWithContext and EIDLogStackTrace in Tracing.cpp** - `dba861f` (feat)

**Plan metadata:** (pending final commit)

## Files Created/Modified
- `EIDCardLibrary/Tracing.h` - Added EIDLogErrorWithContext/EIDLogErrorWithContextEx and EIDLogStackTrace/EIDLogStackTraceEx declarations
- `EIDCardLibrary/Tracing.cpp` - Implemented EIDLogErrorWithContextEx (lines 461-514) and EIDLogStackTraceEx (lines 519-569) using stack-allocated buffers

## Decisions Made
- Use CaptureStackBackTrace (not std::stacktrace) per Phase 27 finding - std::stacktrace not implemented in MSVC and not reliable in LSASS context
- Stack-allocated buffers only (no std::string, new, malloc) for LSASS safety
- Structured prefixes [ERROR_CONTEXT] and [STACK_TRACE] enable filtering in security monitoring tools
- Stack trace header logged at ERROR level, individual frames at VERBOSE level

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None - build succeeded on first attempt.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Foundation diagnostic helpers complete and ready for use
- Subsequent plans (28-02 through 28-05) can now use EIDLogErrorWithContext and EIDLogStackTrace
- No blockers

## Self-Check: PASSED

- [x] EIDLogErrorWithContext macro and EIDLogErrorWithContextEx function exist in Tracing.h
- [x] EIDLogStackTrace macro and EIDLogStackTraceEx function exist in Tracing.h
- [x] EIDLogErrorWithContextEx implemented in Tracing.cpp (line 461)
- [x] EIDLogStackTraceEx implemented in Tracing.cpp (line 519)
- [x] CaptureStackBackTrace used (line 549)
- [x] Tracing.cpp is 568 lines (minimum required: 460)
- [x] Commits exist: 755086b, dba861f

---
*Phase: 28-diagnostics-logging*
*Completed: 2026-02-18*
