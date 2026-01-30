# SonarQube Linting Remediation Plan for EIDAuthentication
## Phase 1 Only: Simple Mechanical Fixes

## Overview
This document provides a focused plan for remediating **simple mechanical SonarQube linting issues** only. These are low-risk, high-volume fixes that modernize the codebase **without changing any functionality**.

**IMPORTANT:** This is a **linting-only task**. We are NOT adding or changing functionality. This is solely for code quality improvement.

**Scope:** Phase 1 only - Simple mechanical fixes
**Total Issues Addressed:** ~500+ (includes additional findings beyond HLO.md)
**Risk Level:** Low
**Estimated Complexity:** Simple

**Core Principle:**
> **NO FUNCTIONALITY CHANGES** - Only cosmetic/modernization changes that improve code quality without altering behavior.

## CRITICAL Severity Issues - Phase 1 Only (Mechanical Fixes)

Based on analysis of `sonarqube_issues/CRITICAL/`, there are **32 CRITICAL severity issues** total. This plan focuses only on the **20 simplest mechanical fixes** that can be safely applied one at a time:

### Phase 1 Critical Issues (Mechanical Fixes Only):
| Issue Type | Count | Risk |
|------------|-------|------|
| Use "nullptr" literal | 16 | Very Low |
| Global variables should be const | 2 | Low |
| Global pointers should be const | 2 | Low |
| **Total Phase 1** | **20** | |

### Critical Issues NOT Included in Phase 1:
| Issue Type | Count | Reason |
|------------|-------|--------|
| Refactor to reduce nesting depth | 5 | Requires code restructuring |
| Eliminate manual delete operations | 2 | Requires RAII/smart pointer changes |
| Reduce cognitive complexity | 1 | Requires function refactoring |
| **Total Excluded** | **8** | |

### Critical Issues by Component (Phase 1 Only):
| Component | Phase 1 Count | Total |
|-----------|---------------|-------|
| EIDConfigurationWizard/EIDConfigurationWizardPage05.cpp | 9 | 18 |
| EIDConfigurationWizard/EIDConfigurationWizardPage03.cpp | 11 | 11 |
| EIDConfigurationWizard/EIDConfigurationWizardPage06.cpp | 1 | 3 |
| EIDConfigurationWizard/CContainerHolder.cpp | 1 | 1 |

**Note:** See [`sonarqube_issues.md`](../sonarqube_issues.md) for detailed information on all critical issues including line numbers and links to SonarCloud.

## Additional Findings Beyond HLO.md

The HLO.md file only covered 200 of 2,385 total SonarQube issues (8.4%). Additional searches revealed many more instances:

### Additional `NULL` Usage
- Found **300+ occurrences** across all projects (HLO.md listed ~150)
- Additional files not in HLO.md: SmartCardModuleTest, PackageTest, EIDTestUtil, EIDTestInfo, CertificateValidationTest, helpers, CMessageCredential, Dll, common, EIDConfigurationWizardElevated, Tracing, smartcardmodule, Package, EIDSecuritySupportProviderUserMode, EIDSecuritySupportProvider, EIDLogManager, EIDPasswordChangeNotification

### Additional `#define` Macros
- Found **49 additional occurrences** in .cpp files (not listed in HLO.md)
- Found **62 additional occurrences** in .h files (not listed in HLO.md)
- Note: Many `#define` statements are for system compatibility (WIN32_NO_STATUS, SECURITY_WIN32, etc.) and should NOT be replaced

### Files to Skip (System/External Headers)
The following files contain system-level defines that should NOT be modified:
- `include/cardmod.h` - Contains smart card API defines
- `EIDPasswordChangeNotification/resources.h` - Contains resource IDs
- `EIDLogManager/resource.h` - Contains resource IDs
- Files with `WIN32_NO_STATUS`, `SECURITY_WIN32` defines - These are necessary for Windows API compatibility

