# Feature Research: SonarQube Issue Remediation

**Domain:** C++23 SonarQube code quality fixes for Windows LSASS codebase
**Researched:** 2026-02-18
**Confidence:** HIGH (based on codebase analysis, C++ Core Guidelines, SonarQube rules)

---

## Executive Summary

This research covers best practices and modern C++23 techniques for fixing ~730 SonarQube issues in the EIDAuthentication codebase. The codebase is already on C++23 with MSVC, compiles cleanly, and has resolved all security issues. This milestone focuses purely on maintainability improvements (CODE_SMELL type issues).

**Key Findings:**
1. **Most issues are table stakes** - Standard C++23 modernization patterns with well-established solutions
2. **LSASS constraints drive decisions** - No exceptions, no dynamic allocation in hot paths, no STL across API boundaries
3. **Windows API compatibility limits options** - Some "fixes" are actually anti-patterns in this context
4. **Incremental approach essential** - Fix by category, not by file, to maintain consistency

---

## Feature Landscape

### Table Stakes (Standard Remediation Patterns)

These are well-established patterns for each SonarQube issue category. Every project should follow these.

| Issue Category | SonarQube Count | Why Expected | Complexity | LSASS Safety |
|----------------|-----------------|--------------|------------|--------------|
| **Global variable const correctness** | 102 | C++ core guideline; prevents accidental modification | LOW | Safe - compile-time check |
| **C-style cast removal** | ~50 | Type safety; prevents undefined behavior | LOW | Safe - use appropriate cast |
| **[[fallthrough]] annotation** | 1 (blocker) | C++17 standard; documents intentional fallthrough | LOW | Safe - explicit annotation |
| **Merge nested if statements** | 17 | Reduces nesting depth | LOW | Safe - preserves logic |
| **Empty block comments** | 17 | Documents intentional empty blocks | LOW | Safe - comment only |
| **Variable shadowing** | ~20 | Prevents logic errors | LOW | Safe - rename variable |

### Differentiators (Context-Dependent Remediation)

These require judgment based on LSASS constraints and Windows API compatibility.

| Issue Category | SonarQube Count | Value Proposition | Complexity | LSASS Safety |
|----------------|-----------------|-------------------|------------|--------------|
| **C-style array to std::array** | 28 | Bounds safety, size() method | MEDIUM | **CAUTION** - stack size, ABI |
| **C-style char array to std::string** | 149 | Memory safety, easier manipulation | MEDIUM | **CAUTION** - no STL across DLL |
| **Macro to constexpr** | 111 | Type safety, debugging, scope | MEDIUM | **CAUTION** - some macros required |
| **Replace redundant type with auto** | 126 | Reduces duplication | LOW | **CAUTION** - may obscure types |
| **Deep nesting reduction** | 52 | Readability, maintainability | MEDIUM-HIGH | Safe - preserves logic |
| **Define identifiers separately** | 50 | Readability, easier debugging | LOW | Safe - simple refactor |
| **Range-based for loops** | ~30 | Simpler iteration, fewer bugs | LOW | Safe - equivalent logic |
| **enum class conversion** | ~20 | Type safety, scoped enumerators | MEDIUM | **CAUTION** - Windows API interop |
| **Init-statements in if/switch** | ~15 | Limits variable scope | LOW | Safe - scope management |
| **auto usage clarity** | ~40 | Context-dependent; may help or hurt | LOW | **CAUTION** - type clarity |

### Anti-Features (What NOT to Do)

Patterns that SonarQube suggests but should be avoided in this LSASS codebase.

| Anti-Feature | Why Suggested | Why Problematic Here | What to Do Instead |
|--------------|---------------|----------------------|-------------------|
| **std::string across DLL boundaries** | Modern C++ practice | ABI incompatibility; memory allocator mismatch | Keep C-style strings at API boundaries |
| **std::vector for all arrays** | Bounds checking, dynamic sizing | Heap allocation in LSASS; stack preferred | Use std::array for fixed-size, evaluate case-by-case |
| **Eliminate all macros** | Type safety, scoping | Windows API requires some (WIN32_NO_STATUS, SECURITY_WIN32) | Convert only type/constant macros; keep flow-control macros |
| **Always use auto** | Reduces redundancy | Obscures types in security-critical code (HRESULT vs NTSTATUS) | Use auto for iterators/long types; explicit for domain types |
| **Aggressive refactoring for complexity** | Reduces cognitive load | Risk of introducing bugs in security-critical paths | Extract helpers judiciously; extensive testing required |
| **enum class everywhere** | Type safety | Windows APIs use plain enums with specific values | Convert internal enums; keep Windows API enums as-is |

