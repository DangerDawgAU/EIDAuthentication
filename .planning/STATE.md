# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-02-15)

**Core value:** Successfully compile the entire codebase with `/std:c++23` and leverage modern C++23 features to improve code quality, safety, and maintainability without breaking existing functionality.
**Current focus:** Phase 3: Compile-Time Enhancements

## Current Position

Phase: 3 of 6 (Compile-Time Enhancements)
Plan: 2 of 4 in current phase
Status: Plan 03-02 Complete - Added constexpr+noexcept validation functions with static_assert
Last activity: 2026-02-15 - Completed 03-02 plan (constexpr validation functions)

Progress: [===============-] 75%

## Performance Metrics

**Velocity:**
- Total plans completed: 11
- Average duration: 10 min
- Total execution time: 1.9 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 1. Build System | 3 | 3 | 12 min |
| 2. Error Handling | 3 | 4 | 15 min |
| 2.1. C++23 Conformance | 1 | 1 | 12 min |
| 2.2. Const-Correctness | 3 | 3 | 9 min |
| 3. Compile-Time | 2 | 4 | 7 min |
| 4. Code Quality | 0 | 4 | - |
| 5. Documentation | 0 | 2 | - |
| 6. Verification | 0 | 4 | - |

**Recent Trend:**
- Last 5 plans: 6 min, 12 min, 8 min, 4 min, 4 min
- Trend: Improving

*Updated after each plan completion*
| Phase 03-02 | 10 min | 4 tasks | 3 files |

## Accumulated Context

### Decisions

Decisions are logged in PROJECT.md Key Decisions table.
Recent decisions affecting current work:

- [Init]: Target C++23 with `/std:c++23preview` flag (stable flag not yet available)
- [Init]: Use v143 toolset to maintain Windows 7+ compatibility
- [Init]: Incremental modernization - fix compile errors first, then refactor
- [01-01]: Used stdcpp23 enum value (VS 2026 uses this for /std:c++23preview)
- [01-01]: 42 compile errors expected - const-correctness issues from /Zc:strictStrings
- [01-02]: Applied stdcpp23 pattern consistently across all 6 dependent projects
- [01-02]: Deferred linking errors - dependent projects cannot build until EIDCardLibrary compile errors are fixed
- [01-03]: Phase 1 Goal ACHIEVED - all 7 projects consistently configured for C++23
- [01-03]: 23 compile errors documented for Phase 2 resolution
- [01-03]: No new C++23-specific warnings introduced
- [02-01a]: Use static char arrays for OID string literals to fix LPSTR const-correctness
- [02-01b]: Prefer const-correct function signatures over static buffers when function only reads string data
- [02-01b]: Use static arrays only when Windows API requires non-const pointers
- [Phase 02-error-handling]: Use std::expected<T, HRESULT> instead of custom Result<T> type
- [Phase 02-error-handling]: All error handling functions must be noexcept for LSASS compatibility
- [02.1-01]: Member declarations inside class definitions must not use class:: qualifier (C++23 /permissive-)
- [02.1-01]: Template dependent types like std::list<T*>::iterator require typename keyword
- [02.2-02b]: Use static TCHAR arrays for PTSTR compatibility with Windows ListView APIs
- [02.2-02b]: Use static wchar_t arrays for LPTSTR compatibility with Windows Tooltip APIs
- [02.2-02a]: Use static char/wchar_t arrays for LSA_STRING and ExportOneTraceFile string parameters
- [02.2-01]: Use static wchar_t arrays for LPWSTR struct members (CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR.pszLabel)
- [02.2-01]: Removing const from array declaration does NOT fix C2440 - static buffers required
- [02.2-gap]: CEIDCredential.cpp line 649 (PWSTR* assignment) fixed with same static buffer pattern as other C2440 errors
- [03-01]: Include <utility> in headers containing enum definitions to enable std::to_underlying() for future use
- [03-03a]: if consteval not applicable to codebase - no functions have different compile-time vs runtime code paths
- [03-03b]: No std::unreachable() in security-critical code - external data sources (registry, caller params) can contain invalid values
- [03-02]: constexpr validation functions also marked noexcept for LSASS compatibility

### Roadmap Evolution

- Phase 02.1 inserted after Phase 2: Fix C++23 conformance errors (URGENT)
- Phase 02.2 inserted after Phase 2.1: Fix const-correctness in dependent projects (URGENT)

### Pending Todos

None yet.

### Blockers/Concerns

- All 23 const-correctness compile errors in EIDCardLibrary fixed (12 in 02-01a, 11 in 02-01b)
- All 7 C++23 conformance errors fixed (3 C4596, 3 C7510, 1 C3861) in 02.1-01
- All 14 C2440 const-correctness errors in dependent projects fixed (02.2-01, 02.2-02a, 02.2-02b + gap closure)
- EIDCardLibrary builds successfully with C++23
- EIDCredentialProvider builds successfully with C++23 (all const-correctness errors fixed)
- EIDConfigurationWizard builds successfully with C++23 (all const-correctness errors fixed)
- Missing cardmod.h header prevents full solution rebuild (Windows SDK dependency - requires Smart Card Credential Provider SDK)
- Result<T> error handling infrastructure ready for 02-03 (API boundary conversion layer)

## Session Continuity

Last session: 2026-02-15
Stopped at: Completed 03-02 plan (constexpr validation functions)
Resume file: .planning/phases/03-compile-time-enhancements/03-02-SUMMARY.md

---

## Key Constraints (from research)

- **LSASS context:** No exceptions, no console I/O, static CRT required
- **Windows 7 support:** Must use v143 toolset (v145 drops Win7)
- **C++23 flag:** Use `/std:c++23preview` (stable `/std:c++23` not yet available)
- **C-style APIs:** Preserve HRESULT/BOOL at exported boundaries
