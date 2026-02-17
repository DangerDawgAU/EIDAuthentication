# Phase 23: SonarQube Const Issues - Research

**Researched:** 2026-02-17
**Domain:** C++ global variable const correctness
**Confidence:** HIGH

## Summary

This phase addresses 63 remaining SonarQube "Global variables should be const" issues and 31 "Global pointers should be const at every level" issues. Phase 16 (v1.2 Const Correctness) already fixed approximately 8 global variables, leaving ~63 remaining. The remaining issues require careful analysis to distinguish between truly immutable globals (should be marked const), mutable state (cannot be const), and Windows API interop requirements (won't fix).

**Primary recommendation:** Categorize remaining globals into three buckets: (1) immutable constants that can be safely marked const, (2) mutable state required for program operation, and (3) Windows API interop buffers that cannot be const.

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SONAR-03 | Review and resolve global variable const issues (~63 remaining) | Global variable categories identified; safe conversion patterns documented; won't-fix categories defined with Windows API justifications |

## Standard Stack

### Const Declaration Patterns

| Pattern | Use Case | Why Standard |
|---------|----------|--------------|
| `const DWORD X = 32;` | Immutable numeric global | Clear intent, compiler optimization |
| `const BOOL X = TRUE;` | Immutable boolean flag | Read-only guarantee |
| `static const CHAR X[] = "value";` | File-local immutable string | Internal linkage, read-only |
| `constexpr DWORD X = 32;` | Compile-time constant | Best for true constants (Phase 22) |

### When to Use const vs constexpr

| Pattern | Use When | Example |
|---------|----------|---------|
| `constexpr` | Value known at compile time | `constexpr DWORD MAX_SIZE = 256;` |
| `const` | Value set at runtime but never modified | `const BOOL TraceAllocation = TRUE;` |

## Architecture Patterns

### Pattern 1: Immutable Configuration Constants

**What:** Mark configuration constants that never change as `const`
**When to use:** Global variables initialized at declaration and never reassigned

**Before:**
```cpp
BOOL TraceAllocation = TRUE;
```

**After:**
```cpp
const BOOL TraceAllocation = TRUE;
```

**Already converted (Package.cpp):**
```cpp
const BOOL TraceAllocation = TRUE;
```

### Pattern 2: Mutable State Variables (Do Not Mark const)

**What:** Identify global variables that MUST remain mutable
**When to use:** Variables that are modified during program execution

**Examples that CANNOT be const:**

```cpp
// EIDCardLibrary/Package.cpp - LSA function pointers set during initialization
PLSA_ALLOCATE_LSA_HEAP MyAllocateHeap = nullptr;  // Set by SetAlloc()
PLSA_FREE_LSA_HEAP MyFreeHeap = nullptr;          // Set by SetFree()
PLSA_IMPERSONATE_CLIENT MyImpersonate = nullptr;  // Set by SetImpersonate()

// EIDCardLibrary/Tracing.cpp - Tracing state
BOOL IsTracingEnabled = FALSE;  // Modified by EnableCallback()
BOOL bFirst = TRUE;             // Modified during initialization

// EIDCredentialProvider/Dll.cpp - DLL state
HINSTANCE g_hinst = nullptr;    // Set by DllMain
static LONG g_cRef = 0;         // Modified by AddRef/Release

// EIDConfigurationWizard/ UI state
DWORD dwCurrentCredential = 0xFFFFFFFF;  // Changed during selection
BOOL fHasDeselected = TRUE;              // UI state tracking
```

### Pattern 3: Windows API Interop Buffers (Do Not Mark const)

**What:** Identify buffers that cannot be const due to Windows API requirements
**When to use:** When Windows API takes non-const pointer parameters

**Examples that CANNOT be const (CertificateUtilities.cpp):**
```cpp
// These MUST remain non-const writable arrays
// CERT_ENHKEY_USAGE.rgpszUsageIdentifier is LPSTR* (non-const)
// With /Zc:strictStrings, literals are const and cannot convert to LPSTR
static char s_szOidClientAuth[] = szOID_PKIX_KP_CLIENT_AUTH;
static char s_szOidServerAuth[] = szOID_PKIX_KP_SERVER_AUTH;
static char s_szOidSmartCardLogon[] = szOID_KP_SMARTCARD_LOGON;
```

**Examples that CANNOT be const (EIDCredentialProvider/common.h):**
```cpp
// CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR.pszLabel is LPWSTR (non-const)
static wchar_t s_wszEmptyLabel[] = L"";
```

### Pattern 4: File-Local Constants

**What:** Use `static const` for file-scope constants
**When to use:** Constants used only within one translation unit

**Before:**
```cpp
HANDLE hInternalLogWriteHandle = nullptr;
```

**Analysis:** This is set once and never modified - could potentially be const if set at declaration time, but since it's initialized to nullptr and set later, it cannot be const.

### Anti-Patterns to Avoid

- **Marking LSA callback pointers as const:** These are set during LSA package initialization
- **Marking Windows API output buffers as const:** API may write to them
- **Marking synchronization primitives as const:** CRITICAL_SECTION, etc. are modified
- **Marking UI state variables as const:** These change during user interaction

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Thread-safe mutable globals | Custom synchronization wrapper | Keep as mutable, existing sync is correct | Adding const would be incorrect |
| Configuration that changes | Const with mutable wrapper | Keep as non-const | Const means never changes |

## Common Pitfalls

### Pitfall 1: Const Pointer vs Pointer to Const

**What goes wrong:** Confusing `TYPE* const` with `const TYPE*`
**Why it happens:** C++ pointer const syntax is subtle
**How to avoid:**
- `const TYPE* ptr` - pointer to const data (data can't be modified through ptr)
- `TYPE* const ptr` - const pointer (ptr can't point elsewhere)
- `const TYPE* const ptr` - both

**For global function pointers:**
```cpp
// WRONG - pointer itself cannot change
PLSA_ALLOCATE_LSA_HEAP const MyAllocateHeap = nullptr;  // Can't be assigned later

// WRONG - data pointed to cannot change (but function pointer has no data)
const PLSA_ALLOCATE_LSA_HEAP MyAllocateHeap = nullptr;

// CORRECT - this is mutable state, cannot be const
PLSA_ALLOCATE_LSA_HEAP MyAllocateHeap = nullptr;
```

### Pitfall 2: Windows API Non-Const Parameters

**What goes wrong:** Marking a buffer const that Windows API expects to be writable
**Why it happens:** Windows APIs often use non-const for historical reasons
**How to avoid:** Check API signatures - if parameter is non-const pointer, buffer cannot be const
**Warning signs:** Compile errors about const correctness, or runtime crashes

### Pitfall 3: Initialization Order

**What goes wrong:** Declaring global as const but it's set during initialization phase
**Why it happens:** C++ globals are initialized in undefined order across TUs
**How to avoid:** Only mark as const if initialized at definition with a constant expression
**Warning signs:** Const global that needs to be "set up" during runtime

### Pitfall 4: extern Variables

**What goes wrong:** const mismatch between declaration and definition
**Why it happens:** extern variable has declaration in .h and definition in .cpp
**How to avoid:** BOTH must match - add const to both declaration and definition
**Warning signs:** Linker errors about duplicate definitions or missing definitions

## Code Examples

### Category 1: Can Be Marked const (Do Convert)

**Already const in codebase (examples):**
```cpp
// Package.cpp - already correctly const
const BOOL TraceAllocation = TRUE;

// EIDConfigurationWizard/EIDConfigurationWizard.cpp - already const
const DWORD dwReaderSize = ARRAYSIZE(szReader);
const DWORD dwCardSize = ARRAYSIZE(szCard);
const DWORD dwUserNameSize = ARRAYSIZE(szUserName);
const DWORD dwPasswordSize = ARRAYSIZE(szPassword);
```

**Potential const candidates (review needed):**
None identified - most remaining globals are legitimately mutable.

### Category 2: Cannot Be const - Mutable State (Won't Fix)

**LSA function pointers (Package.cpp):**
```cpp
// These are set by LSA during package initialization
PLSA_ALLOCATE_LSA_HEAP MyAllocateHeap = nullptr;  // Won't Fix: Set by SetAlloc()
PLSA_FREE_LSA_HEAP MyFreeHeap = nullptr;          // Won't Fix: Set by SetFree()
PLSA_IMPERSONATE_CLIENT MyImpersonate = nullptr;  // Won't Fix: Set by SetImpersonate()
```

**Tracing state (Tracing.cpp):**
```cpp
REGHANDLE hPub;                  // Won't Fix: Set by EventRegister()
BOOL bFirst = TRUE;              // Won't Fix: Modified during init
BOOL IsTracingEnabled = FALSE;   // Won't Fix: Modified by EnableCallback()
```

**DLL state (Dll.cpp):**
```cpp
static LONG g_cRef = 0;          // Won't Fix: Reference counting
HINSTANCE g_hinst = nullptr;     // Won't Fix: Set by DllMain
```

**UI state (EIDConfigurationWizard/*.cpp):**
```cpp
DWORD dwCurrentCredential = 0xFFFFFFFF;  // Won't Fix: Selection tracking
BOOL fHasDeselected = TRUE;              // Won't Fix: UI state
BOOL fGotoNewScreen = FALSE;             // Won't Fix: Navigation state
PCCERT_CONTEXT pRootCertificate = nullptr;  // Won't Fix: User-selected cert
DWORD dwWizardError = 0;                 // Won't Fix: Error tracking
HWND hwndInvalidPasswordBalloon = nullptr;  // Won't Fix: Window handle
```

**Handle state:**
```cpp
HANDLE hInternalLogWriteHandle = nullptr;  // Won't Fix: Set during operation
HANDLE hFile = nullptr;                    // Won't Fix: Set during operation
HMODULE samsrvDll = nullptr;               // Won't Fix: Loaded at runtime
static HANDLE g_hTraceOutputFile = nullptr;  // Won't Fix: Set during tracing
```

### Category 3: Cannot Be const - Windows API Requirement (Won't Fix)

**API interop buffers (CertificateUtilities.cpp):**
```cpp
// CERT_ENHKEY_USAGE.rgpszUsageIdentifier is LPSTR* (non-const char**)
// These MUST be writable arrays for Windows CryptoAPI
static char s_szOidClientAuth[] = szOID_PKIX_KP_CLIENT_AUTH;      // Won't Fix: API requirement
static char s_szOidServerAuth[] = szOID_PKIX_KP_SERVER_AUTH;      // Won't Fix: API requirement
static char s_szOidSmartCardLogon[] = szOID_KP_SMARTCARD_LOGON;   // Won't Fix: API requirement
static char s_szOidEfs[] = szOID_KP_EFS;                          // Won't Fix: API requirement
static char s_szOidKeyUsage[] = szOID_KEY_USAGE;                  // Won't Fix: API requirement
static char s_szOidBasicConstraints2[] = szOID_BASIC_CONSTRAINTS2;  // Won't Fix: API requirement
static char s_szOidEnhancedKeyUsage[] = szOID_ENHANCED_KEY_USAGE;   // Won't Fix: API requirement
static char s_szOidSubjectKeyId[] = szOID_SUBJECT_KEY_IDENTIFIER;   // Won't Fix: API requirement
static char s_szOidSha1RsaSign[] = szOID_OIWSEC_sha1RSASign;        // Won't Fix: API requirement
```

**API interop buffers (common.h):**
```cpp
// CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR.pszLabel is LPWSTR (non-const)
static wchar_t s_wszEmptyLabel[] = L"";  // Won't Fix: API requirement
```

## Global Variable Category Analysis

Based on codebase review, here are the global variable categories and const decisions:

| Category | Count (Est.) | Decision | Rationale |
|----------|--------------|----------|-----------|
| LSA function pointers | 3 | Won't Fix | Set during LSA initialization |
| Tracing state variables | 3 | Won't Fix | Modified at runtime |
| DLL state (ref count, hinst) | 2 | Won't Fix | Modified by COM/runtime |
| UI state variables | 7 | Won't Fix | Modified during wizard operation |
| Handle variables | 5 | Won't Fix | Set/opened during operation |
| Windows API buffers | 10 | Won't Fix | Non-const parameter requirements |
| Already const | ~8 | Already Fixed | Done in Phase 16/22 |

**Estimated can be const:** ~0-2 (most already fixed or legitimately mutable)
**Estimated won't fix:** ~63 (all remaining issues are legitimately mutable)

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Mutable globals everywhere | Minimize mutable globals | Modern C++ | Thread safety, clearer code |
| Implicit const-correctness | Explicit const annotations | C++11+ | Compiler-enforced immutability |
| Global state | Dependency injection | Modern patterns | Testability, explicit dependencies |

**Deprecated/outdated:**
- Relying on convention for global const-ness: Use explicit `const` keyword
- Mutable globals for configuration: Use `constexpr` or `const`

## LSASS Safety Considerations

| Consideration | Impact on const Usage |
|---------------|-----------------------|
| No exceptions | No impact - const is compile-time only |
| Static CRT | No impact - const globals in read-only section |
| Memory allocation | No impact - const prevents modification |
| Thread safety | Positive - const globals are inherently thread-safe |

**Key insight:** Marking truly immutable globals as `const` improves LSASS safety by placing them in read-only memory, preventing accidental or malicious modification.

## Open Questions

1. **Are there any hidden mutable globals in headers?**
   - What we know: Most globals are in .cpp files
   - What's unclear: extern declarations in headers may reference additional globals
   - Recommendation: Search all .h files for extern declarations and verify matching definitions

2. **Can any static locals be promoted to const?**
   - What we know: Function-local statics may be const-candidates
   - What's unclear: Need to check if they're modified after initialization
   - Recommendation: Review function-local statics in high-issue files

3. **SonarQube Issue Count Accuracy**
   - What we know: VERIFICATION.md reports 63 "Global variables should be const" issues
   - What's unclear: Some may already be fixed; count may be stale
   - Recommendation: Re-run SonarQube scan to get current issue count before planning

## Sources

### Primary (HIGH confidence)
- `.planning/VERIFICATION.md` - Phase 19 issue analysis (63 global const issues, 31 pointer const issues)
- `.planning/sonarqube-analysis.md` - Issue categorization (71 globals should be const, 31 pointers)
- `.planning/phases/16-const-correctness/16-01-PLAN.md` - Phase 16 execution context
- Codebase analysis - Identified global variable patterns

### Secondary (MEDIUM confidence)
- `.planning/phases/22-sonarqube-macro-issues/22-RESEARCH.md` - Format reference, constexpr patterns
- `.planning/STATE.md` - Project constraints and prior decisions

### Tertiary (LOW confidence)
- SonarQube rule documentation - Not accessible (RSPEC rules)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - `const` is well-documented C++ feature, patterns clear
- Architecture: HIGH - Global variable patterns well-understood
- Pitfalls: HIGH - Common C++ knowledge about const-correctness

**Research date:** 2026-02-17
**Valid until:** Stable - C++ `const` semantics don't change frequently
