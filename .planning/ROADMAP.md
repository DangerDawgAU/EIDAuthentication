# Roadmap: EIDAuthentication C++23 Modernization

## Overview

This roadmap transforms the EIDAuthentication Windows smart card authentication codebase from C++14 to C++23. The journey begins with build system configuration to enable C++23 compilation, progresses through modernizing error handling with `std::expected`, adopts compile-time enhancements, improves code quality with modern utilities, updates documentation, and culminates in comprehensive verification to ensure no regressions in the security-critical authentication flow.

## Milestones

- **v1.0 C++23 Modernization** - Phases 1-6 (COMPLETE 2026-02-16)
- **v1.1 SonarQube Quality Remediation** - Phases 7-14 (COMPLETE 2026-02-17)
- **v1.2 Code Modernization** - Phases 15-20 (COMPLETE 2026-02-17)
- **v1.3 Deep Modernization** - Phases 21-30 (COMPLETE 2026-02-18)
- **v1.4 SonarQube Zero** - Phases 31-40 (IN PROGRESS)

## Phases

**Phase Numbering:**
- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

### v1.0 C++23 Modernization (COMPLETE)

<details>
<summary>v1.0 Phases 1-6 - SHIPPED 2026-02-16</summary>

- [x] **Phase 1: Build System** - Enable C++23 compilation across all 7 projects (Complete) 2026-02-15
- [x] **Phase 2: Error Handling** - Adopt `std::expected` for internal error handling (Complete) 2026-02-16
- [x] **Phase 2.1: C++23 Conformance** - Fix conformance errors (INSERTED) (Complete) 2026-02-15
- [x] **Phase 2.2: Const-Correctness** - Fix const errors in dependent projects (INSERTED) (Complete) 2026-02-15
- [x] **Phase 3: Compile-Time Enhancements** - Leverage `consteval`, `constexpr`, and related features (Complete) 2026-02-15
- [x] **Phase 4: Code Quality** - Modernize with `std::format`, `std::span`, and string utilities (Complete) 2026-02-15
- [x] **Phase 5: Documentation** - Update README and build instructions (Complete) 2026-02-15
- [x] **Phase 6: Verification** - Comprehensive testing across all Windows versions (Complete*) 2026-02-15

</details>

### v1.1 SonarQube Quality Remediation (COMPLETE)

<details>
<summary>v1.1 Phases 7-14 - SHIPPED 2026-02-17</summary>