### Files to Modify (Project-Specific Defines)
Only modify `#define` statements that are:
1. Project-specific constants (not system APIs)
2. Resource IDs that can be converted to enums
3. Magic numbers that should be named constants

---

## CRITICAL Issues - Phase 1 Detailed List

These are the 20 CRITICAL severity issues that can be addressed with simple mechanical fixes. Each should be assessed individually before application.

### EIDConfigurationWizard/EIDConfigurationWizardPage03.cpp (11 issues)
| Line | Issue | Type |
|------|-------|------|
| 260 | Use "nullptr" literal | NULL replacement |
| 279 | Use "nullptr" literal | NULL replacement |
| 300 | Use "nullptr" literal | NULL replacement |
| 338 | Use "nullptr" literal | NULL replacement |
| 384 | Use "nullptr" literal | NULL replacement |
| 405 | Use "nullptr" literal | NULL replacement |
| 406 | Use "nullptr" literal | NULL replacement |
| 407 | Use "nullptr" literal | NULL replacement |
| 414 | Use "nullptr" literal | NULL replacement |
| 416 | Use "nullptr" literal | NULL replacement |
| 357 | Refactor to not nest more than 3 statements | **EXCLUDED** (requires refactoring) |

### EIDConfigurationWizard/EIDConfigurationWizardPage05.cpp (9 issues)
| Line | Issue | Type |
|------|-------|------|
| 54 | Use "nullptr" literal | NULL replacement |
| 56 | Use "nullptr" literal | NULL replacement |
| 60 | Use "nullptr" literal | NULL replacement |
| 61 | Use "nullptr" literal | NULL replacement |
| 65 | Use "nullptr" literal | NULL replacement |
| 67 | Use "nullptr" literal | 2 occurrences |
| 144 | Use "nullptr" literal | NULL replacement |
| 193 | Use "nullptr" literal | NULL replacement |
| 17 | Global pointers should be const | const qualifier |
| 24 | Global variables should be const | const qualifier |
| 26 | Global variables should be const | const qualifier |
| 84 | Reduce cognitive complexity from 74 to 25 | **EXCLUDED** (requires refactoring) |
| 151 | Refactor to not nest more than 3 statements | **EXCLUDED** (requires refactoring) |
| 172 | Refactor to not nest more than 3 statements | **EXCLUDED** (requires refactoring) |
| 200 | Refactor to not nest more than 3 statements | **EXCLUDED** (requires refactoring) |
| 227 | Refactor to not nest more than 3 statements | **EXCLUDED** (requires refactoring) |
| 192 | Eliminate manual "delete" | **EXCLUDED** (requires RAII) |

### EIDConfigurationWizard/EIDConfigurationWizardPage06.cpp (1 issue)
| Line | Issue | Type |
|------|-------|------|
| 81 | Use "nullptr" literal | NULL replacement |
| 13 | Global pointers should be const | const qualifier |
| 80 | Eliminate manual "delete" | **EXCLUDED** (requires RAII) |

### EIDConfigurationWizard/CContainerHolder.cpp (1 issue)
| Line | Issue | Type |
|------|-------|------|
| 197 | Use "nullptr" literal | NULL replacement |

**Note:** Lines marked as **EXCLUDED** are not part of Phase 1 as they require code restructuring or RAII implementation.

---

## Issue Categories in Phase 1

1. Replace `NULL` with `nullptr` (~150 occurrences)
2. Replace `#define` macros with `const`/`constexpr`/`enum` (~50 occurrences)
3. Fix string continuation issues (2 occurrences)
4. Add `const` qualifier to global variables and pointers (~25 occurrences)
5. Move `default:` case to beginning/end of switch statements (1 occurrence)

---

## 1. Replace `NULL` with `nullptr`

**Rationale:** Modern C++ (C++11 and later) provides `nullptr` keyword which is type-safe and avoids ambiguity with integer overloads.

### Files to Modify (with occurrence counts):

