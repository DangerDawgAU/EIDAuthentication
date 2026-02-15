---
phase: 03-compile-time-enhancements
plan: 03b
subsystem: compile-time
tags: [std::unreachable, exhaustive-switch, optimization, LSASS, security]

# Dependency graph
requires:
  - phase: 03-01
    provides: <utility> header with std::unreachable
provides:
  - Documented analysis of switch statements for std::unreachable applicability
  - Safety rationale for NOT using std::unreachable in security-critical code
affects: []

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Conservative approach to std::unreachable: prefer safe defaults over optimization hints"
    - "Security-critical code validates all inputs including enum values"

key-files:
  created: []
  modified: []

key-decisions:
  - "No std::unreachable() applied - all switch statements have reachable default cases due to security requirements"
  - "External data sources (registry, caller parameters) can contain invalid enum values"
  - "Default cases performing logging and error returns are essential defensive programming"

patterns-established:
  - "Pattern: Exhaustive switch with defensive default case - preferred over std::unreachable for LSASS code"
  - "Pattern: All enum-based switches in security code must handle invalid values safely"

# Metrics
duration: 4min
completed: 2026-02-15
---

# Phase 03 Plan 03b: std::unreachable() Analysis Summary

**Analyzed all switch statements in target files and determined std::unreachable() is NOT applicable due to security requirements.**

## Performance

- **Duration:** 4 min
- **Started:** 2026-02-15T08:43:40Z
- **Completed:** 2026-02-15T08:47:17Z
- **Tasks:** 3 (analysis and documentation only)
- **Files modified:** 0

## Accomplishments

- Comprehensive analysis of all switch statements in EIDCardLibrary.h, StoredCredentialManagement.cpp, and CompleteToken.cpp
- Documented safety rationale for NOT using std::unreachable() in security-critical LSASS code
- Verified that existing defensive programming patterns (default cases with error handling) are appropriate

## Task Commits

No code commits were required - analysis determined std::unreachable() is not applicable:

1. **Task 1: Identify exhaustive switch statements** - Analysis only (no code changes)
2. **Task 2: Apply std::unreachable()** - Not applicable (no safe candidates found)
3. **Task 3: Verify build** - Skipped (no code changes to verify)

**Plan metadata:** This summary documents the decision.

## Files Analyzed

| File | Switch Count | std::unreachable Safe? | Rationale |
|------|-------------|----------------------|-----------|
| EIDCardLibrary.h | 0 | N/A | No switch statements |
| StoredCredentialManagement.cpp | 3 | NO | External data sources, security-sensitive |
| CompleteToken.cpp | 1 | NO | Error handling code, NET_API_STATUS |

## Switch Statement Analysis

### StoredCredentialManagement.cpp

**Line 675 - switch(*pType) on EID_PRIVATE_DATA_TYPE:**
- Enum values: `eidpdtClearText(1)`, `eidpdtCrypted(2)`, `eidpdtDPAPI(3)`
- All 3 cases handled explicitly
- Default case: logs warning, triggers `__leave` for cleanup
- **UNSAFE for std::unreachable**: Data read from registry could be corrupted/tampered

**Line 953 - switch(dwChallengeType) on EID_PRIVATE_DATA_TYPE:**
- Only handles `eidpdtClearText` and `eidpdtCrypted`
- **Missing `eidpdtDPAPI`** - NOT exhaustive
- **UNSAFE for std::unreachable**: Not all enum values handled

**Line 1402 - switch(dwChallengeType) on EID_PRIVATE_DATA_TYPE:**
- All 3 cases handled: `eidpdtClearText`, `eidpdtCrypted`, `eidpdtDPAPI`
- Default case: logs warning, returns FALSE
- **UNSAFE for std::unreachable**: Parameter comes from caller, could be invalid

### CompleteToken.cpp

**Line 350 - switch(netStatus) on NET_API_STATUS:**
- Error handling code for `NetUserGetInfo()` result
- Handles `ERROR_ACCESS_DENIED`, `NERR_InvalidComputer`, `NERR_UserNotFound`
- Default case handles all other Windows error codes
- **UNSAFE for std::unreachable**: Error handling paths are inherently reachable

### CredentialManagement.cpp (adjacent analysis)

**Lines 252, 267, 286, 305 - switch(_State) on EID_MESSAGE_STATE:**
- State machine switches for security context operations
- Deliberately NOT exhaustive - only expected states handled per context
- Default cases return error status (STATUS_INVALID_SIGNATURE)
- **UNSAFE for std::unreachable**: State machines can receive unexpected states

## Decisions Made

1. **No std::unreachable() applied**: All analyzed switch statements have legitimate reasons to reach their default cases
2. **Security over optimization**: In LSASS code, defensive programming is more valuable than optimization hints
3. **External data is untrusted**: Registry values and caller parameters can contain corrupted or malicious data

## Deviations from Plan

None - plan was executed exactly as written. The plan explicitly anticipated this outcome:
- "Note if no safe candidates exist"
- "Conservative approach: safe defaults preferred over optimization"
- "Remember: std::unreachable() triggers undefined behavior if executed"

## Key Findings

### Why std::unreachable() is NOT Applicable Here

1. **Data provenance**: Values being switched come from:
   - Windows registry (can be corrupted or tampered)
   - Caller parameters (untrusted input)
   - Network API results (error codes vary by context)
   - State machine state (can transition unexpectedly)

2. **Security context**: This code runs in LSASS (Local Security Authority Subsystem Service):
   - Must handle all inputs defensively
   - Undefined behavior from std::unreachable() could crash the authentication system
   - Default cases with logging provide valuable debugging information

3. **Error handling**: Several switches are explicitly for error handling:
   - Error handling code MUST be reachable
   - Adding std::unreachable() to error paths defeats their purpose

### When std::unreachable() WOULD Be Safe

std::unreachable() would only be safe if ALL these conditions are met:
1. Switch variable is a scoped enum (enum class)
2. ALL enum values have explicit case labels
3. Switch variable cannot be cast from integer or external source
4. Code is NOT security-critical
5. Code is NOT in an error-handling path
6. There is absolute certainty the default case is unreachable

None of the analyzed switches meet all these criteria.

## Next Phase Readiness

- Phase 3 compile-time enhancements can continue to next plan
- No technical debt introduced
- Defensive programming patterns are preserved
- <utility> header from 03-01 remains available for future use

---
*Phase: 03-compile-time-enhancements*
*Plan: 03b*
*Completed: 2026-02-15*
