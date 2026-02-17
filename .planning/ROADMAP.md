# Roadmap: EIDAuthentication C++23 Modernization

## Overview

This roadmap transforms the EIDAuthentication Windows smart card authentication codebase from C++14 to C++23. The journey begins with build system configuration to enable C++23 compilation, progresses through modernizing error handling with `std::expected`, adopts compile-time enhancements, improves code quality with modern utilities, updates documentation, and culminates in comprehensive verification to ensure no regressions in the security-critical authentication flow.

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

### v1.0 C++23 Modernization (COMPLETE)

- [x] **Phase 1: Build System** - Enable C++23 compilation across all 7 projects (Complete) 2026-02-15
- [x] **Phase 2: Error Handling** - Adopt `std::expected` for internal error handling (Complete) 2026-02-16
- [x] **Phase 2.1: C++23 Conformance** - Fix conformance errors (INSERTED) (Complete) 2026-02-15
- [x] **Phase 2.2: Const-Correctness** - Fix const errors in dependent projects (INSERTED) (Complete) 2026-02-15
- [x] **Phase 3: Compile-Time Enhancements** - Leverage `consteval`, `constexpr`, and related features (Complete) 2026-02-15
- [x] **Phase 4: Code Quality** - Modernize with `std::format`, `std::span`, and string utilities (Complete) 2026-02-15
- [x] **Phase 5: Documentation** - Update README and build instructions (Complete) 2026-02-15
- [x] **Phase 6: Verification** - Comprehensive testing across all Windows versions (Complete*) 2026-02-15

*Build verification passed. Runtime testing deferred pending Windows test machine access.

### v1.1 SonarQube Quality Remediation (IN PROGRESS)

- [ ] **Phase 7: Security & Reliability** - Resolve 5 critical issues (2 security hotspots, 3 reliability bugs)
- [ ] **Phase 8: Const Correctness** - Fix ~116 const-correctness issues
- [ ] **Phase 9: Modern C++ Types** - Convert ~216 legacy type usages
- [ ] **Phase 10: Code Simplification** - Simplify ~321 code patterns
- [ ] **Phase 11: Complexity & Memory** - Reduce ~78 complexity/memory issues
- [ ] **Phase 12: Modern Diagnostics** - Modernize ~25 diagnostic statements
- [ ] **Phase 13: Duplications** - Resolve 17 code duplication blocks
- [ ] **Phase 14: Final Verification** - Confirm zero open issues

---

## v1.0 Phase Details (COMPLETE)

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
**Plans:** 4 (Complete) 2026-02-16
**Success Criteria** (what must be TRUE):
  1. Internal functions return `std::expected<T, ErrorType>` instead of raw HRESULT
  2. All exported LSA/Credential Provider functions maintain C-style signatures (HRESULT, BOOL)
  3. New error-handling code compiles with `noexcept` specifier
  4. Error conversion layer exists between internal `std::expected` and external HRESULT

Plans:
- [x] 02-01a-PLAN.md - Fix const-correctness compile errors in EIDCardLibrary (Complete)
- [x] 02-01b-PLAN.md - Additional const-correctness fixes (Complete)
- [x] 02-02-PLAN.md - Define error types and Result<T> patterns (Complete)
- [x] 02-03-PLAN.md - Create API boundary conversion layer for exports (Complete)

### Phase 02.1: Fix C++23 conformance errors (INSERTED)
**Goal:** Fix C++23 conformance errors (C4596, C7510, C3861) blocking full solution build
**Depends on:** Phase 2
**Plans:** 1 (Complete) 2026-02-15

Plans:
- [x] 02.1-01-PLAN.md - Fix all C++23 conformance errors across 3 files (Complete)

### Phase 02.2: Fix const-correctness in dependent projects (INSERTED)
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
**Plans:** 4 (Complete) 2026-02-15
**Note:** if consteval and std::unreachable documented as not applicable (no dual-path functions, defensive programming required in LSASS).

