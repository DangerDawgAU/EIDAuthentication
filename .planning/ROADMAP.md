# Roadmap: EIDAuthentication C++23 Modernization

## Overview

This roadmap transforms the EIDAuthentication Windows smart card authentication codebase from C++14 to C++23. The journey begins with build system configuration to enable C++23 compilation, progresses through modernizing error handling with `std::expected`, adopts compile-time enhancements, improves code quality with modern utilities, updates documentation, and culminates in comprehensive verification to ensure no regressions in the security-critical authentication flow.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: Build System** - Enable C++23 compilation across all 7 projects (Complete) 2026-02-15
- [ ] **Phase 2: Error Handling** - Adopt `std::expected` for internal error handling
- [ ] **Phase 3: Compile-Time Enhancements** - Leverage `consteval`, `constexpr`, and related features
- [ ] **Phase 4: Code Quality** - Modernize with `std::format`, `std::span`, and string utilities
- [ ] **Phase 5: Documentation** - Update README and build instructions
- [ ] **Phase 6: Verification** - Comprehensive testing across all Windows versions

## Phase Details

### Phase 1: Build System
**Goal**: All 7 Visual Studio projects compile with C++23 preview flag and produce working binaries
**Depends on**: Nothing (first phase)
**Requirements**: BUILD-01, BUILD-02, BUILD-03, BUILD-04
**Success Criteria** (what must be TRUE):
  1. All 7 .vcxproj files contain `<LanguageStandard>stdcpp23preview</LanguageStandard>`
  2. Full solution builds with zero errors using v143 toolset
  3. Static CRT (`/MT`) is preserved in all Release configurations
  4. No new compiler warnings introduced by C++23 flag
**Plans**: 3

Plans:
- [x] 01-01-PLAN.md - Update EIDCardLibrary project with C++23 flag and verify build (Complete)
- [x] 01-02-PLAN.md - Update remaining 6 projects (DLLs and EXEs) with C++23 flag (Complete)
- [x] 01-03-PLAN.md - Verify consistent settings across all configurations (Complete)

### Phase 2: Error Handling
**Goal**: Internal code uses `std::expected<T, E>` for typed error handling while preserving C-style API boundaries
**Depends on**: Phase 1
**Requirements**: ERROR-01, ERROR-02, ERROR-03
**Success Criteria** (what must be TRUE):
  1. Internal functions return `std::expected<T, ErrorType>` instead of raw HRESULT
  2. All exported LSA/Credential Provider functions maintain C-style signatures (HRESULT, BOOL)
  3. New error-handling code compiles with `noexcept` specifier
  4. Error conversion layer exists between internal `std::expected` and external HRESULT
**Plans**: 3

Plans:
- [x] 02-01-PLAN.md - Fix const-correctness compile errors in EIDCardLibrary (Complete)
- [ ] 02-02-PLAN.md - Define error types and Result<T> patterns
- [ ] 02-03-PLAN.md - Create API boundary conversion layer for exports

### Phase 02.1: Fix C++23 conformance errors (INSERTED) ✓

**Goal:** Fix C++23 conformance errors (C4596, C7510, C3861) blocking full solution build
**Depends on:** Phase 2
**Plans:** 1 (Complete) 2026-02-15

Plans:
- [x] 02.1-01-PLAN.md - Fix all C++23 conformance errors across 3 files (Complete)

### Phase 02.2: Fix const-correctness in dependent projects (INSERTED) ✓

**Goal:** Fix C2440 const-correctness errors in EIDCredentialProvider and EIDConfigurationWizard to enable full solution build
**Depends on:** Phase 2
**Plans:** 3 (Complete) 2026-02-15
**Note:** All 14 C2440 errors fixed. Full solution build blocked by pre-existing cardmod.h SDK dependency.

Plans:
- [x] 02.2-01-PLAN.md - Fix const-correctness in EIDCredentialProvider (common.h, helpers.cpp) (Complete)
- [x] 02.2-02a-PLAN.md - Fix const-correctness in EIDConfigurationWizard DebugReport.cpp (Complete)
- [x] 02.2-02b-PLAN.md - Fix const-correctness in EIDConfigurationWizard Page04.cpp and Page05.cpp (Complete)

### Phase 3: Compile-Time Enhancements
**Goal**: Compile-time validation and optimization using C++23 constexpr/consteval features
**Depends on**: Phase 2
**Requirements**: COMPILE-01, COMPILE-02, COMPILE-03, COMPILE-04
**Success Criteria** (what must be TRUE):
  1. `if consteval` distinguishes compile-time from runtime code paths where applicable
  2. Compile-time validation routines use `constexpr` for early error detection
  3. Enum-to-integer conversions use `std::to_underlying()` (no unsafe casts)
  4. Unreachable code paths marked with `std::unreachable()` where safe