---

## Detailed Remediation Guide by Category

### 1. Global Variable Const Correctness (102 issues)

**SonarQube Rule:** Global variables should be const

**Why it matters:** Prevents accidental modification, enables compiler optimizations, documents intent.

**Before (typical pattern in codebase):**
```cpp
// CertificateValidation.cpp:34
static char s_szOidSmartCardLogon[] = szOID_KP_SMARTCARD_LOGON;
// CertificateUtilities.cpp:36-44
static char s_szOidClientAuth[] = szOID_PKIX_KP_CLIENT_AUTH;
static char s_szOidServerAuth[] = szOID_PKIX_KP_SERVER_AUTH;
// ... more OID arrays
```

**After (Windows API compatible fix):**
```cpp
// These MUST remain non-const because Windows API (CERT_ENHKEY_USAGE.rgpszUsageIdentifier)
// requires LPSTR* (non-const char**). This is intentional for Windows API compatibility.
// Comment documents WHY it's not const, satisfying code review requirements.
static char s_szOidSmartCardLogon[] = szOID_KP_SMARTCARD_LOGON;  // Non-const for LPSTR* compatibility

// For truly constant globals (not passed to Windows APIs):
static constexpr TCHAR REMOVALPOLICYKEY[] = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Removal Policy");
// Or:
static const LPCWSTR ALLOWED_PROVIDERS[] = { /* ... */ };
```

**Decision Matrix:**
| Condition | Fix | Mark Won't Fix? |
|-----------|-----|-----------------|
| Passed to Windows API requiring non-const | Add comment explaining why | Yes, with comment |
| Used internally only | Make `static constexpr` or `static const` | No, fix it |
| Modified at runtime | No change needed (not a SonarQube issue) | N/A |

**LSASS Safety:** HIGH - Compile-time check only, no runtime impact.

---

### 2. C-Style Cast Removal (~50 issues)

**SonarQube Rule:** C-style casts should not be used

**Why it matters:** C-style casts can silently perform dangerous conversions. Named casts document intent.

**Before:**
```cpp
// CertificateValidation.cpp:116
if (!CryptGetKeyParam(phUserKey, KP_CERTIFICATE, (BYTE*)Data, &DataSize, 0))

// CertificateUtilities.cpp:639
CertEnhKeyUsage.rgpszUsageIdentifier = (LPSTR*) EIDAlloc(sizeof(LPSTR)*CertEnhKeyUsage.cUsageIdentifier);

// CContainer.cpp:52
LPTSTR szResult = (LPTSTR) EIDAlloc((DWORD)(sizeof(TCHAR) * (len + 1)));
```

**After:**
```cpp
// Use reinterpret_cast for pointer type conversions (Windows API interop)
if (!CryptGetKeyParam(phUserKey, KP_CERTIFICATE, reinterpret_cast<BYTE*>(Data), &DataSize, 0))

// Use static_cast for related type conversions
CertEnhKeyUsage.rgpszUsageIdentifier = static_cast<LPSTR*>(EIDAlloc(sizeof(LPSTR) * CertEnhKeyUsage.cUsageIdentifier));

// For memory allocation, consider a helper to reduce boilerplate:
template<typename T>
T* eid_alloc(size_t count) {
    return static_cast<T*>(EIDAlloc(static_cast<DWORD>(sizeof(T) * count)));
}
// Usage:
LPTSTR szResult = eid_alloc<TCHAR>(len + 1);
```

**Decision Matrix:**
| Cast Type | C++ Cast | Notes |
|-----------|----------|-------|
| Pointer to pointer (same base) | `static_cast<To*>` | Related pointer types |
| Pointer to pointer (unrelated) | `reinterpret_cast<To*>` | Windows API void* |
| Removing const | `const_cast<To>` | **RARE** - usually bug |
| Numeric conversion | `static_cast<To>` | With narrowing check |
| Any combination | Named cast | Never C-style |