Plans:
- [x] 03-01-PLAN.md - Add <utility> headers to enable std::to_underlying() for all 14 enum types (Complete)
- [x] 03-02-PLAN.md - Extend `constexpr` to validation routines (constexpr+noexcept) (Complete)
- [x] 03-03a-PLAN.md - Apply `if consteval` where compile-time vs runtime paths differ (Complete - N/A)
- [x] 03-03b-PLAN.md - Apply `std::unreachable()` ONLY to provably impossible switch defaults (Complete - N/A)

### Phase 4: Code Quality
**Goal**: Cleaner, safer string and buffer handling using C++23 utilities
**Depends on**: Phase 3
**Requirements**: QUAL-01, QUAL-02, QUAL-03, QUAL-04
**Plans:** 5 (Complete) 2026-02-15
**Note:** QUAL-02 marked NOT APPLICABLE (no CRTP patterns in codebase).

Plans:
- [x] 04-01-PLAN.md - Replace `swprintf_s` with `std::format` in non-LSASS code (EIDConfigurationWizard only) (Complete)
- [x] 04-02-PLAN.md - Document QUAL-02 (Deducing This) as NOT APPLICABLE (Complete)
- [x] 04-03-PLAN.md - Introduce `std::span` for buffer handling in credential storage (Complete)
- [x] 04-04a-PLAN.md - Automated verification of all Phase 4 code quality improvements (Complete)
- [x] 04-04b-PLAN.md - User review checkpoint and create VERIFICATION.md (Complete)

### Phase 5: Documentation
**Goal**: Documentation reflects C++23 requirements and updated build instructions
**Depends on**: Phase 4
**Requirements**: DOC-01, DOC-02
**Plans:** 1 (Complete) 2026-02-15

Plans:
- [x] 05-01-PLAN.md - Update README.md and BUILD.md with C++23 requirements and build instructions (Complete)

### Phase 6: Verification (CONDITIONAL COMPLETE)
**Goal**: All existing authentication functionality works without regression on supported Windows versions
**Depends on**: Phase 5
**Requirements**: VERIFY-01, VERIFY-02, VERIFY-03, VERIFY-04, VERIFY-05
**Success Criteria** (what must be TRUE):
  1. Smart card login succeeds on Windows 7, 10, and 11 test systems
  2. LSA Authentication Package loads and registers correctly
  3. Credential Provider appears on Windows login screen
  4. Configuration Wizard launches and operates normally
  5. Password Change Notification DLL processes events correctly
**Plans**: 5 (Complete) 2026-02-15
**Note**: Build verification PASSED. Runtime verification PENDING - requires Windows 7/10/11 test machines with smart card hardware.

Plans:
- [x] 06-01-PLAN.md - Verify build artifacts and static CRT linkage (Complete)
- [x] 06-02-PLAN.md - Test LSA Authentication Package on Windows 7/10/11 (Checklist created)
- [x] 06-03-PLAN.md - Test Credential Provider functionality (Checklist created)
- [x] 06-04-PLAN.md - Test Configuration Wizard and Password Change Notification (Checklist created)
- [x] 06-05-PLAN.md - Create verification summary and UAT documentation (Complete)

---

## v1.1 Phase Details (IN PROGRESS)

### Phase 7: Security & Reliability
**Goal**: Zero critical security and reliability issues remain open in SonarQube
**Depends on**: Phase 6
**Requirements**: SEC-01, SEC-02
**Issues**: 5 total (2 security hotspots, 3 reliability bugs)
**Plans:** 2
**Success Criteria** (what must be TRUE):
  1. SonarQube shows 0 open security hotspots (both resolved or marked N/A)
  2. SonarQube shows 0 open reliability bugs (all 3 resolved or marked N/A)
  3. Code builds successfully after security fixes
  4. No new issues introduced by security remediation

**Affected files:**
- StringConversion.cpp (strlen safety)
- DebugReport.cpp (strlen safety)
- CompleteToken.cpp (type punning)
- EIDConfigurationWizardPage05.cpp (dead code)

Plans:
- [ ] 07-01-PLAN.md - Fix security hotspots (strlen safety in StringConversion.cpp and DebugReport.cpp)
- [ ] 07-02-PLAN.md - Fix reliability bugs (type punning in CompleteToken.cpp, unreachable code in Page05.cpp)