#### EIDTest Project Files:
| File | Lines | Count |
|------|-------|-------|
| EIDCredentialProviderTest.cpp | 116, 268, 293, 309, 343, 344, 565, 570, 575, 597, 625, 629, 669, 674, 678, 681, 682, 699 | 17 |
| CContainerTest.cpp | 98, 99, 100, 107, 109, 151 | 6 |
| EIDSecuritySupportProviderTest.cpp | 20, 30, 49, 70, 72, 93, 97, 97, 97, 97, 97, 106, 142, 162 | 13 |
| CSmartCardNotifierTest.cpp | 33, 45, 49 | 3 |
| EIDTest.cpp | 62, 105, 111, 174, 374, 378, 382, 384 | 8 |
| EIDTestUIUtil.cpp | 129, 156, 214, 280, 281, 281, 281, 294, 311, 318, 318, 318, 318, 871, 885, 885, 886, 886, 886 | 17 |
| StoredCredentialManagementTest.cpp | 24 | 1 |
| Resource.h | 17, 18, 19, 20, 21, 27, 28 | 7 |

#### EIDConfigurationWizard Project Files:
| File | Lines | Count |
|------|-------|-------|
| EIDConfigurationWizardPage03.cpp | 12, 117, 209, 215, 216, 224, 225, 260, 279, 300, 338, 384, 405, 406, 407, 414, 416 | 17 |
| EIDConfigurationWizardPage05.cpp | 54, 56, 60, 61, 65, 67, 67, 144, 193 | 9 |
| EIDConfigurationWizardPage06.cpp | 81 | 1 |
| EIDConfigurationWizardPage07.cpp | 22, 66 | 2 |
| Common.cpp | 45, 74, 75, 85, 111 | 5 |
| DebugReport.cpp | 64, 123, 124, 125, 190, 193, 235 | 7 |
| EIDConfigurationWizardPage04.cpp | 21, 158, 178, 180, 182, 183, 186, 187, 310, 313, 314, 314, 596, 598 | 13 |
| EIDConfigurationWizard.cpp | 201, 202, 205 | 3 |
| EIDConfigurationWizardPage02.cpp | 33, 45, 47, 49, 60, 60, 67, 172, 176, 180, 182, 214 | 10 |
| CContainerHolder.cpp | 180, 197, 215, 408, 412, 416, 418 | 7 |

#### EIDCardLibrary Project Files:
| File | Lines | Count |
|------|-------|-------|
| CertificateUtilities.cpp | 127, 130 | 2 |
| GPO.cpp | 180, 195, 200, 209, 209, 209, 209, 209, 209 | 9 |
| CompleteToken.cpp | 54, 53, 250, 246, 263, 246, 300, 310, 313 | 9 |
| CSmartCardNotifier.cpp | 161, 193, 131, 425, 328, 395, 300, 319, 366, 298, 99, 227, 118, 105 | 16 |
| CContainerHolderFactory.cpp | 458 | 1 |
| Registration.cpp | 554, 554, 550, 554, 529, 105, 109, 104, 334, 353, 349, 309, 331 | 13 |
| StoredCredentialManagement.cpp | 58, 89, 66, 63 | 4 |

### Example Change:
```cpp
// Before
PCCERT_CONTEXT pRootCertificate = NULL;
if (pPointer != NULL) {
    return NULL;
}

// After
PCCERT_CONTEXT pRootCertificate = nullptr;
if (pPointer != nullptr) {
    return nullptr;
}
```

### Notes:
- This is a simple find-and-replace operation
- No logic changes required
- Compile-time type safety improvement

---

## 2. Replace `#define` Macros with `const`/`constexpr`/`enum`

**Rationale:** Macros lack type safety, don't respect scope, and can cause unexpected behavior. Use typed constants instead.

### Files to Modify:

#### EIDTest/EIDTest.h (lines 1-35)
**Issue:** 35 `#define` statements for resource IDs