**LSASS Safety:** HIGH - Equivalent code, clearer intent.

---

### 3. Macro to Constexpr Conversion (111 issues)

**SonarQube Rule:** Replace macro with "const", "constexpr" or "enum"

**Why it matters:** Macros have no scope, no type, no debugging info, and can cause unexpected behavior.

**When SAFE to convert:**
```cpp
// Before:
#define MAX_CONTAINER_NAME_LENGTH 1024
#define MAX_READER_NAME_LENGTH 256

// After (already done in some files):
constexpr DWORD MAX_CONTAINER_NAME_LENGTH = 1024;
constexpr DWORD MAX_READER_NAME_LENGTH = 256;
```

**When NOT SAFE to convert:**
```cpp
// These MUST remain macros - they control Windows header behavior
#define WIN32_NO_STATUS 1  // Controls ntstatus.h inclusion behavior
#define SECURITY_WIN32    // Controls sspi.h behavior
#define _CRTDBG_MAP_ALLOC // Controls CRT debug allocation

// Flow-control macros used with __try/__leave/__finally
#define CHECK_DWORD(_X) { if (ERROR_SUCCESS != (status = (_X))) { __leave; } }
// Cannot be constexpr - it's control flow, not a value

// Tracing macros that use __FILE__, __LINE__, __FUNCTION__
#define EIDCardLibraryTrace(level, format, ...) \
    EIDCardLibraryTraceEx(__FILE__, __LINE__, __FUNCTION__, level, format, __VA_ARGS__)
// These must be macros to capture source location
```

**Decision Matrix:**
| Macro Type | Convertible? | Replacement |
|------------|--------------|-------------|
| Constant value | Yes | `constexpr` or `const` |
| Type alias | Yes | `using` alias |
| Function-like (simple) | Maybe | `constexpr` function |
| Function-like (control flow) | No | Keep as macro |
| Token pasting/stringizing | No | Keep as macro |
| Conditional compilation | No | Keep as macro |
| Windows header control | No | Keep as macro |
| Source location capture | No | Keep as macro |

**LSASS Safety:** MEDIUM - Ensure constexpr functions are truly constexpr (no hidden runtime costs).

---

### 4. Deep Nesting Reduction (52 issues)

**SonarQube Rule:** Refactor code with nesting depth > 3

**Why it matters:** Deep nesting reduces readability and increases cognitive load.

**Pattern 1: Early Return / Guard Clause**

**Before:**
```cpp
NTSTATUS DoSomething(Parameter* p) {
    NTSTATUS status = STATUS_SUCCESS;
    __try {
        if (p != nullptr) {
            if (p->IsValid()) {
                if (p->HasPermission()) {
                    // ... actual work at depth 4+
                    if (SomethingElse()) {
                        // ... even deeper
                    }
                }
            }
        }
    } __finally {
        // cleanup
    }
    return status;
}
```

**After:**
```cpp
NTSTATUS DoSomething(Parameter* p) {
    // Guard clauses at top - fail fast
    if (p == nullptr) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!p->IsValid()) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!p->HasPermission()) {
        return STATUS_ACCESS_DENIED;
    }

    // Main logic at reduced depth
    __try {
        // ... actual work at depth 1-2
    } __finally {
        // cleanup
    }
    return STATUS_SUCCESS;
}
```

**Pattern 2: Extract Helper Function**

**Before:**
```cpp
void ProcessCertificates() {
    for (auto& cert : certificates) {
        if (cert.IsValid()) {
            for (auto& chain : cert.Chains()) {
                if (chain.IsTrusted()) {
                    // ... nested processing
                }
            }
        }
    }
}
```

**After:**
```cpp
// Extract nested logic to helper
static bool ProcessChainIfNeeded(const Chain& chain) {
    if (!chain.IsTrusted()) {
        return false;
    }
    // ... chain processing
    return true;
}

static bool ProcessCertificateIfNeeded(const Certificate& cert) {
    if (!cert.IsValid()) {
        return false;
    }
    for (const auto& chain : cert.Chains()) {
        ProcessChainIfNeeded(chain);
    }
    return true;
}

void ProcessCertificates() {
    for (const auto& cert : certificates) {
        ProcessCertificateIfNeeded(cert);
    }
}
```

