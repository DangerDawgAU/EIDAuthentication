# Phase 2: Error Handling - Research

**Researched:** 2026-02-15
**Domain:** C++23 `std::expected<T, E>` error handling, Windows HRESULT/NTSTATUS interop, LSASS exception-free patterns
**Confidence:** HIGH

## Summary

This phase introduces `std::expected<T, E>` for internal typed error handling while preserving C-style API boundaries at exported functions (HRESULT, BOOL, NTSTATUS). The codebase currently uses `BOOL` + `SetLastError()`/`GetLastError()` and `__try/__finally` patterns throughout. Before `std::expected` can be adopted, 23 compile errors from `/Zc:strictStrings` const-correctness must be fixed in EIDCardLibrary.

The key technical challenges are: (1) fixing string literal const-correctness errors that block compilation, (2) designing an error type suitable for `std::expected<T, Error>`, (3) creating noexcept conversion functions between internal `std::expected` and external HRESULT/NTSTATUS/BOOL, and (4) ensuring all new error-handling code is marked `noexcept` for LSASS compatibility.

**Primary recommendation:** Fix `/Zc:strictStrings` compile errors first, then introduce a `Result<T>` alias for `std::expected<T, HRESULT>` with noexcept conversion utilities at API boundaries.

## Standard Stack

### Core
| Component | Version | Purpose | Why Standard |
|-----------|---------|---------|--------------|
| `<expected>` header | C++23 (VS 2022 17.6+) | Type-safe error handling | C++23 standard, no exceptions needed |
| `std::expected<T, E>` | C++23 | Return either value or error | Replaces BOOL+GetLastError pattern |
| `std::unexpected<E>` | C++23 | Wrap error values | Part of std::expected interface |

### Supporting
| Component | Purpose | When to Use |
|-----------|---------|-------------|
| `noexcept` specifier | Exception-free guarantee | All new error-handling functions |
| `[[nodiscard]]` attribute | Force error checking | Functions returning std::expected |
| `HRESULT` | Windows error codes | External API boundaries |
| `NTSTATUS` | LSA/Kernel error codes | LSA authentication functions |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `std::expected<T, E>` | Exceptions | Cannot use in LSASS context |
| `std::expected<T, E>` | `std::optional<T>` | No error information |
| `std::expected<T, HRESULT>` | Custom Result<T> type | More code to maintain, not standard |

**Header requirement:**
```cpp
#include <expected>  // C++23 - available in VS 2022 17.6+
```

## Architecture Patterns

### Recommended Error Type Hierarchy
```
EIDCardLibrary/
├── ErrorHandling.h          # Result<T> alias, conversion utilities
├── ErrorHandling.cpp        # Conversion implementations (if needed)
└── [existing files]         # Modified to use Result<T>
```

### Pattern 1: Result<T> Type Alias
**What:** Define a convenient alias for `std::expected` with HRESULT error type.
**When to use:** All internal functions that can fail.

```cpp
// Source: Pattern for Windows HRESULT-based error handling
#include <expected>
#include <winerror.h>

namespace EID {

template<typename T>
using Result = std::expected<T, HRESULT>;

// Specialization for void-returning functions
using ResultVoid = std::expected<void, HRESULT>;

// Helper to create unexpected HRESULT
[[nodiscard]] inline auto make_error(HRESULT hr) noexcept {
    return std::unexpected(hr);
}

// Success helper
template<typename T>
[[nodiscard]] inline auto make_success(T&& value) noexcept {
    return Result<std::decay_t<T>>(std::forward<T>(value));
}

} // namespace EID
```

### Pattern 2: API Boundary Conversion (Internal to External)
**What:** Convert `std::expected` to HRESULT/BOOL at exported function boundaries.
**When to use:** Every exported LSA/Credential Provider function.

```cpp
// Source: Pattern for HRESULT boundary conversion
// Internal function using std::expected
[[nodiscard]] EID::Result<PCCERT_CONTEXT> GetCertificateFromCspInfoInternal(
    PEID_SMARTCARD_CSP_INFO pCspInfo) noexcept;

// Exported function maintaining C-style API
HRESULT GetCertificateFromCspInfo(
    __in PEID_SMARTCARD_CSP_INFO pCspInfo,
    __out PCCERT_CONTEXT* ppCertContext)
{
    if (ppCertContext == nullptr) {
        return E_POINTER;
    }

    auto result = GetCertificateFromCspInfoInternal(pCspInfo);
    if (result) {
        *ppCertContext = *result;
        return S_OK;
    }
    return result.error();
}
```

### Pattern 3: BOOL Boundary Conversion
**What:** Convert `std::expected` to BOOL + SetLastError pattern.
**When to use:** Functions currently returning BOOL with GetLastError.