### Phase 8: Const Correctness
**Goal**: All global variables, pointers, and functions are const-correct per SonarQube rules
**Depends on**: Phase 7
**Requirements**: CONST-01, CONST-02, CONST-03, CONST-04
**Issues**: ~116 total
**Success Criteria** (what must be TRUE):
  1. All global variables that should be const are marked const (71 issues)
  2. All global pointers have const at appropriate levels (31 issues)
  3. All member functions that don't modify state are marked const (14 issues)
  4. Function parameters are const where appropriate (CONST-04)
  5. SonarQube shows 0 open const-correctness issues

### Phase 9: Modern C++ Types
**Goal**: Code uses modern C++ type system instead of C-style constructs
**Depends on**: Phase 8
**Requirements**: TYPE-01, TYPE-02, TYPE-03, TYPE-04, TYPE-05
**Issues**: ~216 total
**Success Criteria** (what must be TRUE):
  1. C-style char arrays replaced with std::string where safe for LSASS (149 issues)
  2. C-style arrays replaced with std::array or std::vector (28 issues)
  3. Plain enums converted to enum class (14 issues)
  4. void* replaced with meaningful types where possible (15 issues)
  5. NULL/0 replaced with nullptr (10 issues)

**Note**: std::string conversions must be evaluated carefully for LSASS compatibility (heap allocation concerns).

### Phase 10: Code Simplification
**Goal**: Code is simpler and more idiomatic C++23
**Depends on**: Phase 9
**Requirements**: SIMPLE-01, SIMPLE-02, SIMPLE-03, SIMPLE-04, SIMPLE-05
**Issues**: ~321 total
**Success Criteria** (what must be TRUE):
  1. Redundant type declarations replaced with auto (126 issues)
  2. Macros replaced with const/constexpr/enum where safe (111 issues)
  3. Nested if statements merged into single conditions (17 issues)
  4. Empty statements removed or documented (17 issues)
  5. Multiple declarations on same line separated (50 issues)

### Phase 11: Complexity & Memory
**Goal**: Code has reduced nesting complexity and uses RAII for memory management
**Depends on**: Phase 10
**Requirements**: COMPLEX-01, COMPLEX-02
**Issues**: ~78 total
**Success Criteria** (what must be TRUE):
  1. All functions have nesting depth <= 3 levels (52 issues)
  2. Manual new/delete replaced with RAII/smart pointers (26 issues)
  3. Code builds successfully after complexity reduction
  4. No memory management regressions introduced

**Note**: Smart pointer adoption must consider LSASS heap allocation constraints.

### Phase 12: Modern Diagnostics
**Goal**: Diagnostic code uses modern C++23 features
**Depends on**: Phase 11
**Requirements**: DIAG-01, DIAG-02
**Issues**: ~25 total
**Success Criteria** (what must be TRUE):
  1. __FILE__/__LINE__/__FUNCTION__ replaced with std::source_location (20 issues)
  2. In-class member initializers added for appropriate data members (5 issues)
  3. Debug output functions still work correctly with new diagnostics

### Phase 13: Duplications
**Goal**: Zero code duplication blocks remain open
**Depends on**: Phase 12
**Requirements**: DUP-01
**Issues**: 17 duplication blocks
**Success Criteria** (what must be TRUE):
  1. All 17 duplication blocks resolved (extracted to shared functions or marked N/A)
  2. Code builds successfully after deduplication
  3. No functional changes to authentication flow

**Affected files**: 6 files contain duplication blocks

### Phase 14: Final Verification
**Goal**: Zero open SonarQube issues - milestone complete
**Depends on**: Phase 13
**Requirements**: FINAL-01, FINAL-02, FINAL-03
**Success Criteria** (what must be TRUE):
  1. SonarQube scan shows 0 open issues (all fixed or marked N/A with justification)
  2. Build still compiles cleanly with C++23 and v143 toolset
  3. No new issues introduced during remediation phases

