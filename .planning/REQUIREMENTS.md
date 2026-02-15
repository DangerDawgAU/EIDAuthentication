# Requirements: EIDAuthentication C++23 Modernization

**Defined:** 2026-02-15
**Core Value:** Successfully compile the entire codebase with `/std:c++23` and leverage modern C++23 features to improve code quality, safety, and maintainability without breaking existing functionality.

## v1 Requirements

Requirements for initial C++23 modernization milestone. Each maps to roadmap phases.

### Build System

- [ ] **BUILD-01**: All 7 Visual Studio projects updated with `/std:c++23preview` flag
- [ ] **BUILD-02**: Code compiles cleanly with C++23 (no errors, no new warnings)
- [ ] **BUILD-03**: v143 toolset maintained for Windows 7+ compatibility
- [ ] **BUILD-04**: Static CRT (`/MT`) preserved for LSASS isolation

### Error Handling

- [ ] **ERROR-01**: `std::expected<T, E>` adopted for internal error handling
- [ ] **ERROR-02**: C-style API boundaries preserved (HRESULT, BOOL at exports)
- [ ] **ERROR-03**: All new error-handling code marked `noexcept`

### Compile-Time Enhancements

- [ ] **COMPILE-01**: `if consteval` used to distinguish compile-time vs runtime paths
- [ ] **COMPILE-02**: `constexpr` extended where beneficial for compile-time validation
- [ ] **COMPILE-03**: `std::to_underlying` replaces unsafe enum casts
- [ ] **COMPILE-04**: `std::unreachable()` marks impossible code paths (with care)

### Code Quality

- [ ] **QUAL-01**: `std::format` replaces `sprintf`/`snprintf` in non-LSASS code
- [ ] **QUAL-02**: ~~Deducing `this` modernizes CRTP patterns where applicable~~ **Status: NOT APPLICABLE** - Codebase contains ZERO CRTP (Curiously Recurring Template Pattern) implementations. Deducing `this` is a C++23 feature specifically designed to modernize CRTP code. Without existing CRTP patterns, there are no candidates for deducing `this` modernization. (See 04-RESEARCH.md section "QUAL-02: Deducing `this` - NOT APPLICABLE")
- [ ] **QUAL-03**: `std::string::contains()` adopted for simpler string operations
- [ ] **QUAL-04**: `std::span` used for buffer handling at internal boundaries

### Documentation

- [ ] **DOC-01**: README.md updated to reflect C++23 requirement
- [ ] **DOC-02**: Build instructions updated with MSVC version requirements

### Verification

- [ ] **VERIFY-01**: All existing authentication flows work (no regressions)
- [ ] **VERIFY-02**: Smart card login functional on Windows 7, 10, 11
- [ ] **VERIFY-03**: LSA Authentication Package loads correctly
- [ ] **VERIFY-04**: Credential Provider appears on login screen
- [ ] **VERIFY-05**: Configuration Wizard operational

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

Which phases cover which requirements. Updated during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| BUILD-01 | Phase 1 | Pending |
| BUILD-02 | Phase 1 | Pending |
| BUILD-03 | Phase 1 | Pending |
| BUILD-04 | Phase 1 | Pending |
| ERROR-01 | Phase 2 | Pending |
| ERROR-02 | Phase 2 | Pending |
| ERROR-03 | Phase 2 | Pending |
| COMPILE-01 | Phase 3 | Pending |
| COMPILE-02 | Phase 3 | Pending |
| COMPILE-03 | Phase 3 | Pending |
| COMPILE-04 | Phase 3 | Pending |
| QUAL-01 | Phase 4 | Pending |
| QUAL-02 | Phase 4 | NOT APPLICABLE |
| QUAL-03 | Phase 4 | Pending |
| QUAL-04 | Phase 4 | Pending |
| DOC-01 | Phase 5 | Pending |
| DOC-02 | Phase 5 | Pending |
| VERIFY-01 | Phase 6 | Pending |
| VERIFY-02 | Phase 6 | Pending |
| VERIFY-03 | Phase 6 | Pending |
| VERIFY-04 | Phase 6 | Pending |
| VERIFY-05 | Phase 6 | Pending |

**Coverage:**
- v1 requirements: 22 total
- Mapped to phases: 22
- Unmapped: 0

---
*Requirements defined: 2026-02-15*
*Last updated: 2026-02-15 after roadmap creation*