```cpp
// Source: Pattern for BOOL boundary conversion
// Internal function
[[nodiscard]] EID::Result<void> DoSomethingInternal() noexcept;

// Exported function maintaining BOOL + GetLastError pattern
BOOL DoSomething() noexcept
{
    auto result = DoSomethingInternal();
    if (result) {
        SetLastError(ERROR_SUCCESS);
        return TRUE;
    }
    SetLastError(static_cast<DWORD>(result.error()));
    return FALSE;
}
```

### Pattern 4: NTSTATUS Boundary Conversion
**What:** Convert `std::expected` to NTSTATUS for LSA functions.
**When to use:** LSA authentication package exports.

```cpp
// Source: Pattern for NTSTATUS boundary conversion
[[nodiscard]] EID::Result<void> CheckAuthorizationInternal(
    PWSTR UserName,
    NTSTATUS* SubStatus,
    LARGE_INTEGER* ExpirationTime) noexcept;

NTSTATUS CheckAuthorization(
    PWSTR UserName,
    NTSTATUS* SubStatus,
    LARGE_INTEGER* ExpirationTime)
{
    auto result = CheckAuthorizationInternal(UserName, SubStatus, ExpirationTime);
    if (result) {
        return STATUS_SUCCESS;
    }
    // Map HRESULT to NTSTATUS if needed, or use direct NTSTATUS as error type
    return HRESULTToNTStatus(result.error());
}
```

### Pattern 5: noexcept Function Design
**What:** All new error-handling functions must be noexcept.
**When to use:** Every function returning `std::expected`.

```cpp
// Source: LSASS-compatible error handling pattern
// GOOD: noexcept, no allocations in error path
[[nodiscard]] EID::Result<DWORD> GetValue() noexcept
{
    DWORD value;
    if (!RegistryRead(&value)) {
        return EID::make_error(HRESULT_FROM_WIN32(GetLastError()));
    }
    return value;  // implicit conversion to Result<DWORD>
}

// BAD: Can throw, not LSASS-safe
EID::Result<std::string> GetName()  // NOT noexcept
{
    std::string name;  // Can throw bad_alloc
    // ...
}
```

### Anti-Patterns to Avoid
- **Throwing exceptions:** Cannot use in LSASS context - use `std::expected` instead
- **Returning `std::expected` without `[[nodiscard]]`:** Caller might ignore error
- **Memory allocation in error path:** Can throw, breaks noexcept guarantee
- **Using `std::string` in `std::expected` error type:** Use fixed-size buffer or HRESULT
- **Mixing error types:** Stick to HRESULT internally for consistency

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Custom Result<T> type | Class with value/error union | `std::expected<T, E>` | Standard, well-tested, compiler optimizations |
| HRESULT to string | Custom FormatMessage wrapper | `GetLastError()` + tracing | Already in codebase |
| Error propagation | Manual if-checks | `.and_then()` / `.transform()` | Cleaner code, standard monadic interface |
| Optional values | `T*` with nullptr check | `std::optional<T>` | Type-safe, no null dereference risk |

**Key insight:** The standard library's `std::expected` is designed exactly for this use case - exception-free error handling with type safety. Building a custom solution adds maintenance burden without benefit.

## Common Pitfalls

### Pitfall 1: String Literal to LPSTR/LPWSTR (Compile Error)
**What goes wrong:** With `/Zc:strictStrings`, string literals are `const char*/const wchar_t*` and cannot implicitly convert to non-const `LPSTR/LPWSTR`.
**Why it happens:** Legacy Windows APIs use non-const string parameters even when they don't modify the string.
**How to avoid:**
1. Use `const_cast` only when API guarantees no modification
2. Use temporary buffers with `wcscpy_s`/`strcpy_s`
3. Prefer wide string literals `L"text"` with `LPWSTR` types

**Example fix:**
```cpp
// BEFORE (compile error):
CERT_ENHKEY_USAGE CertEnhKeyUsage = { 0, nullptr };
CertEnhKeyUsage.rgpszUsageIdentifier[0] = szOID_KP_SMARTCARD_LOGON;  // LPSTR* = const char*

// AFTER (fixed):
static char s_szOidSmartCardLogon[] = szOID_KP_SMARTCARD_LOGON;  // Non-const copy
CertEnhKeyUsage.rgpszUsageIdentifier[0] = s_szOidSmartCardLogon;
```

**Warning signs:** Compiler errors like "cannot convert from 'const char[N]' to 'LPSTR'"

### Pitfall 2: Forgetting noexcept on std::expected Functions
**What goes wrong:** Function may throw exceptions, crashing LSASS.
**Why it happens:** Developer forgets LSASS context restriction.
**How to avoid:** Add `noexcept` to every function returning `std::expected`.
**Warning signs:** Code review finds function without `noexcept` that allocates memory or uses STL containers.