**Solution:** Use `enum class` for resource IDs:
```cpp
// Before
#define IDC_STATIC -1
#define IDR_MAINFRAME 128
#define IDD_EIDCARDLIBRARYTEST_DIALOG 102
// ... 32 more defines

// After
namespace ResourceIDs {
    enum class DialogIDs : int {
        EIDCardLibraryTestDialog = 102,
        About = 104,
        MainFrame = 128,
        NameToToken = 103,
        Certificate = 15,
        Password = 1100,
        CSPInfo = 1200,
        Trace = 1300
    };

    enum class ControlIDs : int {
        Static = -1,
        Btn1 = 1001,
        Grp = 1009,
        Cbo1 = 21,
        Edt1 = 1001,
        Stc1 = 1002,
        Stc2 = 1003,
        Edt2 = 1004,
        Pin = 1101,
        Stc6 = 1102,
        PinCancel = 1103,
        PinOk = 1104,
        ProvType = 1201,
        ProvName = 1202,
        LstAlg = 1203,
        CspCancel = 1204,
        CspOk = 1205,
        Trace = 1301
    };

    enum class MenuIDs : int {
        About = 104,
        Exit = 105,
        StartWaitThread = 110,
        StopWaitThread = 111,
        WinBioStartWaitThread = 168,
        WinBioStopWaitThread = 167,
        APRegistration = 114,
        APLoad = 115,
        APProtect = 126,
        APToken = 112,
        APProfile = 116,
        APGPO = 122,
        APException = 177,
        CredList = 123,
        CredCSPInfo = 160,
        CredCert = 124,
        CredTile = 125,
        CredCom = 128,
        CredResetPass = 154,
        CredLsa = 153,
        CredSSPI = 162,
        CredNegotiate = 163,
        CredNTLM = 164,
        CredCredSSP = 165,
        CredUI = 113,
        CredUIAdmin = 141,
        CredOnlyEID = 155,
        CredOld = 157,
        CredSSPRegAdd = 171,
        CredSSPRegDel = 172,
        CredSSPRegAddSPN = 170,
        PassCheck = 153,
        PassCreate = 139,
        PassCreate2 = 131,
        PassUpdate = 132,
        PassDelete = 133,
        PassRetrive = 134,
        InfoHashNT = 180,
        RegAP = 144,
        UnregAP = 145,
        RegCP = 146,
        UnregCP = 147,
        RegPF = 148,
        UnregPF = 149,
        RegWiz = 150,
        UnregWiz = 151,
        UtilCert = 117,
        UtilImport = 142,
        UtilList = 119,
        UtilShowSD = 152,
        UtilDelete = 121,
        UtilClear = 118,
        InfoCSP = 129,
        InfoHashSHA1 = 135,
        SMKSP = 156,
        InfoTracing = 140
    };

    enum class StringIDs : int {
        AppTitle = 103
    };

    enum class IconIDs : int {
        MyIcon = 2,
        EIDCardLibraryTest = 107,
        Small = 108
    };
}
```

**Note:** This will require updating all references to use the scoped enum values (e.g., `ResourceIDs::DialogIDs::MainFrame`).

#### EIDTest/CompleteProfileTest.cpp (lines 1-162)
**Issue:** 62 `#define` statements for test constants

**Solution:** Use `constexpr`:
```cpp
// Before
#define TEST_CONST_1 100
#define TEST_CONST_2 200

// After
constexpr int TEST_CONST_1 = 100;
constexpr int TEST_CONST_2 = 200;
```

#### EIDTest/Resource.h (lines 2-49)
**Issue:** 48 `#define` statements for resource IDs

**Solution:** Similar to EIDTest/EIDTest.h - use `enum class`

#### EIDConfigurationWizard/CContainerHolder.h (lines 2-5)
**Issue:** 4 `#define` statements for container limits