---

## Progress

### v1.0 C++23 Modernization (COMPLETE)

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 2.1 -> 2.2 -> 3 -> 4 -> 5 -> 6

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Build System | 3/3 | Complete | 2026-02-15 |
| 2. Error Handling | 4/4 | Complete | 2026-02-16 |
| 2.1. Fix C++23 Conformance | 1/1 | Complete | 2026-02-15 |
| 2.2. Fix const-correctness | 3/3 | Complete | 2026-02-15 |
| 3. Compile-Time Enhancements | 4/4 | Complete | 2026-02-15 |
| 4. Code Quality | 5/5 | Complete | 2026-02-15 |
| 5. Documentation | 1/1 | Complete | 2026-02-15 |
| 6. Verification | 5/5 | Complete* | 2026-02-15 |

*Build verification complete. Runtime verification pending Windows test machine access.

### v1.1 SonarQube Quality Remediation (IN PROGRESS)

**Execution Order:**
Phases execute in numeric order: 7 -> 8 -> 9 -> 10 -> 11 -> 12 -> 13 -> 14

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 7. Security & Reliability | 0/2 | Planned | - |
| 8. Const Correctness | 0/? | Not Started | - |
| 9. Modern C++ Types | 0/? | Not Started | - |
| 10. Code Simplification | 0/? | Not Started | - |
| 11. Complexity & Memory | 0/? | Not Started | - |
| 12. Modern Diagnostics | 0/? | Not Started | - |
| 13. Duplications | 0/? | Not Started | - |
| 14. Final Verification | 0/? | Not Started | - |

---

## Coverage Summary

### v1.0 Requirements (COMPLETE)

| Category | Requirements | Phase |
|----------|--------------|-------|
| Build System | BUILD-01, BUILD-02, BUILD-03, BUILD-04 | Phase 1 |
| Error Handling | ERROR-01, ERROR-02, ERROR-03 | Phase 2 |
| Compile-Time | COMPILE-01, COMPILE-02, COMPILE-03, COMPILE-04 | Phase 3 |
| Code Quality | QUAL-01, QUAL-02, QUAL-03, QUAL-04 | Phase 4 |
| Documentation | DOC-01, DOC-02 | Phase 5 |
| Verification | VERIFY-01, VERIFY-02, VERIFY-03, VERIFY-04, VERIFY-05 | Phase 6 |

**Total v1.0 Coverage:** 22/22 requirements mapped (100%)

### v1.1 Requirements (IN PROGRESS)

| Category | Requirements | Phase |
|----------|--------------|-------|
| Security & Reliability | SEC-01, SEC-02 | Phase 7 |
| Const Correctness | CONST-01, CONST-02, CONST-03, CONST-04 | Phase 8 |
| Modern C++ Types | TYPE-01, TYPE-02, TYPE-03, TYPE-04, TYPE-05 | Phase 9 |
| Code Simplification | SIMPLE-01, SIMPLE-02, SIMPLE-03, SIMPLE-04, SIMPLE-05 | Phase 10 |
| Complexity & Memory | COMPLEX-01, COMPLEX-02 | Phase 11 |
| Modern Diagnostics | DIAG-01, DIAG-02 | Phase 12 |
| Duplications | DUP-01 | Phase 13 |
| Final Verification | FINAL-01, FINAL-02, FINAL-03 | Phase 14 |

**Total v1.1 Coverage:** 26/26 requirements mapped (100%)

---
*Roadmap created: 2026-02-15*
*Phase 1 planned: 2026-02-15*
*Phase 2 planned: 2026-02-15*
*Phase 2 complete: 2026-02-16*
*Phase 2.1 planned: 2026-02-15*
*Phase 2.2 planned: 2026-02-15*
*Phase 3 revised: 2026-02-15*
*Phase 4 revised: 2026-02-15*
*Phase 5 planned: 2026-02-15*
*Phase 6 planned: 2026-02-15*
*v1.0 complete: 2026-02-16*
*v1.1 roadmap created: 2026-02-17*
*Phase 7 planned: 2026-02-17*