### Pitfall 3: Using std::string in Error Type
**What goes wrong:** `std::string` allocation can throw `std::bad_alloc`.
**Why it happens:** Developer wants rich error messages.
**How to avoid:** Use HRESULT with optional fixed-size message buffer, or store error code only.
**Warning signs:** `std::expected<T, std::string>` in code.

### Pitfall 4: Ignoring [[nodiscard]] Warning
**What goes wrong:** Caller ignores return value, error is silently dropped.
**Why it happens:** Old habits from HRESULT-returning functions.
**How to avoid:** Always mark `std::expected` returning functions as `[[nodiscard]]`.
**Warning signs:** Compiler warning C4834 discarding return value.

### Pitfall 5: Breaking API Compatibility
**What goes wrong:** Changing exported function signature breaks existing callers.
**Why it happens:** Over-eager refactoring.
**How to avoid:** Only change internal functions to use `std::expected`. Keep exported function signatures unchanged (HRESULT, BOOL, NTSTATUS).
**Warning signs:** DLL export table changes.

## Code Examples

### Complete Result<T> Header (ErrorHandling.h)
```cpp
// Source: Pattern for Windows C++23 error handling
#pragma once

#include <expected>
#include <winerror.h>
#include <ntstatus.h>

namespace EID {

// Primary result type using HRESULT as error
template<typename T>
using Result = std::expected<T, HRESULT>;

// Void result for operations with no return value
using ResultVoid = std::expected<void, HRESULT>;

// Create an error result
[[nodiscard]] inline auto make_unexpected(HRESULT hr) noexcept {
    return std::unexpected(hr);
}

// Convert Win32 error to HRESULT
[[nodiscard]] inline HRESULT win32_to_hr(DWORD win32Error) noexcept {
    return HRESULT_FROM_WIN32(win32Error);
}

// Convert Result to BOOL (sets GetLastError)
template<typename T>
[[nodiscard]] inline BOOL to_bool(Result<T>&& result) noexcept {
    if (result) {
        return TRUE;
    }
    SetLastError(static_cast<DWORD>(result.error()));
    return FALSE;
}

// Convert ResultVoid to BOOL
[[nodiscard]] inline BOOL to_bool(ResultVoid&& result) noexcept {
    if (result) {
        return TRUE;
    }
    SetLastError(static_cast<DWORD>(result.error()));
    return FALSE;
}

// Success check helper
template<typename T>
[[nodiscard]] inline bool succeeded(const Result<T>& result) noexcept {
    return result.has_value();
}

} // namespace EID
```