**Pattern 3: Continue/Break in Loops**

**Before:**
```cpp
for (DWORD i = 0; i < count; i++) {
    if (items[i].IsValid()) {
        if (items[i].HasData()) {
            // process
        }
    }
}
```

**After:**
```cpp
for (DWORD i = 0; i < count; i++) {
    if (!items[i].IsValid()) {
        continue;  // Early skip
    }
    if (!items[i].HasData()) {
        continue;  // Early skip
    }
    // process at lower depth
}
```

**LSASS Safety:** HIGH - Logic equivalence is verifiable. Extract helpers must follow LSASS rules (no exceptions, careful allocation).

---

### 5. C-Style Array to std::array (28 issues)

**SonarQube Rule:** Use "std::array" or "std::vector" instead of C-style array

**Why it matters:** Bounds safety, `.size()` method, iterators, no pointer decay.

**When SAFE to convert:**
```cpp
// Before:
BYTE Data[4096];
DWORD DataSize = ARRAYSIZE(Data);

// After:
std::array<BYTE, 4096> Data{};
DWORD DataSize = static_cast<DWORD>(Data.size());

// Even better with C++23:
std::array<BYTE, 4096> Data{};
DWORD DataSize = std::to_underlying(Data.size());  // If size_t to DWORD
```

**When NOT SAFE to convert:**
```cpp
// Fixed-size buffers for Windows API - evaluate stack impact
WCHAR szUserName[256];  // 512 bytes on stack
// std::array<WCHAR, 256> has same stack size, so OK

// But large buffers may need to stay as-is:
BYTE CertificateBuffer[65536];  // 64KB on stack
// This is already questionable - might need heap allocation anyway

// Arrays in structures for Windows API compatibility:
struct EID_SMARTCARD_CSP_INFO {
    DWORD dwCspInfoLen;
    // ... other fields ...
    BYTE bBuffer[ANYSIZE_ARRAY];  // Must remain C-array for Windows API
};
```

**Decision Matrix:**
| Condition | Convert? | Notes |
|-----------|----------|-------|
| Local fixed-size buffer < 4KB | Yes | Same stack size, safer API |
| Local fixed-size buffer > 4KB | Evaluate | May need heap anyway |
| Windows API structure member | No | Must match Windows layout |
| Global/static array | Maybe | std::array has same storage |
| Passed to API expecting pointer | Yes | `.data()` method |

**LSASS Safety:** MEDIUM - Verify stack usage. Large std::array still consumes stack.

---

### 6. C-Style Char Array to std::string (149 issues)

**SonarQube Rule:** Use "std::string" instead of C-style char array

**Why it matters:** Memory safety, automatic cleanup, easier manipulation.

**CRITICAL LSASS CONSTRAINT:**
```cpp
// NEVER pass std::string across DLL boundaries
// NEVER pass std::string to/from LSA functions

// WRONG - will cause heap corruption:
HRESULT WINAPI GetUserName(__out std::wstring& name) {
    name = L"username";  // Allocates in wrong heap!
}

// CORRECT - C-style at boundary, std::string internally:
HRESULT WINAPI GetUserName(__out_ecount(cchName) LPWSTR name, __in DWORD cchName) {
    std::wstring internalName = GetUserNameInternal();
    wcscpy_s(name, cchName, internalName.c_str());
    return S_OK;
}
```

**Pattern for LSASS:**
```cpp
// Internal function - use std::string/std::wstring
std::expected<std::wstring, HRESULT> GetUserNameInternal() noexcept {
    std::wstring name;
    name.reserve(256);
    // ... build name ...
    return name;
}

// Exported function - C-style interface
HRESULT WINAPI GetUserName(__out_ecount(cchName) LPWSTR name, __in DWORD cchName) {
    if (!name || cchName == 0) return E_POINTER;

    auto result = GetUserNameInternal();
    if (!result) {
        return result.error();
    }

    const auto& internalName = *result;
    if (internalName.size() + 1 > cchName) {
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    wcscpy_s(name, cchName, internalName.c_str());
    return S_OK;
}
```

