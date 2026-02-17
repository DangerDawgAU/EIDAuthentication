---
phase: 27-cpp23-advanced-features
plan: 01
subsystem: documentation
tags: [cpp23, modules, flat-containers, stacktrace, msvc, deferral]

# Dependency graph
requires:
  - phase: 27-cpp23-advanced-features
    provides: Research findings on C++23 feature feasibility
provides:
  - Formal documentation of deferred C++23 features with justification
  - Phase 28 recommendations for alternative approaches
  - Re-evaluation triggers for future consideration
affects: [28-diagnostics-improvements, future-cpp23-adoption]

# Tech tracking
tech-stack:
  added: []
  patterns: []

key-files:
  created:
    - .planning/phases/27-cpp23-advanced-features/27-DEFERRAL.md
  modified: []

key-decisions:
  - "CPP23-01 (import std;): Won't-fix - Partial MSVC support, immature tooling, CMake experimental - Defer to future milestone"
  - "CPP23-02 (std::flat_map/flat_set): Won't-fix - NOT IMPLEMENTED in MSVC (only GCC 15+/Clang 20+) - Defer until MSVC implements"
  - "CPP23-03 (std::stacktrace): Won't-fix - Partial/buggy in MSVC 19.34+ - Use CaptureStackBackTrace Win32 API in Phase 28"

patterns-established: []

requirements-completed:
  - CPP23-01
  - CPP23-02
  - CPP23-03

# Metrics
duration: 2min
completed: 2026-02-18
---

# Phase 27 Plan 01: C++23 Advanced Features Deferral Summary

**Documented deferral decisions for all three C++23 advanced features (modules, flat containers, stacktrace) with MSVC implementation status, justification, and Phase 28 alternative approaches.**

## Performance

- **Duration:** ~2 min
- **Started:** 2026-02-17T14:18:43Z
- **Completed:** 2026-02-17T14:20:20Z
- **Tasks:** 1
- **Files modified:** 1

## Accomplishments
- Created comprehensive deferral documentation with structured justification for all three C++23 features
- Documented MSVC implementation status for each feature with cppreference verification
- Provided Phase 28 recommendations enabling diagnostics work to proceed with alternatives
- Established re-evaluation triggers for future C++23 feature adoption

## Task Commits

Each task was committed atomically:

1. **Task 1: Create C++23 Deferral Documentation** - `3ee756d` (docs)

**Plan metadata:** (pending final commit)

## Files Created/Modified
- `.planning/phases/27-cpp23-advanced-features/27-DEFERRAL.md` - Formal deferral documentation with requirements CPP23-01, CPP23-02, CPP23-03

## Decisions Made
- **CPP23-01 (modules):** Won't-fix due to partial MSVC support and experimental CMake integration - defer to future milestone
- **CPP23-02 (flat containers):** Won't-fix as feature not implemented in MSVC - defer until Microsoft provides implementation
- **CPP23-03 (std::stacktrace):** Won't-fix due to buggy MSVC implementation - Phase 28 will use CaptureStackBackTrace Win32 API instead

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None - documentation-only task with clear research context.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Phase 28 (Diagnostics Improvements) can proceed immediately
- CaptureStackBackTrace recommended for stack trace implementation
- OutputDebugString/ETW recommended for structured logging
- All alternatives documented in 27-DEFERRAL.md Section 3

## Self-Check: PASSED

- [x] 27-DEFERRAL.md exists at expected path
- [x] File contains all three requirement IDs (CPP23-01, CPP23-02, CPP23-03)
- [x] File contains "Won't-fix" or "deferred" decision for all three
- [x] File contains "CaptureStackBackTrace" as alternative for CPP23-03
- [x] Task commit exists (3ee756d)

---
*Phase: 27-cpp23-advanced-features*
*Completed: 2026-02-18*
