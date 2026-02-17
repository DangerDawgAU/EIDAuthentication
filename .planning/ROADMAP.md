# Roadmap: EIDAuthentication C++23 Modernization

## Overview

This roadmap transforms the EIDAuthentication Windows smart card authentication codebase from C++14 to C++23. The journey begins with build system configuration to enable C++23 compilation, progresses through modernizing error handling with `std::expected`, adopts compile-time enhancements, improves code quality with modern utilities, updates documentation, and culminates in comprehensive verification to ensure no regressions in the security-critical authentication flow.

## Milestones

- **v1.0 C++23 Modernization** - Phases 1-6 (COMPLETE 2026-02-16)
- **v1.1 SonarQube Quality Remediation** - Phases 7-14 (COMPLETE 2026-02-17)
- **v1.2 Code Modernization** - Phases 15-20 (IN PROGRESS)

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

### v1.2 Code Modernization (IN PROGRESS)

**Milestone Goal:** Resolve ~550 fixable SonarQube maintainability issues while documenting remaining ~550 as justified exceptions.

- [x] **Phase 15: Critical Fix** - Fix 1 blocker issue (unannotated fall-through) (completed 2026-02-17)
- [ ] **Phase 16: Const Correctness** - Fix 102+ const-correctness issues (globals, pointers, methods)
- [ ] **Phase 17: Modern Types** - Convert ~197 legacy type usages (C-style arrays, shadowing)
- [ ] **Phase 18: Code Quality** - Build verification after all code changes
- [ ] **Phase 19: Documentation** - Mark ~550 issues as "Won't Fix" with justification
- [ ] **Phase 20: Final Verification** - SonarQube confirmation all fixable issues resolved

---

## v1.2 Phase Details

### Phase 15: Critical Fix
**Goal**: Zero blocker issues remain in SonarQube scan
**Depends on**: Phase 14 (v1.1 Final Verification)
**Requirements**: CRIT-01
**Issues**: 1 total (blocker severity)
**Success Criteria** (what must be TRUE):
  1. SonarQube shows 0 blocker issues (fall-through resolved)
  2. Code compiles successfully after fix
  3. No new issues introduced by the fix
  4. Switch statement has explicit `[[fallthrough]]` annotation or case restructure
**Plans**: 1 plan

Plans:
- [ ] 15-01-PLAN.md - Add [[fallthrough]] annotation to EIDConfigurationWizardPage06.cpp

### Phase 16: Const Correctness
**Goal**: All global variables, pointers, and member functions are const-correct per SonarQube rules
**Depends on**: Phase 15
**Requirements**: CONST-01, CONST-02, CONST-03
**Issues**: 102+ total
**Success Criteria** (what must be TRUE):
  1. All global variables that should be const are marked const (71 issues)
  2. All global pointers have const at appropriate levels (31 issues)
  3. All member functions that don't modify state are marked const (remaining issues)
  4. Code compiles successfully after const changes
  5. No new warnings introduced by const additions
**Plans**: 3 plans

Plans:
- [ ] 16-01-PLAN.md - Mark global variables as const (71 issues across codebase)
- [ ] 16-02-PLAN.md - Fix global pointer const-correctness (31 issues)
- [ ] 16-03-PLAN.md - Mark member functions const where appropriate (remaining issues)

### Phase 17: Modern Types
**Goal**: Code uses modern C++ type system instead of C-style constructs where LSASS-safe
**Depends on**: Phase 16
**Requirements**: TYPE-01, TYPE-02, TYPE-03
**Issues**: ~197 total
**Success Criteria** (what must be TRUE):
  1. C-style char arrays converted to std::string where safe for LSASS (149 issues evaluated)
  2. C-style arrays converted to std::array (28 issues)
  3. Variable shadowing issues resolved (~20 issues)
  4. Code compiles successfully after type conversions
  5. No runtime regressions in LSASS-critical code paths
**Plans**: TBD

Plans:
- [ ] 17-01: Convert C-style char arrays to std::string where LSASS-safe (149 issues)
- [ ] 17-02: Convert C-style arrays to std::array (28 issues)
- [ ] 17-03: Resolve variable shadowing issues (~20 issues)

### Phase 18: Code Quality
**Goal**: Codebase builds cleanly after all v1.2 modernization changes
**Depends on**: Phase 17
**Requirements**: QUAL-01, QUAL-02
**Issues**: ~15 unused variable issues
**Success Criteria** (what must be TRUE):
  1. All 7 projects compile with zero errors using v143 toolset
  2. Unused variables removed (~15 issues)
  3. No new compiler warnings introduced by modernization
  4. Static CRT (`/MT`) preserved in all Release configurations
**Plans**: TBD

Plans:
- [ ] 18-01: Remove unused variables (~15 issues)
- [ ] 18-02: Full solution build verification

### Phase 19: Documentation
**Goal**: All justified exceptions documented in SonarQube with clear rationale
**Depends on**: Phase 18
**Requirements**: DOC-01, DOC-02
**Issues**: ~550 "Won't Fix" issues
**Success Criteria** (what must be TRUE):
  1. All ~550 "Won't Fix" issues have documented justification in SonarQube
  2. Justifications are categorized (Windows API, LSASS constraints, style preferences)
  3. VERIFICATION.md updated with v1.2 results and won't-fix summary
  4. Future maintainers can understand why each issue was not fixed
**Plans**: TBD

Plans:
- [ ] 19-01: Document ~550 "Won't Fix" issues in SonarQube with justification
- [ ] 19-02: Update VERIFICATION.md with v1.2 results

### Phase 20: Final Verification
**Goal**: SonarQube confirms all fixable issues resolved, milestone complete
**Depends on**: Phase 19
**Requirements**: VER-01, VER-02
**Success Criteria** (what must be TRUE):
  1. SonarQube scan shows 0 fixable issues remaining (all resolved or documented)
  2. No new issues introduced during v1.2 modernization
  3. Build still compiles cleanly with C++23 and v143 toolset
  4. Milestone sign-off documented
**Plans**: TBD

Plans:
- [ ] 20-01: Run SonarQube scan and verify all fixable issues resolved
- [ ] 20-02: Confirm no new issues introduced during modernization

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

### v1.2 Code Modernization (IN PROGRESS)

**Execution Order:**
Phases execute in numeric order: 15 -> 16 -> 17 -> 18 -> 19 -> 20

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 15. Critical Fix | 0/1 | Complete    | 2026-02-17 |
| 16. Const Correctness | 0/3 | Planned | - |
| 17. Modern Types | 0/TBD | Not started | - |
| 18. Code Quality | 0/TBD | Not started | - |
| 19. Documentation | 0/TBD | Not started | - |
| 20. Final Verification | 0/TBD | Not started | - |

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

### v1.2 Requirements (IN PROGRESS)

| Category | Requirements | Phase |
|----------|--------------|-------|
| Critical Fixes | CRIT-01 | Phase 15 |
| Const Correctness | CONST-01, CONST-02, CONST-03 | Phase 16 |
| Modern Types | TYPE-01, TYPE-02, TYPE-03 | Phase 17 |
| Code Quality | QUAL-01, QUAL-02 | Phase 18 |
| Documentation | DOC-01, DOC-02 | Phase 19 |
| Verification | VER-01, VER-02 | Phase 20 |

**Total v1.2 Coverage:** 13/13 requirements mapped (100%)

---
*Roadmap created: 2026-02-15*
*v1.0 complete: 2026-02-16*
*v1.1 complete: 2026-02-17*
*v1.2 roadmap created: 2026-02-17*
*Phase 16 plans created: 2026-02-17*