**Decision Matrix:**
| Location | Convert? | Pattern |
|----------|----------|---------|
| Internal helper function | Yes | Use `std::wstring` freely |
| Function parameters (internal) | Yes | `std::wstring_view` for read-only |
| Function parameters (exported) | No | Keep `LPWSTR` / `LPCWSTR` |
| Return value (internal) | Yes | `std::expected<std::wstring, E>` |
| Return value (exported) | No | Output parameter + `LPWSTR` |
| Class member | Yes | `std::wstring` with proper cleanup |
| Global | No | Security issue anyway |

**LSASS Safety:** MEDIUM - Strict boundary discipline required. Review all cross-DLL usages.

---

### 7. Auto Usage Clarity (126 + ~40 issues)

**SonarQube Rule:** Replace redundant type with "auto" / Use auto appropriately

**Why it matters:** Can reduce redundancy or obscure types, depending on context.

**When auto IMPROVES clarity:**
```cpp
// Iterators - obvious type from context
for (auto it = container.begin(); it != container.end(); ++it)

// Factory functions where return type is clear
auto context = CertCreateCertificateContext(...);  // Obviously PCCERT_CONTEXT
auto lock = std::unique_lock(mutex);  // Obviously unique_lock

// Long, complex types that obscure intent
auto callback = static_cast<PRShowRestoreFromMsginaW>(
    GetProcAddress(keymgrDll, "PRShowRestoreFromMsginaW"));

// Range-based for
for (const auto& cert : certificates)
```

**When auto REDUCES clarity (avoid):**
```cpp
// Security-critical types - be explicit about HRESULT vs NTSTATUS vs DWORD
HRESULT hr = DoSomething();  // GOOD: Explicitly HRESULT
NTSTATUS status = NtDoSomething();  // GOOD: Explicitly NTSTATUS
auto result = DoSomething();  // BAD: What type? What error handling?

// Numeric types where precision matters
DWORD size = GetDataSize();  // GOOD: Explicitly 32-bit
auto size = GetDataSize();  // BAD: Is it size_t? DWORD? int?

// Pointer ownership semantics
PCCERT_CONTEXT cert = CertDuplicateCertificateContext(...);  // GOOD: Must call CertFree
auto cert = CertDuplicateCertificateContext(...);  // BAD: Ownership unclear

// Boolean return values in security context
BOOL success = CheckAccess(...);  // GOOD: Explicitly BOOL, not int
bool allowed = IsAllowed(...);  // GOOD: Explicitly bool
auto result = CheckAccess(...);  // BAD: Is it BOOL, bool, HRESULT?
```

**Decision Matrix:**
| Context | Use auto? | Rationale |
|---------|-----------|-----------|
| Iterators | Yes | Redundant type |
| Factory result (type obvious) | Yes | Type clear from call |
| Complex template type | Yes | Type less important than behavior |
| Error/success type | **No** | Critical to see HRESULT vs bool |
| Numeric calculation | **No** | Type affects precision |
| Pointer with ownership | **No** | Ownership must be clear |
| Security decision result | **No** | Must see type for audit |

**LSASS Safety:** MEDIUM - Type clarity critical for security review.

---

### 8. Init-Statements in If/Switch (C++17) (~15 issues)

**SonarQube Rule:** (Various) Limit variable scope

**Why it matters:** Variables only exist where needed, reduces scope for bugs.

**Pattern: if init-statement**
```cpp
// Before:
{
    HANDLE hToken = GetToken();
    if (hToken != nullptr) {
        // use hToken
        CloseHandle(hToken);
    }
}

// After (C++17):
if (HANDLE hToken = GetToken(); hToken != nullptr) {
    // use hToken
    CloseHandle(hToken);
}
// hToken out of scope here
```

**Pattern: switch init-statement**
```cpp
// Before:
{
    auto status = GetStatus();
    switch (status) {
        case STATUS_SUCCESS: /* ... */ break;
        // ...
    }
}

// After (C++17):
switch (auto status = GetStatus(); status) {
    case STATUS_SUCCESS: /* ... */ break;
    // ...
}
// status out of scope
```

**LSASS Safety:** HIGH - Pure scope reduction, no behavior change.

---

### 9. Range-Based For Loops (~30 issues)

**SonarQube Rule:** (Various) Use range-based for where appropriate

**Why it matters:** Simpler syntax, fewer off-by-one errors, automatic bounds.

