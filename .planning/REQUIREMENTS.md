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

## v1.1 Requirements

SonarQube Quality Remediation — Close all 1,167 open issues.

### Security & Reliability (Critical)

- [ ] **SEC-01**: Resolve 2 security hotspots (strlen safety in StringConversion.cpp, DebugReport.cpp)
- [ ] **SEC-02**: Fix 3 reliability bugs (type punning in CompleteToken.cpp, dead code in Page05.cpp)

### Const Correctness

- [ ] **CONST-01**: Mark global variables as `const` (71 issues)
- [ ] **CONST-02**: Mark global pointers `const` at all levels (31 issues)
- [ ] **CONST-03**: Mark member functions `const` where appropriate (14 issues)
- [ ] **CONST-04**: Mark function parameters `const` where appropriate (multiple issues)

### Modern C++ Types

- [ ] **TYPE-01**: Replace C-style char arrays with `std::string` (149 issues)
- [ ] **TYPE-02**: Replace C-style arrays with `std::array`/`std::vector` (28 issues)
- [ ] **TYPE-03**: Convert plain `enum` to `enum class` (14 issues)
- [ ] **TYPE-04**: Replace `void*` with meaningful types (15 issues)
- [ ] **TYPE-05**: Replace `NULL`/`0` with `nullptr` (10 issues)

### Code Simplification

- [ ] **SIMPLE-01**: Replace redundant types with `auto` (126 issues)
- [ ] **SIMPLE-02**: Replace macros with `const`/`constexpr`/`enum` (111 issues)
- [ ] **SIMPLE-03**: Merge nested if statements (17 issues)
- [ ] **SIMPLE-04**: Remove or document empty statements (17 issues)
- [ ] **SIMPLE-05**: Separate multiple declarations (50 issues)

### Complexity & Memory

- [ ] **COMPLEX-01**: Reduce nesting depth to max 3 levels (52 issues)
- [ ] **COMPLEX-02**: Replace manual `new`/`delete` with RAII/smart pointers (26 issues)

### Modern Diagnostics

- [ ] **DIAG-01**: Replace `__FILE__`/`__LINE__`/`__FUNCTION__` with `std::source_location` (20 issues)
- [ ] **DIAG-02**: Use in-class initializers for member data (5 issues)

### Duplications

- [ ] **DUP-01**: Resolve 17 code duplication blocks across 6 files

### Final Verification

- [ ] **FINAL-01**: SonarQube scan shows 0 open issues (all fixed or marked N/A)
- [ ] **FINAL-02**: Build still compiles with C++23
- [ ] **FINAL-03**: No new issues introduced by remediation

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

### v1.1 Requirements (In Progress)

| Requirement | Phase | Status |
|-------------|-------|--------|
| SEC-01 | Phase 7 | Pending |
| SEC-02 | Phase 7 | Pending |
| CONST-01 | Phase 8 | Pending |
| CONST-02 | Phase 8 | Pending |
| CONST-03 | Phase 8 | Pending |
| CONST-04 | Phase 8 | Pending |
| TYPE-01 | Phase 9 | Pending |
| TYPE-02 | Phase 9 | Pending |
| TYPE-03 | Phase 9 | Pending |
| TYPE-04 | Phase 9 | Pending |
| TYPE-05 | Phase 9 | Pending |
| SIMPLE-01 | Phase 10 | Pending |
| SIMPLE-02 | Phase 10 | Pending |
| SIMPLE-03 | Phase 10 | Pending |
| SIMPLE-04 | Phase 10 | Pending |
| SIMPLE-05 | Phase 10 | Pending |
| COMPLEX-01 | Phase 11 | Pending |
| COMPLEX-02 | Phase 11 | Pending |
| DIAG-01 | Phase 12 | Pending |
| DIAG-02 | Phase 12 | Pending |
| DUP-01 | Phase 13 | Pending |
| FINAL-01 | Phase 14 | Pending |
| FINAL-02 | Phase 14 | Pending |
| FINAL-03 | Phase 14 | Pending |

**Coverage:**
- v1.1 requirements: 26 total
- Mapped to phases: 26
- Unmapped: 0

---
*Requirements defined: 2026-02-15*
*Last updated: 2026-02-17 after v1.1 milestone definition*