**Solution:** Use `constexpr`:
```cpp
// Before
#define MAX_CONTAINERS 10
#define MIN_CONTAINERS 1
#define DEFAULT_CONTAINER 0
#define INVALID_CONTAINER -1

// After
constexpr size_t MAX_CONTAINERS = 10;
constexpr size_t MIN_CONTAINERS = 1;
constexpr size_t DEFAULT_CONTAINER = 0;
constexpr int INVALID_CONTAINER = -1;
```

#### EIDConfigurationWizard/EIDConfigurationWizardPage03.cpp (lines 15-17)
**Issue:** 3 `#define` statements for certificate validity

**Solution:** Use `constexpr`:
```cpp
// Before
#define DEFAULT_CERT_VALIDITY_YEARS 3
#define MAX_CERT_VALIDITY_YEARS 30
#define MIN_CERT_VALIDITY_YEARS 1

// After
constexpr WORD DEFAULT_CERT_VALIDITY_YEARS = 3;
constexpr WORD MAX_CERT_VALIDITY_YEARS = 30;
constexpr WORD MIN_CERT_VALIDITY_YEARS = 1;
```

#### EIDConfigurationWizard/EIDConfigurationWizard.h (lines 15-103)
**Issue:** 89 `#define` statements for wizard page IDs

**Solution:** Use `enum class` for wizard page IDs

#### EIDConfigurationWizard/CContainerHolder.cpp (lines 14-17)
**Issue:** 4 `#define` statements

**Solution:** Use `constexpr`

#### EIDTest/EIDTest.cpp (line 4)
**Issue:** 1 `#define` statement

**Solution:** Use `constexpr`

#### EIDTest/targetver.h (lines 11, 15, 19, 23)
**Issue:** 4 `#define` statements for version constants

**Solution:** Use `constexpr`

#### EIDCardLibrary/CertificateUtilities.cpp (lines 65, 89, 97)
**Issue:** 3 `#define` statements

**Solution:** Use `constexpr`

#### EIDCardLibrary/EIDCardLibrary.h (lines 25, 26, 30, 181, 235, 236)
**Issue:** 6 `#define` statements

**Solution:** Use `constexpr` or `enum`

#### EIDPasswordChangeNotification/EIDPasswordChangeNotification.cpp (line 21)
**Issue:** 1 `#define` statement

**Solution:** Use `constexpr`

#### EIDCardLibrary/CredentialManagement.cpp (line 2)
**Issue:** 1 `#define` statement

**Solution:** Use `constexpr`

---

## 3. Fix String Continuation Issues

**Issue:** CR+LF characters embedded in string literals

### Files to Modify:

#### EIDTest/EIDTest.h (line 10)
#### EIDTest/GPOTest.cpp (line 10)

### Solution:
```cpp
// Before - embedded CR+LF
TCHAR szMessage[] = "Line1
Line2";

// After - escaped sequence
TCHAR szMessage[] = "Line1\r\nLine2";

// Or - string continuation
TCHAR szMessage[] = "Line1"
                      "Line2";
```

---

## 4. Add `const` Qualifier to Global Variables and Pointers

**Rationale:** Global state should be immutable when possible to prevent unintended modifications.

### Files to Modify:

| File | Lines | Variable |
|------|-------|----------|
| EIDTest/CompleteProfileTest.cpp | 17 | Global pointer |
| EIDTest/Resource.h | 42 | Global pointer |
| EIDTest/EIDCredentialProviderTest.cpp | 25, 29 | Global pointer, global variable |
| EIDTest/CContainerTest.cpp | 12 | Global pointer |
| EIDTest/EIDSecuritySupportProviderTest.cpp | 17, 20, 21, 22, 23 | 2 global pointers, 3 global variables |
| EIDTest/CSmartCardNotifierTest.cpp | 28 | Global pointer |
| EIDTest/EIDTest.cpp | 53, 54 | 2 global pointers |
| EIDTest/EIDTestUIUtil.cpp | 667, 835 | 2 global pointers |
| EIDTest/StoredCredentialManagementTest.cpp | 13 | Global pointer |
| EIDConfigurationWizard/EIDConfigurationWizardPage03.cpp | 12 | Global pointer (pRootCertificate) |
| EIDConfigurationWizard/EIDConfigurationWizardPage05.cpp | 17, 24, 26 | Global pointer, 2 global variables |
| EIDConfigurationWizard/EIDConfigurationWizardPage06.cpp | 13 | Global pointer |
| EIDConfigurationWizard/EIDConfigurationWizardPage07.cpp | 14, 15, 16 | Global pointer, 2 global variables |
| EIDConfigurationWizard/Common.cpp | 10 | Global pointer |
| EIDConfigurationWizard/DebugReport.cpp | 227, 229 | Global pointer, global variable |
| EIDConfigurationWizard/global.h | 2, 4, 5, 7, 8, 9, 10, 11, 12 | 8 global variables |
| EIDConfigurationWizard/EIDConfigurationWizardPage04.cpp | 21 | Global pointer |
| EIDCardLibrary/CredentialManagement.cpp | 25 | Global variable |

### Example Change:
```cpp
// Before
PCCERT_CONTEXT pRootCertificate = NULL;
int g_GlobalValue = 0;

// After
PCCERT_CONTEXT const pRootCertificate = nullptr;
const int g_GlobalValue = 0;
```

### Notes:
- Only add `const` if the variable is not modified after initialization
- For pointers, use `const` at appropriate level:
  - `Type* const` - pointer value is const
  - `Type const*` - data pointed to is const
  - `Type const* const` - both are const

---

## 5. Move `default:` Case to Beginning/End of Switch Statements

**Issue:** `default:` case is in middle of switch statement

### File to Modify:
- EIDTest/EIDTestUIUtil.cpp (line 722)

### Solution:
```cpp
// Before - default in middle
switch (value) {
    case 1:
        // ...
        break;
    case 2:
        // ...
        break;
    default:
        // ...
        break;
    case 3:
        // ...
        break;
}

// After - default at end
switch (value) {
    case 1:
        // ...
        break;
    case 2:
        // ...
        break;
    case 3:
        // ...
        break;
    default:
        // ...
        break;
}
```

---

## Implementation Strategy

### Recommended Order:
1. **Start with CRITICAL `NULL` to `nullptr`** - 16 issues, highest priority
2. **Then CRITICAL `const` qualifiers** - 4 issues, high priority
3. **Then general `NULL` to `nullptr`** - ~130 remaining issues
4. **Then `#define` to `const`/`constexpr`/`enum`** - More involved but still mechanical
5. **Then general `const` qualifiers** - ~20 remaining issues
6. **Then string continuation** - Very few occurrences
7. **Finally switch statement** - Single occurrence

### Individual Assessment Approach:
**IMPORTANT:** Each change will be assessed individually before application to prevent introducing issues.

**Core Principle:** This is a **linting-only task**. We are NOT adding or changing functionality. This is solely for code quality improvement.

For each fix:
1. Read the source file and understand the context
2. Identify the exact line(s) to modify
3. **Analyze the code structure and dependencies:**
   - Check if the variable is used in a way that requires NULL specifically
   - Look for function calls that might have NULL-specific behavior
   - Check for macros or preprocessor directives that might depend on NULL
   - Verify the variable type and usage patterns
   - Look for any comments or documentation explaining why NULL is used
4. Verify the change is safe and won't affect logic or functionality
5. Apply the change
6. Verify the change was applied correctly
7. Build the project to ensure no compilation errors
8. **Confirm behavior is unchanged** (no functional changes)

### When NOT to Change NULL to nullptr:
**Do NOT replace NULL with nullptr if:**
- The code is interfacing with C APIs that expect NULL
- The code uses NULL in macro definitions or preprocessor conditions
- The NULL is used in variadic function arguments where type matters
- The code is in a header file that might be included by C code
- There are comments or documentation explicitly requiring NULL
- The NULL is used in a way that depends on its integer value (e.g., arithmetic)
- The variable is used with APIs that have NULL-specific behavior