**Before:**
```cpp
for (DWORD dwI = 0; dwI < dwEntriesRead; dwI++) {
    ProcessUser(pUserInfo[dwI].usri3_name);
}
```

**After:**
```cpp
// If we can create a span/view:
auto users = std::span(pUserInfo, dwEntriesRead);
for (const auto& user : users) {
    ProcessUser(user.usri3_name);
}

// Or with pointer arithmetic:
for (DWORD i = 0; i < dwEntriesRead; i++) {
    const auto& user = pUserInfo[i];
    ProcessUser(user.usri3_name);
}
```

**When NOT to use range-based for:**
```cpp
// Need index for other purposes
for (DWORD i = 0; i < count; i++) {
    if (NeedIndex(i)) {
        DoSomethingWithIndex(i, items[i]);
    }
}

// Modifying container size during iteration
for (auto it = items.begin(); it != items.end(); ) {
    if (ShouldRemove(*it)) {
        it = items.erase(it);  // Range-based for would break
    } else {
        ++it;
    }
}

// Windows API array with explicit count
// Keep as-is if the count is needed for API calls
```

**LSASS Safety:** HIGH - Equivalent logic, simpler code.

---

### 10. Enum Class Conversion (~20 issues)

**SonarQube Rule:** Use enum class instead of plain enum

**Why it matters:** Type safety, scoped enumerators, no implicit conversions.

**When SAFE to convert:**
```cpp
// Before:
enum PolicyValue {
    AllowSignatureOnlyKeys = 0,
    AllowCertificatesWithNoEKU = 1,
    EnforceCSPWhitelist = 2
};

// After:
enum class PolicyValue : DWORD {
    AllowSignatureOnlyKeys = 0,
    AllowCertificatesWithNoEKU = 1,
    EnforceCSPWhitelist = 2
};

// Usage requires explicit scoping:
if (GetPolicyValue(PolicyValue::AllowSignatureOnlyKeys) == 0)

// Conversion to underlying type:
DWORD value = std::to_underlying(PolicyValue::AllowSignatureOnlyKeys);
// Or pre-C++23:
DWORD value = static_cast<DWORD>(PolicyValue::AllowSignatureOnlyKeys);
```

**When NOT SAFE to convert:**
```cpp
// Windows API enums - must match Windows definition
enum _SECURITY_LOGON_TYPE {
    Interactive = 2,
    Network = 3,
    // ...
};

// Don't convert - Windows API expects this exact type
```

**Decision Matrix:**
| Condition | Convert? | Notes |
|-----------|----------|-------|
| Internal-only enum | Yes | Type safety benefits |
| Used with Windows API | No | Must match Windows type |
| Values serialized/stored | Maybe | Need explicit conversion |
| Implicit conversion relied upon | No | Would break code |

**LSASS Safety:** MEDIUM - Verify no implicit conversions relied upon.

---

### 11. [[fallthrough]] Annotation (1 blocker issue)

**SonarQube Rule:** Switch cases should end with unconditional break/return/[[fallthrough]]

**Why it matters:** Documents intentional fallthrough; catches accidental fallthrough.

**Before:**
```cpp
switch (status) {
    case STATUS_SUCCESS:
        // Do success processing
    case STATUS_PENDING:
        // Handle both success and pending
        break;
}
```

**After:**
```cpp
switch (status) {
    case STATUS_SUCCESS:
        // Do success processing
        [[fallthrough]];  // C++17 attribute
    case STATUS_PENDING:
        // Handle both success and pending
        break;
}
```

**LSASS Safety:** HIGH - Zero runtime impact, pure annotation.

---

### 12. Cognitive Complexity Reduction (52+ issues)

**SonarQube Rule:** (Cognitive Complexity) Reduce cognitive complexity

**Why it matters:** Highly complex code is hard to review, test, and maintain.

**Definition:** Cognitive complexity measures:
- Nesting depth (adds to score)
- Break/continue in loops (adds to score)
- Multiple conditions (adds to score)
- else/else if (adds to score)

**Remediation Strategies:**

1. **Extract functions** - Move complex logic to well-named helpers
2. **Use guard clauses** - Return early to reduce nesting
3. **Use lookup tables** - Replace complex switch/if chains with data
4. **Polymorphism** - Replace type-checking conditionals with virtual calls (limited in LSASS)

