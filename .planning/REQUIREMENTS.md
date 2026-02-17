# Requirements: EIDAuthentication C++23 Modernization

**Defined:** 2026-02-15
**Core Value:** A clean, maintainable, and secure codebase with zero static analysis issues, leveraging modern C++23 features while preserving all existing authentication functionality.

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

### Remaining v1.1 Issues — Deferred to v1.2 or Won't Fix

- CONST-01 through CONST-04, TYPE-01 through TYPE-05, SIMPLE-01 through SIMPLE-05, COMPLEX-01 through COMPLEX-02, DIAG-01 through DIAG-02, DUP-01 — Moved to v1.2 or marked Won't Fix

## v1.2 Requirements

Code Modernization — Resolve ~550 fixable SonarQube maintainability issues.

### Critical Fixes

- [ ] **CRIT-01**: Fix unannotated fall-through in switch statement (EIDConfigurationWizardPage06.cpp:44)

### Const Correctness

- [ ] **CONST-01**: Global variables marked const (71 issues)
- [ ] **CONST-02**: Global pointers have const at appropriate levels (31 issues)
- [ ] **CONST-03**: Member functions that don't modify state marked const (remaining issues)

### Modern Types

- [ ] **TYPE-01**: C-style char arrays converted to std::string where LSASS-safe (149 issues)
- [ ] **TYPE-02**: C-style arrays converted to std::array (28 issues)
- [ ] **TYPE-03**: Variable shadowing issues resolved (~20 issues)

### Code Quality

- [ ] **QUAL-01**: Unused variables removed (~15 issues)
- [ ] **QUAL-02**: Build passes after all code changes

### Documentation

- [ ] **DOC-01**: ~550 "Won't Fix" issues documented with justification in SonarQube
- [ ] **DOC-02**: VERIFICATION.md updated with v1.2 results

### Verification

- [ ] **VER-01**: SonarQube scan confirms all fixable issues resolved
- [ ] **VER-02**: No new issues introduced during modernization

## v2 Requirements

Deferred to future milestone. Tracked but not in current roadmap.

### Advanced Features

- **ADV-01**: `std::mdspan` for multidimensional buffer handling
- **ADV-02**: `auto(x)` decay-copy simplification (requires VS 2026)
- **ADV-03**: Multidimensional `operator[]` if applicable to buffer code
- **ADV-04**: Static `operator[]` for type-erased access patterns

### Future Considerations

- **FUTURE-01**: `import std;` modules when MSVC support matures
- **FUTURE-02**: `std::stacktrace` when MSVC bugs are fixed
- **FUTURE-03**: `std::flat_map` / `std::flat_set` when implemented in MSVC
- **FUTURE-04**: Stable `/std:c++23` flag (non-preview) when available
- **FUTURE-05**: v145 toolset if Windows 7 support is dropped

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

### v1.0 Requirements (Complete)

| Requirement | Phase | Status |
|-------------|-------|--------|
| BUILD-01 | Phase 1 | Complete |
| BUILD-02 | Phase 1 | Complete |
| BUILD-03 | Phase 1 | Complete |
| BUILD-04 | Phase 1 | Complete |
| ERROR-01 | Phase 2 | Complete |
| ERROR-02 | Phase 2 | Complete |
| ERROR-03 | Phase 2 | Complete |
| COMPILE-01 | Phase 3 | Complete (N/A) |
| COMPILE-02 | Phase 3 | Complete |
| COMPILE-03 | Phase 3 | Complete |
| COMPILE-04 | Phase 3 | Complete (N/A) |
| QUAL-01 | Phase 4 | Complete |
| QUAL-02 | Phase 4 | NOT APPLICABLE |
| QUAL-03 | Phase 4 | Complete |
| QUAL-04 | Phase 4 | Complete |
| DOC-01 | Phase 5 | Complete |
| DOC-02 | Phase 5 | Complete |
| VERIFY-01 | Phase 6 | Complete |
| VERIFY-02 | Phase 6 | Pending (runtime) |
| VERIFY-03 | Phase 6 | Pending (runtime) |
| VERIFY-04 | Phase 6 | Pending (runtime) |
| VERIFY-05 | Phase 6 | Pending (runtime) |

### v1.1 Requirements (Complete)

| Requirement | Phase | Status |
|-------------|-------|--------|
| SEC-01 | Phase 7 | Complete |
| SEC-02 | Phase 7 | Complete |
| CONST-01 | Phase 8 | Deferred to v1.2 |
| CONST-02 | Phase 8 | Deferred to v1.2 |
| CONST-03 | Phase 8 | Deferred to v1.2 |
| CONST-04 | Phase 8 | Won't Fix |
| TYPE-01 | Phase 9 | Deferred to v1.2 |
| TYPE-02 | Phase 9 | Deferred to v1.2 |
| TYPE-03 | Phase 9 | Won't Fix |
| TYPE-04 | Phase 9 | Won't Fix |
| TYPE-05 | Phase 9 | Complete (v1.1) |
| SIMPLE-01 | Phase 10 | Won't Fix (style) |
| SIMPLE-02 | Phase 10 | Won't Fix (style) |
| SIMPLE-03 | Phase 10 | Won't Fix |
| SIMPLE-04 | Phase 10 | Won't Fix |
| SIMPLE-05 | Phase 10 | Won't Fix |
| COMPLEX-01 | Phase 11 | Won't Fix (risky) |
| COMPLEX-02 | Phase 11 | Won't Fix (LSASS) |
| DIAG-01 | Phase 12 | Won't Fix |
| DIAG-02 | Phase 12 | Won't Fix |
| DUP-01 | Phase 13 | Complete (1.9%) |
| FINAL-01 | Phase 14 | Deferred to v1.2 |
| FINAL-02 | Phase 14 | Complete |
| FINAL-03 | Phase 14 | Deferred to v1.2 |

**Coverage:**
- v1.1 requirements: 26 total
- Complete: 5
- Deferred to v1.2: 6
- Won't Fix: 15

### v1.2 Requirements (In Progress)

| Requirement | Phase | Status |
|-------------|-------|--------|
| CRIT-01 | Phase 15 | Pending |
| CONST-01 | Phase 16 | Pending |
| CONST-02 | Phase 16 | Pending |
| CONST-03 | Phase 16 | Pending |
| TYPE-01 | Phase 17 | Pending |
| TYPE-02 | Phase 17 | Pending |
| TYPE-03 | Phase 17 | Pending |
| QUAL-01 | Phase 18 | Pending |
| QUAL-02 | Phase 18 | Pending |
| DOC-01 | Phase 19 | Pending |
| DOC-02 | Phase 19 | Pending |
| VER-01 | Phase 20 | Pending |
| VER-02 | Phase 20 | Pending |

**Coverage:**
- v1.2 requirements: 13 total
- Mapped to phases: 13
- Unmapped: 0

---
*Requirements defined: 2026-02-15*
*Last updated: 2026-02-17 after v1.2 roadmap created*