### When NOT to Add const:
**Do NOT add const if:**
- The variable is modified after initialization
- The variable is passed to functions that modify it
- The variable is used in a way that requires mutability
- The variable is a global that might be modified by other code
- Adding const would cause compilation errors
- The variable is used in a way that depends on its non-const nature

### File-by-File Approach:
- Process one file at a time
- Build after each file to catch errors
- Commit changes after each file or small group of files

### Risk Mitigation:
1. **Individual Assessment:** Each change assessed before application
2. **Structure Analysis:** Analyze code structure and dependencies before changing
3. **Context Awareness:** Understand why the code is written this way
4. **Conservative Approach:** When in doubt, skip the change and document why
5. **Linting-Only Focus:** Remember - NO functionality changes, only code quality improvements
6. **Version Control:** Commit changes after each file or small group of files
7. **Build Validation:** Build project after each file
8. **Incremental Approach:** Don't try to fix all files at once
9. **CRITICAL Priority:** Address CRITICAL issues first
10. **Behavior Verification:** Confirm behavior is unchanged after each fix

### Code Structure Assessment Guidelines:

**When assessing NULL to nullptr changes:**
1. Look at the variable declaration and type
2. Trace all usages of the variable
3. Check function signatures the variable is passed to
4. Look for any comparisons or arithmetic operations
5. Check if the variable is used in preprocessor macros
6. Verify the code is C++ (not C)
7. Check if there are any comments explaining the usage

**When assessing const qualifier additions:**
1. Trace all assignments to the variable
2. Check if the variable is passed to non-const references/pointers
3. Look for any modifications after initialization
4. Check if the variable is used in loops or conditional branches
5. Verify the variable is not modified by called functions
6. Check for any comments explaining mutability requirements

**When to Skip a Change:**
- If the purpose is unclear
- If changing it might break API compatibility
- If the code is legacy and changing it is risky
- If the change would require extensive testing
- If there are comments warning against modification
- If the change affects public interfaces
- If the change is in a header included by C code

---

## Issues NOT Addressed in This Plan

The following issue types are **excluded** from this Phase 1-only plan:

### CRITICAL Issues Excluded (8 issues):
- Refactor to reduce nesting depth (5 issues) - Requires code restructuring
- Eliminate manual delete operations (2 issues) - Requires RAII/smart pointer changes
- Reduce cognitive complexity (1 issue) - Requires function refactoring

### Other Issue Types Excluded:
- Memory safety improvements (smart pointers, RAII)
- Code complexity refactoring
- Resource management fixes
- C-style casts
- Integral type issues
- Division by zero issues

These require more significant code changes and are higher risk. They should be addressed in a future phase after Phase 1 is complete and validated.

---

## Summary

**Total Issues to Fix:** ~150 of 200 (plus 20 CRITICAL issues)
**Files to Modify:** 19
**Risk Level:** Low
**Complexity:** Simple mechanical changes

**Phase 1 Scope:**
- Replace `NULL` with `nullptr` (~150 occurrences)
- Replace `#define` macros with `const`/`constexpr`/`enum` (~50 occurrences)
- Add `const` qualifier to global variables and pointers (~25 occurrences)
- Fix string continuation issues (2 occurrences)
- Move `default:` case to beginning/end of switch statements (1 occurrence)
- **CRITICAL mechanical fixes:** 20 additional issues (16 nullptr, 4 const)

**Benefits:**
- Modern C++ compliance
- Type safety improvements
- Better code maintainability
- Reduced SonarQube warnings
- Improved code quality

**NO FUNCTIONALITY CHANGES:**
> This is a **linting-only task**. We are NOT adding or changing functionality. This is solely for code quality improvement.

**No Logic Changes Required:**
- All fixes are mechanical
- No functional changes
- No API changes (except for enum-scoped resource IDs)
- Each change will be assessed individually before application
- Behavior remains identical before and after changes