### Before/After: Refactoring BOOL Function
```cpp
// BEFORE: Current pattern in codebase
BOOL HasCertificateRightEKU(__in PCCERT_CONTEXT pCertContext)
{
    BOOL fValidation = FALSE;
    DWORD dwError = 0, dwSize = 0, dwI;
    PCERT_ENHKEY_USAGE pCertUsage = nullptr;
    __try
    {
        if (!GetPolicyValue(AllowCertificatesWithNoEKU))
        {
            if (!CertGetEnhancedKeyUsage(pCertContext, 0, nullptr, &dwSize))
            {
                dwError = GetLastError();
                __leave;
            }
            pCertUsage = (PCERT_ENHKEY_USAGE)EIDAlloc(dwSize);
            if (!pCertUsage)
            {
                dwError = GetLastError();
                __leave;
            }
            // ... more logic
        }
        fValidation = TRUE;
    }
    __finally
    {
        if (pCertUsage)
            EIDFree(pCertUsage);
    }
    SetLastError(dwError);
    return fValidation;
}

// AFTER: Using std::expected (internal function)
[[nodiscard]] EID::Result<void> HasCertificateRightEKUInternal(
    __in PCCERT_CONTEXT pCertContext) noexcept
{
    if (GetPolicyValue(AllowCertificatesWithNoEKU) != 0) {
        return {};  // Success
    }

    DWORD dwSize = 0;
    if (!CertGetEnhancedKeyUsage(pCertContext, 0, nullptr, &dwSize)) {
        return EID::make_unexpected(HRESULT_FROM_WIN32(GetLastError()));
    }

    // Use unique_ptr-style cleanup (EIDFree wrapper)
    struct CertUsageDeleter {
        void operator()(PCERT_ENHKEY_USAGE p) const noexcept {
            if (p) EIDFree(p);
        }
    };
    std::unique_ptr<CERT_ENHKEY_USAGE, CertUsageDeleter> pCertUsage(
        static_cast<PCERT_ENHKEY_USAGE>(EIDAlloc(dwSize)));

    if (!pCertUsage) {
        return EID::make_unexpected(E_OUTOFMEMORY);
    }

    if (!CertGetEnhancedKeyUsage(pCertContext, 0, pCertUsage.get(), &dwSize)) {
        return EID::make_unexpected(HRESULT_FROM_WIN32(GetLastError()));
    }

    for (DWORD i = 0; i < pCertUsage->cUsageIdentifier; i++) {
        if (strcmp(pCertUsage->rgpszUsageIdentifier[i], szOID_KP_SMARTCARD_LOGON) == 0) {
            return {};  // Success
        }
    }

    return EID::make_unexpected(CERT_TRUST_IS_NOT_VALID_FOR_USAGE);
}

// Exported wrapper maintains BOOL signature
BOOL HasCertificateRightEKU(__in PCCERT_CONTEXT pCertContext) noexcept
{
    return EID::to_bool(HasCertificateRightEKUInternal(pCertContext));
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `BOOL` + `SetLastError` | `std::expected<T, HRESULT>` | C++23 (2023) | Type-safe, no forgotten error checks |
| `__try/__finally` | RAII + `std::expected` | C++23 | Cleaner cleanup, no SEH overhead |
| Exceptions | `std::expected` | C++23 | LSASS-compatible |
| `HRESULT` everywhere | Internal `std::expected`, boundary `HRESULT` | C++23 | Clean internals, compatible API |

**Deprecated/outdated:**
- Exceptions in LSASS context: Never were allowed, but `std::expected` gives type-safe alternative
- Manual error propagation: Use monadic `.and_then()` / `.transform()` instead

## Open Questions

1. **Should we use HRESULT or custom error type in std::expected?**
   - What we know: HRESULT is universal in Windows, has facility codes
   - What's unclear: If a custom error type would provide better debugging
   - Recommendation: Start with HRESULT, evaluate custom type later if needed

2. **How to handle the LPSTR* rgpszUsageIdentifier issue?**
   - What we know: CERT_ENHKEY_USAGE uses `LPSTR*` (non-const)
   - What's unclear: Best pattern for fixing across multiple files
   - Recommendation: Use static char arrays (non-const) at file scope

3. **Should ResultVoid be used or Result<std::monostate>?**
   - What we know: `std::expected<void, E>` is valid in C++23
   - What's unclear: Compiler support completeness
   - Recommendation: Test with VS 2022 17.6+, use `std::monostate` fallback if needed

## Sources

### Primary (HIGH confidence)
- [Microsoft C++ Blog - Functional exception-less error handling with C++23's optional and expected](https://devblogs.microsoft.com/cppblog/cpp23s-optional-and-expected/) - Official MSVC implementation guide
- [Microsoft Learn - /Zc:strictStrings](https://learn.microsoft.com/en-us/cpp/build/reference/zc-strictstrings-disable-string-literal-type-conversion?view=msvc-170) - Const-correctness documentation
- [VS 2022 Changelog](https://github.com/microsoft/STL/wiki/VS-2022-Changelog) - std::expected availability in 17.6

### Secondary (MEDIUM confidence)
- [Microsoft Learn - How to interface between exceptional and non-exceptional code](https://learn.microsoft.com/en-us/cpp/cpp/how-to-interface-between-exceptional-and-non-exceptional-code?view=msvc-170) - Exception boundary patterns
- [cppreference - std::expected](https://en.cppreference.com/w/cpp/utility/expected) - Standard reference

### Tertiary (LOW confidence)
- [Modern C++ - std::expected Tutorial](https://www.modernescpp.com/index.php/c23-a-new-way-of-error-handling-with-stdexpected/) - Community tutorial

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - std::expected is well-documented, available in VS 2022 17.6+
- Architecture: HIGH - Clear patterns from Microsoft blog and C++23 standard
- Pitfalls: HIGH - /Zc:strictStrings errors are well-documented MSVC issues

**Research date:** 2026-02-15
**Valid until:** 90 days - C++23 std::expected is stable standard feature

## Appendix: Compile Errors to Fix (from Phase 1)

These `/Zc:strictStrings` errors must be fixed before `std::expected` adoption:

| File | Line Pattern | Issue |
|------|--------------|-------|
| CertificateUtilities.cpp | `szOID_*` to `LPSTR*` | String literal const-correctness |
| CertificateValidation.cpp | `szOID_*` to `LPSTR*` | String literal const-correctness |
| CompleteToken.cpp | String literal to `WCHAR*` | Const-correctness |
| Registration.cpp | String literals to `LPWSTR`/`PTSTR` | Const-correctness |
| smartcardmodule.cpp | String literals to `LPWSTR` | Const-correctness |
| TraceExport.cpp | `TEXT()` macros | Const-correctness |

**Fix pattern:**
```cpp
// BEFORE (compile error):
params.rgpszUsageIdentifier[0] = szOID_KP_SMARTCARD_LOGON;  // const char* to LPSTR

// AFTER (fixed):
static char s_szOidSmartCardLogon[] = szOID_KP_SMARTCARD_LOGON;  // Non-const storage
params.rgpszUsageIdentifier[0] = s_szOidSmartCardLogon;
```
