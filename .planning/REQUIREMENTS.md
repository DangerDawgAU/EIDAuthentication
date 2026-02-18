# Requirements: EIDAuthentication C++23 Modernization

**Defined:** 2026-02-15
**Core Value:** A clean, maintainable, and secure codebase with zero static analysis issues, leveraging modern C++23 features while preserving all existing authentication functionality.

## v1.4 Requirements (CURRENT)

SonarQube Zero — Eliminate all fixable SonarQube issues and document remaining won't-fix with justifications.

### Macro Modernization

- [x] **MACRO-01**: Simple value macros converted to constexpr where safe
- [x] **MACRO-02**: Resource compiler macros documented as won't-fix (cannot convert .rc macros)
- [x] **MACRO-03**: Flow-control macros documented as won't-fix

### Auto Modernization

- [x] **AUTO-01**: Redundant type declarations converted to auto where type is obvious
- [x] **AUTO-02**: Security-critical types (HRESULT, NTSTATUS, handles) kept explicit

### Const Correctness

- [x] **CONST-01**: Global variables marked const where truly immutable (all const-eligible already marked)
- [x] **CONST-02**: Runtime-assigned globals documented as won't-fix (LSA pointers, DLL state, tracing state, UI state, file handles, SAM function pointers)
- [ ] **CONST-03**: Member functions marked const where state is not modified
- [ ] **CONST-04**: COM/interface methods documented as won't-fix (cannot change signatures)

### Code Structure

- [ ] **STRUCT-01**: Cognitive complexity reduced via helper function extraction
- [ ] **STRUCT-02**: SEH-protected code documented as won't-fix for complexity
- [ ] **STRUCT-03**: Deep nesting reduced via early return/guard clauses
- [ ] **STRUCT-04**: SEH blocks kept intact (no extraction from __try)

### Modern C++ Patterns

- [ ] **MODERN-01**: Init-statements used in if/switch where scope benefits
- [ ] **MODERN-02**: C-style casts replaced with named casts (static_cast, etc.)
- [ ] **MODERN-03**: C-style arrays converted to std::array where stack size permits
- [ ] **MODERN-04**: std::string/std::vector documented as won't-fix (LSASS safety)
- [ ] **MODERN-05**: Enum types converted to enum class where safe
- [ ] **MODERN-06**: Windows API enum types documented as won't-fix

### Verification

- [ ] **VER-01**: All 7 projects build with zero errors
- [ ] **VER-02**: No new warnings introduced
- [ ] **VER-03**: SonarQube scan shows improvement from baseline
- [ ] **VER-04**: Won't-fix issues documented with justifications

## v1.3 Requirements (COMPLETE)

Deep Modernization — Continue v1.2 work with more aggressive refactoring.

### SonarQube Resolution

- [x] **SONAR-01**: Review and resolve style preference issues (~124 "replace with auto")
- [x] **SONAR-02**: Review and resolve macro issues (~111 "replace with const/constexpr")
- [x] **SONAR-03**: Review and resolve global variable const issues (~63 remaining)
- [x] **SONAR-04**: Review and resolve nesting depth issues (~52 deep nesting)

### Code Refactoring

- [x] **REFACT-01**: Reduce cognitive complexity in key functions (~24 high complexity)
- [x] **REFACT-02**: Extract helper functions from deeply nested code blocks
- [x] **REFACT-03**: Consolidate duplicate code patterns (17 duplication blocks)

### C++23 Advanced Features

- [x] **CPP23-01**: Evaluate `import std;` modules feasibility — Deferred
- [x] **CPP23-02**: Apply `std::flat_map` / `std::flat_set` where applicable — Deferred
- [x] **CPP23-03**: Modernize with `std::stacktrace` (if MSVC bugs fixed) — Won't Fix

### Diagnostics & Logging

- [x] **DIAG-01**: Enhance error messages with more context
- [x] **DIAG-02**: Improve tracing coverage for key code paths
- [x] **DIAG-03**: Add structured logging where feasible

### Verification

- [x] **VER-01**: Build verification after all changes
- [x] **VER-02**: Final SonarQube scan showing improvement

## v1 Requirements (COMPLETE)

Requirements for C++23 modernization milestone. All phases executed.

### Build System

- [x] **BUILD-01**: All 7 Visual Studio projects updated with `/std:c++23preview` flag
- [x] **BUILD-02**: Code compiles cleanly with C++23 (no errors, no new warnings)
- [x] **BUILD-03**: v143 toolset maintained for Windows 7+ compatibility
- [x] **BUILD-04**: Static CRT (`/MT`) preserved for LSASS isolation

### Error Handling

- [x] **ERROR-01**: `std::expected<T, E>` adopted for internal error handling
- [x] **ERROR-02**: C-style API boundaries preserved (HRESULT, BOOL at exports)
- [x] **ERROR-03**: All new error-handling code marked `noexcept`