**Example - Lookup Table Pattern:**
```cpp
// Before: High cognitive complexity
LPCTSTR GetTrustErrorText(DWORD Status) {
    LPCTSTR pszName = nullptr;
    switch(Status) {
        ERRORTOTEXT(CERT_E_EXPIRED)
        ERRORTOTEXT(CERT_E_VALIDITYPERIODNESTING)
        ERRORTOTEXT(CERT_E_ROLE)
        // ... 20+ more cases ...
        default:
            pszName = nullptr;
            break;
    }
    return pszName;
}

// After: Lower cognitive complexity with lookup table
// (This is actually a good use of a macro for the lookup table!)
static const struct {
    DWORD error;
    LPCWSTR name;
} ErrorNames[] = {
    { CERT_E_EXPIRED, L"CERT_E_EXPIRED" },
    { CERT_E_VALIDITYPERIODNESTING, L"CERT_E_VALIDITYPERIODNESTING" },
    // ...
};

LPCTSTR GetTrustErrorText(DWORD Status) {
    for (const auto& entry : ErrorNames) {
        if (entry.error == Status) {
            return entry.name;
        }
    }
    return nullptr;
}
```

**LSASS Safety:** MEDIUM - Complex refactoring needs careful review and testing.

---

## Feature Dependencies

```
Global const correctness
    └── Independent - can be done in any order

Macro to constexpr
    ├── requires: Identifying Windows API macros (don't convert)
    └── enables: Type-safe constants

Deep nesting reduction
    ├── requires: Understanding __try/__finally semantics
    ├── enables: Easier maintenance
    └── conflicts: With aggressive inlining

std::array conversion
    ├── requires: Stack size analysis
    ├── enables: Bounds checking
    └── conflicts: With Windows API fixed-layout structures

std::string conversion
    ├── requires: Identifying all DLL boundary crossings
    ├── enables: Memory safety internally
    └── conflicts: With LSA/SSPI API requirements

C-style cast removal
    ├── requires: Understanding pointer type relationships
    └── enables: Type-safe casts

enum class conversion
    ├── requires: Identifying Windows API enums
    └── conflicts: With Windows API enums
```

---

## MVP Definition

### Phase 1: Quick Wins (Fix First)

Low-risk, high-value fixes with clear patterns.

- [x] **[[fallthrough]] annotation** - 1 blocker, trivial fix
- [x] **Global const correctness** - 102 issues, most are simple
- [x] **Empty block comments** - 17 issues, trivial
- [x] **Merge nested if** - 17 issues, low risk
- [x] **C-style cast removal** - ~50 issues, clear replacements

**Estimated: ~187 issues, 2-3 days**

### Phase 2: Standard Modernization (After Phase 1)

Medium-risk fixes with established patterns.

- [x] **Macro to constexpr** - 111 issues, need Windows API awareness
- [x] **Define identifiers separately** - 50 issues, simple refactor
- [x] **Init-statements in if/switch** - ~15 issues, C++17 feature
- [x] **Range-based for loops** - ~30 issues, equivalent logic
- [x] **Variable shadowing** - ~20 issues, rename variables
- [x] **Selective auto usage** - ~60 issues (half of auto suggestions)

**Estimated: ~286 issues, 3-5 days**

### Phase 3: Complex Refactoring (After Phase 2)

Higher-risk fixes requiring careful review.

- [x] **Deep nesting reduction** - 52 issues, logic restructuring
- [x] **Cognitive complexity** - 52+ issues, extract helpers
- [x] **C-style array to std::array** - 28 issues, stack size analysis
- [x] **C-style char to std::string** - 149 issues, API boundary care
- [x] **enum class conversion** - ~20 issues, Windows API interop

**Estimated: ~301 issues, 5-10 days**

### Phase 4: Won't Fix (Mark with Justification)

Issues that should not be fixed due to Windows API or LSASS constraints.

- [x] **Global non-const for Windows API** - ~40 issues - Windows API requires non-const
- [x] **Flow-control macros** - ~20 issues - Cannot be constexpr
- [x] **Windows header macros** - ~10 issues - Required for Windows headers
- [x] **Auto for security types** - ~60 issues - Type clarity needed
- [x] **Windows API enum types** - ~10 issues - Must match Windows

