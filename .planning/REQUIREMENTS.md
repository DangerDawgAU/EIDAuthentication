# Requirements: EIDAuthentication C++23 Modernization

**Defined:** 2026-02-15
**Core Value:** A clean, maintainable, and secure codebase with zero static analysis issues, leveraging modern C++23 features while preserving all existing authentication functionality.

## v1.3 Requirements (CURRENT)

Deep Modernization — Continue v1.2 work with more aggressive refactoring.

### SonarQube Resolution

- [ ] **SONAR-01**: Review and resolve style preference issues (~124 "replace with auto")
- [ ] **SONAR-02**: Review and resolve macro issues (~111 "replace with const/constexpr")
- [ ] **SONAR-03**: Review and resolve global variable const issues (~63 remaining)
- [ ] **SONAR-04**: Review and resolve nesting depth issues (~52 deep nesting)

### Code Refactoring

- [ ] **REFACT-01**: Reduce cognitive complexity in key functions (~24 high complexity)
- [ ] **REFACT-02**: Extract helper functions from deeply nested code blocks
- [ ] **REFACT-03**: Consolidate duplicate code patterns (17 duplication blocks)

### C++23 Advanced Features

- [ ] **CPP23-01**: Evaluate `import std;` modules feasibility
- [ ] **CPP23-02**: Apply `std::flat_map` / `std::flat_set` where applicable
- [ ] **CPP23-03**: Modernize with `std::stacktrace` (if MSVC bugs fixed)

### Diagnostics & Logging

- [ ] **DIAG-01**: Enhance error messages with more context
- [ ] **DIAG-02**: Improve tracing coverage for key code paths
- [ ] **DIAG-03**: Add structured logging where feasible

### Verification

- [ ] **VER-01**: Build verification after all changes
- [ ] **VER-02**: Final SonarQube scan showing improvement

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

## Traceability

### v1.3 Requirements (Current Milestone)

| Requirement | Phase | Status |
|-------------|-------|--------|
| SONAR-01 | Phase 21 | Pending |
| SONAR-02 | Phase 22 | Pending |
| SONAR-03 | Phase 23 | Pending |
| SONAR-04 | Phase 24 | Pending |
| REFACT-01 | Phase 25 | Pending |
| REFACT-02 | Phase 25 | Pending |
| REFACT-03 | Phase 26 | Pending |
| CPP23-01 | Phase 27 | Pending |
| CPP23-02 | Phase 27 | Pending |
| CPP23-03 | Phase 27 | Pending |
| DIAG-01 | Phase 28 | Pending |
| DIAG-02 | Phase 28 | Pending |
| DIAG-03 | Phase 28 | Pending |
| VER-01 | Phase 29 | Pending |
| VER-02 | Phase 30 | Pending |

**Coverage:**
- v1.3 requirements: 15 total
- Mapped to phases: 15
- Unmapped: 0 ✓

---
*Requirements defined: 2026-02-15*
*Last updated: 2026-02-17 after v1.3 milestone start*