### Compile-Time Enhancements

- [x] **COMPILE-01**: `if consteval` evaluated — NOT APPLICABLE (no dual-path functions)
- [x] **COMPILE-02**: `constexpr` extended where beneficial for compile-time validation
- [x] **COMPILE-03**: `std::to_underlying` enabled via `<utility>` headers
- [x] **COMPILE-04**: `std::unreachable()` evaluated — NOT APPLICABLE (defensive coding required)

### Code Quality

- [x] **QUAL-01**: `std::format` replaces `swprintf_s` in non-LSASS code (EIDConfigurationWizard)
- [x] **QUAL-02**: NOT APPLICABLE — No CRTP patterns in codebase
- [x] **QUAL-03**: `std::span` used for buffer handling at internal boundaries
- [x] **QUAL-04**: Const-correctness improved across codebase

### Documentation

- [x] **DOC-01**: README.md updated to reflect C++23 requirement
- [x] **DOC-02**: Build instructions updated with MSVC version requirements

### Verification

- [x] **VERIFY-01**: Build verification passed — all 7 projects compile
- [ ] **VERIFY-02**: Runtime verification — smart card login functional (pending test machines)
- [ ] **VERIFY-03**: Runtime verification — LSA Authentication Package loads (pending test machines)
- [ ] **VERIFY-04**: Runtime verification — Credential Provider appears on login screen (pending test machines)
- [ ] **VERIFY-05**: Runtime verification — Configuration Wizard operational (pending test machines)

## v1.1 Requirements (COMPLETE)

SonarQube Quality Remediation — Security hotspots and reliability bugs resolved.

### Security & Reliability (Critical) — COMPLETE

- [x] **SEC-01**: Resolve 2 security hotspots (strlen safety) — Fixed
- [x] **SEC-02**: Fix 3 reliability bugs (type punning, dead code) — Fixed

## v1.2 Requirements (COMPLETE)

Code Modernization — ~55 SonarQube issues fixed, ~1,000 documented as Won't Fix.

- [x] **CRIT-01**: Fix unannotated fall-through in switch statement
- [x] **CONST-01**: Global variables marked const (partial)
- [x] **CONST-02**: Global pointers have const (partial)
- [x] **CONST-03**: Member functions marked const (11 methods)
- [x] **TYPE-03**: Variable shadowing issues resolved (9 issues)
- [x] **QUAL-01**: Unused variables removed (18 issues)
- [x] **QUAL-02**: Build passes after all code changes
- [x] **DOC-01**: ~55 issues fixed, ~1,000 documented with Won't Fix justifications
- [x] **DOC-02**: VERIFICATION.md created with detailed justifications

## Out of Scope

Explicitly excluded. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| `std::print` / `std::println` | No console in LSASS; UTF-8 buffer bugs exist |
| Exceptions | LSASS stability requires exception-free code |
| Coroutines | Stack complexity unacceptable in LSASS context |
| Dynamic CRT (`/MD`) | LSASS isolation requires static runtime |
| v145 toolset | Drops Windows 7/8/8.1 support |
| x86 (32-bit) builds | x64 only per project constraints |
| Domain-joined support | Local accounts only per original scope |
| New authentication features | Modernization only, no new capabilities |
| Code signing | Separate concern, out of scope |
| std::string/std::vector | Heap allocation unsafe in LSASS context |

## Traceability

### v1.4 Requirements (Current Milestone)

| Requirement | Phase | Status |
|-------------|-------|--------|
| MACRO-01 | Phase 31 | Complete |
| MACRO-02 | Phase 31 | Complete |
| MACRO-03 | Phase 31 | Complete |
| AUTO-01 | Phase 32 | Complete |
| AUTO-02 | Phase 32 | Complete |
| CONST-01 | Phase 34 | Complete |
| CONST-02 | Phase 34 | Complete |
| CONST-03 | Phase 35 | Pending |
| CONST-04 | Phase 35 | Pending |
| STRUCT-01 | Phase 36 | Pending |
| STRUCT-02 | Phase 36 | Pending |
| STRUCT-03 | Phase 37 | Pending |
| STRUCT-04 | Phase 37 | Pending |
| MODERN-01 | Phase 38 | Pending |
| MODERN-02 | Phase 33 | Complete |
| MODERN-03 | Phase 39 | Pending |
| MODERN-04 | Phase 39 | Pending |
| MODERN-05 | Phase 33 | Complete |
| MODERN-06 | Phase 33 | Complete |
| VER-01 | Phase 40 | Pending |
| VER-02 | Phase 40 | Pending |
| VER-03 | Phase 40 | Pending |
| VER-04 | Phase 40 | Pending |

**Coverage:**
- v1.4 requirements: 23 total
- Mapped to phases: 23
- Unmapped: 0 ✓

---

*Requirements defined: 2026-02-15*
*Last updated: 2026-02-18 after v1.4 milestone start*