**Estimated: ~140 issues to mark Won't Fix**

---

## Feature Prioritization Matrix

| Category | Value | Risk | Volume | Priority |
|----------|-------|------|--------|----------|
| [[fallthrough]] annotation | HIGH | LOW | 1 | **P1** |
| Global const correctness | HIGH | LOW | 102 | **P1** |
| C-style cast removal | HIGH | LOW | ~50 | **P1** |
| Empty block comments | LOW | LOW | 17 | **P1** |
| Merge nested if | MEDIUM | LOW | 17 | **P1** |
| Macro to constexpr | MEDIUM | LOW | 111 | **P2** |
| Define identifiers separately | LOW | LOW | 50 | **P2** |
| Variable shadowing | MEDIUM | LOW | ~20 | **P2** |
| Init-statements | MEDIUM | LOW | ~15 | **P2** |
| Range-based for | MEDIUM | LOW | ~30 | **P2** |
| Deep nesting reduction | HIGH | MEDIUM | 52 | **P3** |
| Cognitive complexity | HIGH | MEDIUM | 52+ | **P3** |
| std::array conversion | MEDIUM | MEDIUM | 28 | **P3** |
| std::string conversion | HIGH | MEDIUM | 149 | **P3** |
| enum class conversion | MEDIUM | MEDIUM | ~20 | **P3** |
| Auto usage (selective) | LOW | LOW | ~60 | **P2** |

**Priority key:**
- **P1:** Must fix - Low risk, clear patterns
- **P2:** Should fix - Some judgment needed
- **P3:** Careful fix - Review and testing required

---

## Competitor/Reference Analysis

| Pattern | C++ Core Guidelines | MSVC Guidelines | SonarQube | Our Approach |
|---------|---------------------|-----------------|-----------|--------------|
| Global const | ES.3, Con.1 | Prefer constexpr | S1255 | Fix, except Windows API |
| C-style cast | ES.49, Pro.Ccasts | Use named casts | S930 | Fix with appropriate cast |
| Macros | ES.45, Enum.3 | Prefer constexpr | S501 | Convert only values |
| std::array | ES.107, SL.4 | Prefer containers | S5955 | Evaluate stack size |
| std::string | SL.str.1 | Use std::string | S5199 | Internal only |
| auto | ES.11, ES.12 | Use judiciously | S6199 | Security types explicit |
| enum class | Enum.3, Enum.8 | Prefer enum class | S5728 | Internal enums only |

---

## LSASS Safety Checklist

For each fix, verify:

- [ ] **No exceptions** - Fix does not introduce exception-throwing code
- [ ] **No heap in hot paths** - Fix does not add heap allocation in authentication path
- [ ] **No STL across boundaries** - Fix does not pass STL types across DLL boundaries
- [ ] **Stack size stable** - Fix does not significantly increase stack usage
- [ ] **Error handling preserved** - Fix does not change error handling behavior
- [ ] **Logic equivalence** - Fix produces identical behavior for all inputs
- [ ] **Windows API compatible** - Fix works with Windows API types and calling conventions
- [ ] **Security review** - Complex fixes reviewed for security implications

---

## Sources

### C++ Standards and Guidelines
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) - HIGH confidence
- [cppreference - C++23](https://en.cppreference.com/w/cpp/23) - HIGH confidence
- [cppreference - C++17](https://en.cppreference.com/w/cpp/17) - HIGH confidence

### Microsoft Documentation
- [Microsoft Learn - MSVC C++ Conformance](https://learn.microsoft.com/en-us/cpp/overview/msvc-conformance-improvements) - HIGH confidence
- [Microsoft Learn - C++ Best Practices](https://learn.microsoft.com/en-us/cpp/cpp/) - HIGH confidence

### SonarQube Rules
- [SonarQube C++ Rules](https://rules.sonarsource.com/cpp/) - HIGH confidence
- SonarQube analysis of EIDAuthentication codebase - HIGH confidence (project-specific)

### Code Analysis
- EIDAuthentication codebase analysis - HIGH confidence (direct analysis)
- SonarQube issue counts from analysis report - HIGH confidence

---

*Feature research for: SonarQube issue remediation in C++23 Windows LSASS codebase*
*Researched: 2026-02-18*
