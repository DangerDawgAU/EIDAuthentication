# SonarQube Issue Analysis - 2026-02-17

## Overview

| Metric | Value |
|--------|-------|
| Total Issues | 1,104 |
| Security Hotspots | 0 |
| Duplication Density | 1.9% |

### Severity Distribution

| Severity | Count | Percentage |
|----------|-------|------------|
| Blocker | 1 | 0.1% |
| High | 351 | 31.8% |
| Medium | 479 | 43.4% |
| Low | 273 | 24.7% |

**All issues are Maintainability (CODE_SMELL type)**

---

## Top 10 Issue Types

| Rank | Issue Type | Count | Severity |
|------|------------|-------|----------|
| 1 | Use "std::string" instead of C-style char array | 149 | Medium |
| 2 | Replace redundant type with "auto" | 126 | Low |
| 3 | Replace macro with "const", "constexpr" or "enum" | 111 | Low |
| 4 | Global variables should be const | 71 | High |
| 5 | Refactor nesting > 3 if/for/while/switch | 52 | High |
| 6 | Define each identifier in dedicated statement | 50 | Low |
| 7 | Global pointers should be const at every level | 31 | High |
| 8 | Use "std::array" or "std::vector" instead of C-style array | 28 | Medium |
| 9 | Merge "if" statement with enclosing one | 17 | Low |
| 10 | Fill compound statement or add nested comment | 17 | Low |

---

## Files with Most Issues (Top 15)

| File | Issues |
|------|--------|
| EIDCardLibrary/StoredCredentialManagement.cpp | 84 |
| EIDCardLibrary/CertificateUtilities.cpp | 78 |
| EIDConfigurationWizard/EIDConfigurationWizard.h | 68 |
| EIDAuthenticationPackage/EIDSecuritySupportProvider.cpp | 60 |
| EIDCardLibrary/CredentialManagement.cpp | 57 |
| EIDCardLibrary/Tracing.cpp | 53 |
| EIDConfigurationWizard/EIDConfigurationWizardPage03.cpp | 41 |
| EIDConfigurationWizard/EIDConfigurationWizardPage04.cpp | 40 |
| EIDCardLibrary/Package.cpp | 39 |
| EIDAuthenticationPackage/EIDAuthenticationPackage.cpp | 28 |
| EIDCredentialProvider/CEIDProvider.cpp | 27 |
| EIDCardLibrary/CSmartCardNotifier.cpp | 27 |
| EIDConfigurationWizard/DebugReport.cpp | 26 |
| EIDCredentialProvider/CEIDCredential.cpp | 25 |
| EIDCardLibrary/CContainerHolderFactory.cpp | 23 |

---

## Blocker Issue

**1 issue:** Unannotated fall-through between switch labels
- File: `EIDConfigurationWizard/EIDConfigurationWizardPage06.cpp`
- Line: 44
- Fix: Add `[[fallthrough]]` annotation or restructure switch

---

## Assessment: Fix vs "Won't Fix"

### Should Fix (High Impact)

#### Phase 1 - Quick Wins
| Issue Type | Count | Effort |
|------------|-------|--------|
| Blocker: fall-through annotation | 1 | Trivial |
| Global variables const correctness | 71 | Low |
| Global pointers const correctness | 31 | Low-Medium |
| Variable shadowing | ~20 | Low |

#### Phase 2 - Modernization
| Issue Type | Count | Effort |
|------------|-------|--------|
| C-style char array → std::string | 149 | Medium |
| C-style array → std::array/vector | 28 | Medium |
| Nesting depth reduction | 52 | Medium-High |

### Candidates for "Won't Fix"

#### Windows API Compatibility
- Const-pointer suggestions for Windows API structures (LSA_UNICODE_STRING, SecBuffer, etc.)
- C-style casts in Windows API interop
- __FUNCTION__, __FILE__, __LINE__ macros for tracing

#### Style Preferences (Low Impact)
- "Replace with auto" (126) - Style choice
- "Define each identifier separately" (50) - Minor style
- "Replace macro with const/constexpr" (111) - May be intentional

#### Risky Refactoring
- Functions with cognitive complexity > 50
- Deep nesting requiring significant restructuring

---

## Recommended Strategy

### Fixable Estimate
- High Severity: ~300 (85%)
- Medium Severity: ~200 (42%)
- Low Severity: ~50 (18%)
- **Total Fixable: ~550 issues (~50%)**

### Won't Fix Estimate
- Windows API compatibility: ~200
- Style preferences: ~200
- Risky refactoring: ~150
- **Total Won't Fix: ~550 issues (~50%)**

---

## Comparison to v1.1 Milestone

| Metric | v1.1 Target | Current |
|--------|-------------|---------|
| Security hotspots | 0 | 0 |
| Reliability bugs | 0 | 0 |
| Maintainability issues | ~0 | 1,104 |

The v1.1 milestone focused on security hotspots and reliability bugs - both are now at zero.
The 1,104 remaining issues are all **maintainability** (CODE_SMELL), not bugs or vulnerabilities.

---

## Next Steps

1. **Immediate:** Fix the 1 Blocker issue (fall-through annotation)
2. **Short-term:** Address const correctness for globals (~102 issues)
3. **Medium-term:** Create v1.2 milestone for modernization (std::string, std::array, nesting)
4. **Parallel:** Mark ~550 issues as "Won't Fix" with justifications