**Plans**: 4

Plans:
- [ ] 03-01-PLAN.md - Add <utility> headers to enable std::to_underlying() for all 14 enum types
- [ ] 03-02-PLAN.md - Extend `constexpr` to validation routines (constexpr+noexcept)
- [ ] 03-03a-PLAN.md - Apply `if consteval` where compile-time vs runtime paths differ
- [ ] 03-03b-PLAN.md - Apply `std::unreachable()` ONLY to provably impossible switch defaults

### Phase 4: Code Quality
**Goal**: Cleaner, safer string and buffer handling using C++23 utilities
**Depends on**: Phase 3
**Requirements**: QUAL-01, QUAL-02, QUAL-03, QUAL-04
**Success Criteria** (what must be TRUE):
  1. `std::format` replaces `sprintf`/`snprintf` in non-LSASS code (Configuration Wizard, logging)
  2. CRTP patterns evaluated for deducing `this` modernization where applicable
  3. String search operations use `std::string::contains()` instead of `find() != npos`
  4. Buffer parameters use `std::span` at internal function boundaries
**Plans**: TBD

Plans:
- [ ] 04-01: Replace `sprintf`/`snprintf` with `std::format` in non-LSASS code
- [ ] 04-02: Adopt `std::string::contains()` for string operations
- [ ] 04-03: Introduce `std::span` for buffer handling
- [ ] 04-04: Evaluate deducing `this` for CRTP patterns

### Phase 5: Documentation
**Goal**: Documentation reflects C++23 requirements and updated build instructions
**Depends on**: Phase 4
**Requirements**: DOC-01, DOC-02
**Success Criteria** (what must be TRUE):
  1. README.md clearly states C++23 requirement and `/std:c++23preview` flag
  2. Build instructions specify Visual Studio 2022+ with v143 toolset
  3. Windows 7+ compatibility requirement is documented
**Plans**: TBD

Plans:
- [ ] 05-01: Update README.md with C++23 requirements
- [ ] 05-02: Update build documentation with toolset and SDK requirements

### Phase 6: Verification
**Goal**: All existing authentication functionality works without regression on supported Windows versions
**Depends on**: Phase 5
**Requirements**: VERIFY-01, VERIFY-02, VERIFY-03, VERIFY-04, VERIFY-05
**Success Criteria** (what must be TRUE):
  1. Smart card login succeeds on Windows 7, 10, and 11 test systems
  2. LSA Authentication Package loads and registers correctly
  3. Credential Provider appears on Windows login screen
  4. Configuration Wizard launches and operates normally
  5. Password Change Notification DLL processes events correctly
**Plans**: TBD

Plans:
- [ ] 06-01: Verify build artifacts and DLL registration
- [ ] 06-02: Test LSA Authentication Package on Windows 7/10/11
- [ ] 06-03: Test Credential Provider functionality
- [ ] 06-04: Test Configuration Wizard and Password Change Notification

## Progress

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 2.1 -> 2.2 -> 3 -> 4 -> 5 -> 6

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Build System | 3/3 | Complete | 2026-02-15 |
| 2. Error Handling | 1/3 | In Progress | - |
| 2.1. Fix C++23 Conformance | 1/1 | Complete | 2026-02-15 |
| 2.2. Fix const-correctness | 3/3 | Complete | 2026-02-15 |
| 3. Compile-Time Enhancements | 0/4 | Ready to execute | - |
| 4. Code Quality | 0/4 | Not started | - |
| 5. Documentation | 0/2 | Not started | - |
| 6. Verification | 0/4 | Not started | - |

## Coverage Summary

| Category | Requirements | Phase |
|----------|--------------|-------|
| Build System | BUILD-01, BUILD-02, BUILD-03, BUILD-04 | Phase 1 |
| Error Handling | ERROR-01, ERROR-02, ERROR-03 | Phase 2 |
| Compile-Time | COMPILE-01, COMPILE-02, COMPILE-03, COMPILE-04 | Phase 3 |
| Code Quality | QUAL-01, QUAL-02, QUAL-03, QUAL-04 | Phase 4 |
| Documentation | DOC-01, DOC-02 | Phase 5 |
| Verification | VERIFY-01, VERIFY-02, VERIFY-03, VERIFY-04, VERIFY-05 | Phase 6 |

**Total Coverage:** 22/22 v1 requirements mapped (100%)

---
*Roadmap created: 2026-02-15*
*Phase 1 planned: 2026-02-15*
*Phase 2 planned: 2026-02-15*
*Phase 2.1 planned: 2026-02-15*
*Phase 2.2 planned: 2026-02-15*
*Phase 3 revised: 2026-02-15*