- [x] **Phase 7: Security & Reliability** - Resolve 5 critical issues (2 security hotspots, 3 reliability bugs) (Complete) 2026-02-17
- [x] **Phase 8: Const Correctness** - Fix ~116 const-correctness issues (Deferred to v1.2) 2026-02-17
- [x] **Phase 9: Modern C++ Types** - Convert ~216 legacy type usages (Deferred to v1.2) 2026-02-17
- [x] **Phase 10: Code Simplification** - Simplify ~321 code patterns (Won't Fix) 2026-02-17
- [x] **Phase 11: Complexity & Memory** - Reduce ~78 complexity/memory issues (Won't Fix) 2026-02-17
- [x] **Phase 12: Modern Diagnostics** - Modernize ~25 diagnostic statements (Won't Fix) 2026-02-17
- [x] **Phase 13: Duplications** - Resolve 17 code duplication blocks (Complete 1.9%) 2026-02-17
- [x] **Phase 14: Final Verification** - Confirm zero open issues (Deferred to v1.2) 2026-02-17

</details>

### v1.2 Code Modernization (COMPLETE)

<details>
<summary>v1.2 Phases 15-20 - SHIPPED 2026-02-17</summary>

- [x] **Phase 15: Critical Fix** - Fix 1 blocker issue (unannotated fall-through) (Complete) 2026-02-17
- [x] **Phase 16: Const Correctness** - Fix 102+ const-correctness issues (Complete) 2026-02-17
- [x] **Phase 17: Modern Types** - Convert ~197 legacy type usages (Complete) 2026-02-17
- [x] **Phase 18: Code Quality** - Build verification after all code changes (Complete) 2026-02-17
- [x] **Phase 19: Documentation** - Mark ~550 issues as "Won't Fix" with justification (Complete) 2026-02-17
- [x] **Phase 20: Final Verification** - SonarQube confirmation all fixable issues resolved (Complete) 2026-02-17

</details>

### v1.3 Deep Modernization (COMPLETE)

<details>
<summary>v1.3 Phases 21-30 - SHIPPED 2026-02-18</summary>

- [x] **Phase 21: SonarQube Style Issues** - Replace explicit types with auto where clearer (completed 2026-02-17)
- [x] **Phase 22: SonarQube Macro Issues** - Convert macros to const/constexpr where safe (completed 2026-02-17)
- [x] **Phase 23: SonarQube Const Issues** - Mark remaining global variables const (completed 2026-02-17)
- [x] **Phase 24: SonarQube Nesting Issues** - Reduce deep nesting in key functions (completed 2026-02-17)
- [x] **Phase 25: Code Refactoring - Complexity** - Reduce cognitive complexity and extract helpers (completed 2026-02-17)
- [x] **Phase 26: Code Refactoring - Duplicates** - Consolidate duplicate code patterns (completed 2026-02-17)
- [x] **Phase 27: C++23 Advanced Features** - Evaluate modules, flat containers, stacktrace (completed 2026-02-17)
- [x] **Phase 28: Diagnostics & Logging** - Enhance error messages and tracing (completed 2026-02-17)
- [x] **Phase 29: Build Verification** - Verify all changes compile and work (completed 2026-02-17)
- [x] **Phase 30: Final SonarQube Scan** - Confirm improvement in code quality metrics (completed 2026-02-17)

</details>

### v1.4 SonarQube Zero (IN PROGRESS)

**Milestone Goal:** Eliminate all fixable SonarQube issues and document remaining won't-fix with justifications

- [x] **Phase 31: Macro to constexpr** - Convert safe macros to constexpr, document won't-fix for RC/flow-control macros (Complete 2026-02-18)
- [x] **Phase 32: Auto Conversion** - Convert redundant type declarations to auto where type is obvious (Complete 2026-02-18)
- [x] **Phase 33: Independent Style Issues** - Fix C-style casts, enum class conversions, Windows API enum won't-fix (Complete 2026-02-18)
- [ ] **Phase 34: Const Correctness - Globals** - Mark global variables const where truly immutable
- [ ] **Phase 35: Const Correctness - Functions** - Mark member functions const where state is not modified
- [ ] **Phase 36: Complexity Reduction** - Reduce cognitive complexity via helper function extraction
- [ ] **Phase 37: Nesting Reduction** - Reduce deep nesting via early return/guard clauses
- [ ] **Phase 38: Init-statements** - Use init-statements in if/switch where scope benefits
- [ ] **Phase 39: Integration Changes** - std::array conversion, LSA safety won't-fix documentation
- [ ] **Phase 40: Final Verification** - Full build verification, SonarQube scan, won't-fix documentation

---

## v1.4 Phase Details

### Phase 31: Macro to constexpr
**Goal**: Simple value macros converted to constexpr, won't-fix documented for resource compiler and flow-control macros
**Depends on**: Nothing (foundation phase)
**Requirements**: MACRO-01, MACRO-02, MACRO-03
**Success Criteria** (what must be TRUE):
  1. All simple value macros that can be constexpr are converted
  2. Resource compiler macros (used in .rc files) documented as won't-fix with justification
  3. Flow-control macros documented as won't-fix with SEH/context justification
  4. Build passes with zero errors after macro conversions
**Plans**:
- [x] 31-01-PLAN.md - Convert MAX_ULONG, CLSCTX_INPROC_SERVER to constexpr; document CERT_HASH_LENGTH as won't-fix (MACRO-02)

### Phase 32: Auto Conversion
**Goal**: Redundant type declarations converted to auto where type is obvious from initializer
**Depends on**: Nothing (independent phase)
**Requirements**: AUTO-01, AUTO-02
**Success Criteria** (what must be TRUE):
  1. Iterator declarations converted to auto (e.g., `auto it = map.begin()`)
  2. New object declarations converted to auto (e.g., `auto ptr = new Type()`)
  3. Security-critical types (HRESULT, NTSTATUS, handles) kept explicit - documented as won't-fix
  4. Build passes with zero errors after auto conversions
**Plans**:
- [x] 32-01-PLAN.md - Convert new object declarations, iterator declarations, and static_cast results to auto

### Phase 33: Independent Style Issues
**Goal**: Independent style improvements: C-style casts, enum class, Windows API enum won't-fix
**Depends on**: Nothing (independent phase)
**Requirements**: MODERN-02, MODERN-05, MODERN-06
**Success Criteria** (what must be TRUE):
  1. C-style casts replaced with named casts (static_cast, reinterpret_cast, const_cast)
  2. Internal enum types converted to enum class where safe
  3. Windows API enum types documented as won't-fix with API compatibility justification
  4. Build passes with zero errors after style changes
**Plans**:
- [x] 33-01-PLAN.md - Convert internal enum types to enum class, replace C-style casts with named casts in EIDAuthenticationPackage, document Windows API enum types as won't-fix (MODERN-02, MODERN-05, MODERN-06)

### Phase 34: Const Correctness - Globals
**Goal**: Global variables marked const where truly immutable, runtime-assigned documented as won't-fix
**Depends on**: Phase 31 (macros must be constexpr before globals can be const)
**Requirements**: CONST-01, CONST-02
**Success Criteria** (what must be TRUE):
  1. Compile-time constant globals marked const
  2. Runtime-assigned globals (LSA pointers, DLL state, tracing state) documented as won't-fix
  3. Set*() pattern and DllMain initialization checked before marking const
  4. Build passes with zero errors after const additions
**Plans**:
- [ ] 34-01-PLAN.md - Analyze all global variables, document runtime-assigned globals as won't-fix (LSA pointers, DLL state, tracing state, UI state, file handles, SAM function pointers)

### Phase 35: Const Correctness - Functions
**Goal**: Member functions marked const where state is not modified, COM/interface methods documented as won't-fix
**Depends on**: Nothing (independent of macro work)
**Requirements**: CONST-03, CONST-04
**Success Criteria** (what must be TRUE):
  1. Member functions that don't modify state marked const
  2. COM interface methods documented as won't-fix (cannot change signatures)
  3. Windows callback methods documented as won't-fix (API compatibility)
  4. Build passes with zero errors after const additions
**Plans**:
- [ ] 35-01-PLAN.md - Mark CContainerHolderFactory::HasContainerHolder() and ContainerHolderCount() const, mark CMessageCredential::GetStatus() const, document COM interface methods as won't-fix

### Phase 36: Complexity Reduction
**Goal**: Cognitive complexity reduced via helper function extraction, SEH-protected code documented as won't-fix
**Depends on**: Nothing (foundation for Phase 37)
**Requirements**: STRUCT-01, STRUCT-02
**Success Criteria** (what must be TRUE):
  1. High cognitive complexity functions have helper functions extracted
  2. SEH-protected code documented as won't-fix (cannot extract from __try blocks)
  3. Helper functions maintain SEH safety (no heap allocation in LSASS context)
  4. Build passes with zero errors after complexity reduction
**Plans**: TBD

### Phase 37: Nesting Reduction
**Goal**: Deep nesting reduced via early return and guard clauses, SEH blocks kept intact
**Depends on**: Phase 36 (complexity helpers reduce nesting)
**Requirements**: STRUCT-03, STRUCT-04
**Success Criteria** (what must be TRUE):
  1. Guard clauses added at function entry for early return
  2. Deep nesting reduced using helper functions from Phase 36
  3. SEH blocks documented as won't-fix (no extraction from __try)
  4. Build passes with zero errors after nesting reduction
**Plans**: TBD

### Phase 38: Init-statements
**Goal**: Init-statements used in if/switch where scope benefits code clarity
**Depends on**: Nothing (independent, but Phase 37 helps with cleaner code)
**Requirements**: MODERN-01
**Success Criteria** (what must be TRUE):
  1. Variable declarations scoped to if/switch blocks using init-statements
  2. Iterator scope limited using if-init pattern
  3. Build passes with zero errors after init-statement additions
**Plans**:
- [ ] 38-01-PLAN.md - Convert ~49 init-statement opportunities across EIDCredentialProvider, EIDCardLibrary, and EIDConfigurationWizard projects

### Phase 39: Integration Changes
**Goal**: std::array conversion where stack size permits, LSA safety patterns documented as won't-fix
**Depends on**: Phases 31-38 (all other changes complete)
**Requirements**: MODERN-03, MODERN-04
**Success Criteria** (what must be TRUE):
  1. C-style arrays converted to std::array where stack size permits
  2. Large stack buffers kept as C-style arrays (stack safety)
  3. std::string/std::vector documented as won't-fix with LSASS heap safety justification
  4. Build passes with zero errors after integration changes
**Plans**:
- [ ] 39-01-PLAN.md - Convert small C-style arrays to std::array, document won't-fix categories for LSASS safety

### Phase 40: Final Verification
**Goal**: Confirm all remediation is complete, stable, and documented
**Depends on**: Phases 31-39 (all phases complete)
**Requirements**: VER-01, VER-02, VER-03, VER-04
**Success Criteria** (what must be TRUE):
  1. All 7 projects build with zero errors
  2. No new warnings introduced compared to baseline
  3. SonarQube scan shows improvement from v1.4 baseline
  4. All won't-fix issues documented with specific justifications
**Plans**: TBD

---

## Progress

### v1.0 C++23 Modernization (COMPLETE)

<details>
<summary>v1.0 Progress</summary>

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

</details>

### v1.1 SonarQube Quality Remediation (COMPLETE)

<details>
<summary>v1.1 Progress</summary>

**Execution Order:**
Phases execute in numeric order: 7 -> 8 -> 9 -> 10 -> 11 -> 12 -> 13 -> 14

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 7. Security & Reliability | 2/2 | Complete | 2026-02-17 |
| 8. Const Correctness | - | Deferred to v1.2 | 2026-02-17 |
| 9. Modern C++ Types | - | Deferred to v1.2 | 2026-02-17 |
| 10. Code Simplification | - | Won't Fix | 2026-02-17 |
| 11. Complexity & Memory | - | Won't Fix | 2026-02-17 |
| 12. Modern Diagnostics | - | Won't Fix | 2026-02-17 |
| 13. Duplications | 1/1 | Complete (1.9%) | 2026-02-17 |
| 14. Final Verification | - | Deferred to v1.2 | 2026-02-17 |

</details>

### v1.2 Code Modernization (COMPLETE)

<details>
<summary>v1.2 Progress</summary>

**Execution Order:**
Phases execute in numeric order: 15 -> 16 -> 17 -> 18 -> 19 -> 20

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 15. Critical Fix | 1/1 | Complete | 2026-02-17 |
| 16. Const Correctness | 2/3 | Complete | 2026-02-17 |
| 17. Modern Types | 1/3 | Complete | 2026-02-17 |
| 18. Code Quality | 2/2 | Complete | 2026-02-17 |
| 19. Documentation | 2/2 | Complete | 2026-02-17 |
| 20. Final Verification | 0/2 | Complete | 2026-02-17 |

</details>

### v1.3 Deep Modernization (COMPLETE)

<details>
<summary>v1.3 Progress</summary>

**Execution Order:**
Phases execute in numeric order: 21 -> 22 -> 23 -> 24 -> 25 -> 26 -> 27 -> 28 -> 29 -> 30

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 21. SonarQube Style | 0/3 | Complete | 2026-02-17 |
| 22. SonarQube Macros | 0/TBD | Complete | 2026-02-17 |
| 23. SonarQube Const | 0/TBD | Complete | 2026-02-17 |
| 24. SonarQube Nesting | 0/TBD | Complete | 2026-02-17 |
| 25. Code Complexity | 0/TBD | Complete | 2026-02-17 |
| 26. Code Duplicates | 0/1 | Complete | 2026-02-17 |
| 27. C++23 Features | 0/TBD | Complete | 2026-02-17 |
| 28. Diagnostics | 0/TBD | Complete | 2026-02-17 |
| 29. Build Verification | 0/TBD | Complete | 2026-02-17 |
| 30. Final Scan | 0/TBD | Complete | 2026-02-17 |

</details>

### v1.4 SonarQube Zero (IN PROGRESS)

**Execution Order:**
Phases execute in numeric order: 31 -> 32 -> 33 -> 34 -> 35 -> 36 -> 37 -> 38 -> 39 -> 40

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 31. Macro to constexpr | 1/1 | Complete    | 2026-02-18 |
| 32. Auto Conversion | 1/1 | Complete    | 2026-02-18 |
| 33. Independent Style Issues | 1/1 | Complete    | 2026-02-18 |
| 34. Const Correctness - Globals | 0/1 | Planned | - |
| 35. Const Correctness - Functions | 0/1 | Planned | - |
| 36. Complexity Reduction | 0/1 | Not started | - |
| 37. Nesting Reduction | 0/1 | Not started | - |
| 38. Init-statements | 0/1 | Planned | - |
| 39. Integration Changes | 0/1 | Planned | - |
| 40. Final Verification | 0/1 | Not started | - |

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

### v1.1 Requirements (COMPLETE)

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

### v1.2 Requirements (COMPLETE)

| Category | Requirements | Phase |
|----------|--------------|-------|
| Critical Fixes | CRIT-01 | Phase 15 |
| Const Correctness | CONST-01, CONST-02, CONST-03 | Phase 16 |
| Modern Types | TYPE-01, TYPE-02, TYPE-03 | Phase 17 |
| Code Quality | QUAL-01, QUAL-02 | Phase 18 |
| Documentation | DOC-01, DOC-02 | Phase 19 |
| Verification | VER-01, VER-02 | Phase 20 |

**Total v1.2 Coverage:** 13/13 requirements mapped (100%)

### v1.3 Requirements (COMPLETE)

| Category | Requirements | Phase |
|----------|--------------|-------|
| SonarQube Resolution | SONAR-01 | Phase 21 |
| SonarQube Resolution | SONAR-02 | Phase 22 |
| SonarQube Resolution | SONAR-03 | Phase 23 |
| SonarQube Resolution | SONAR-04 | Phase 24 |
| Code Refactoring | REFACT-01, REFACT-02 | Phase 25 |
| Code Refactoring | REFACT-03 | Phase 26 |
| C++23 Advanced Features | CPP23-01, CPP23-02, CPP23-03 | Phase 27 |
| Diagnostics & Logging | DIAG-01, DIAG-02, DIAG-03 | Phase 28 |
| Verification | VER-01 | Phase 29 |
| Verification | VER-02 | Phase 30 |

**Total v1.3 Coverage:** 15/15 requirements mapped (100%)

### v1.4 Requirements (IN PROGRESS)

| Category | Requirements | Phase |
|----------|--------------|-------|
| Macro Modernization | MACRO-01, MACRO-02, MACRO-03 | Phase 31 |
| Auto Modernization | AUTO-01, AUTO-02 | Phase 32 |
| Modern C++ Patterns | MODERN-02, MODERN-05, MODERN-06 | Phase 33 |
| Const Correctness | CONST-01, CONST-02 | Phase 34 |
| Const Correctness | CONST-03, CONST-04 | Phase 35 |
| Code Structure | STRUCT-01, STRUCT-02 | Phase 36 |
| Code Structure | STRUCT-03, STRUCT-04 | Phase 37 |
| Modern C++ Patterns | MODERN-01 | Phase 38 |
| Modern C++ Patterns | MODERN-03, MODERN-04 | Phase 39 |
| Verification | VER-01, VER-02, VER-03, VER-04 | Phase 40 |

**Total v1.4 Coverage:** 23/23 requirements mapped (100%)

---
*Roadmap created: 2026-02-15*
*v1.0 complete: 2026-02-16*
*v1.1 complete: 2026-02-17*
*v1.2 complete: 2026-02-17*
*v1.3 complete: 2026-02-18*
*v1.4 roadmap created: 2026-02-18*
*Phase 34 planned: 2026-02-18*
*Phase 35 planned: 2026-02-18*
*Phase 38 planned: 2026-02-18*
*Phase 31 complete: 2026-02-18*
*Phase 32 complete: 2026-02-18*
*Phase 33 complete: 2026-02-18*
