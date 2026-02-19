# Phase 22: SonarQube Macro Issues - Research

**Researched:** 2026-02-17
**Domain:** C++ preprocessor macro replacement with `const`/`constexpr`
**Confidence:** HIGH

## Summary

This phase addresses 111 SonarQube "Replace macro with const/constexpr" issues (Low severity, maintainability category). The codebase has approximately 150+ macro definitions across different categories, with some already converted to `constexpr` in prior phases (e.g., `WINEVENT_LEVEL_*`, `AUTHENTICATIONPACKAGENAME`, various constants in CContainer.cpp).

**Primary recommendation:** Convert simple value macros (numeric constants, string literals) to `constexpr`. Keep function-like macros, Windows API configuration macros, include guards, and conditional compilation macros unchanged.

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SONAR-02 | Review and resolve macro issues (~111 "replace with const/constexpr") | Macro categories identified; safe conversion patterns documented; "won't fix" categories defined with justifications |

## Standard Stack

### C++ Constant Types

| Type | Use Case | Why Standard |
|------|----------|--------------|
| `constexpr int` | Compile-time integer constants | Type-safe, visible in debugger, respects scope |
| `constexpr DWORD` | Windows API DWORD constants | Maintains Win32 API compatibility |
| `constexpr const char*` | String literals (non-TCHAR) | Type-safe, linkage-safe in headers |
| `constexpr const wchar_t*` | Wide string literals | Type-safe, no macro expansion issues |
| `inline constexpr` | Header-only constants (C++17+) | ODR-safe, no storage allocation |
| `const` | Runtime-initialized constants | When constexpr not possible |

### When to Use Each

| Pattern | Use When | Example |
|---------|----------|---------|
| `constexpr DWORD X = 32;` | Simple numeric constant | `CERT_HASH_LENGTH` |
| `constexpr const char* X = "value";` | ASCII string constant | `AUTHENTICATIONPACKAGENAME` |
| `constexpr wchar_t X[] = L"value";` | Wide string array | Configuration strings |
| `static constexpr` | File-scope constants | Local constants in .cpp files |
| `inline constexpr` | Header constants (C++17+) | Shared constants across TUs |

## Architecture Patterns

### Pattern 1: Numeric Constant Conversion

**What:** Replace numeric value macros with `constexpr` typed constants
**When to use:** Simple numeric values without type casts or complex expressions

**Before:**
```cpp
#define CERT_HASH_LENGTH 32
```

**After:**
```cpp
constexpr DWORD CERT_HASH_LENGTH = 32;
```

**Already converted example (CContainer.cpp):**
```cpp
constexpr DWORD MAX_CONTAINER_NAME_LENGTH = 1024;
constexpr DWORD MAX_READER_NAME_LENGTH = 256;
constexpr DWORD MAX_CARD_NAME_LENGTH = 256;
constexpr DWORD MAX_PROVIDER_NAME_LENGTH = 256;
```

### Pattern 2: String Literal Conversion

**What:** Replace string literal macros with `constexpr` pointer/array constants
**When to use:** String constants used at runtime

**Before:**
```cpp
#define AUTHENTICATIONPACKAGENAMET TEXT("EIDAuthenticationPackage")
```

**After (already done in EIDCardLibrary.h):**
```cpp
constexpr const char* AUTHENTICATIONPACKAGENAME = "EIDAuthenticationPackage";
constexpr const wchar_t* AUTHENTICATIONPACKAGENAMEW = L"EIDAuthenticationPackage";
```

**Note:** The `TEXT()` macro version must remain as a `#define` because `TEXT()` is itself a macro.

### Pattern 3: File-Local Constants

**What:** Use `static constexpr` for constants local to a translation unit
**When to use:** Constants only used within one .cpp file

**Before (CContainer.cpp):**
```cpp
#define REMOVALPOLICYKEY TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Removal Policy")
```

**After:**
```cpp
static constexpr TCHAR REMOVALPOLICYKEY[] = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Removal Policy");
```

### Pattern 4: Version Information

**What:** Convert version string/numeric macros to constexpr
**When to use:** Version identifiers

**Before (EIDAuthenticateVersion.h):**
```cpp
#define EIDAuthenticateVersionText "EIDAuthenticateVersionText_mytext"
#define EIDAuthenticateVersionNumeric 1.1.1.1
```

**After:**
```cpp
constexpr const char* EIDAuthenticateVersionText = "EIDAuthenticateVersionText_mytext";
// Note: EIDAuthenticateVersionNumeric with dots requires special handling (version struct)
```

### Anti-Patterns to Avoid

- **Converting function-like macros:** `#define MAX_ULONG ((ULONG)(-1))` requires type deduction
- **Removing include guards:** `#pragma once` or `#ifndef` guards must stay
- **Converting config macros:** `WIN32_NO_STATUS`, `SECURITY_WIN32` control Windows header behavior
- **Breaking TEXT() macros:** TCHAR-based strings need macro concatenation support

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Type casts in constants | `constexpr auto X = (DWORD)(-1);` | Keep as macro or use `static_cast` | Explicit intent |
| Platform-specific values | Custom detection logic | Keep macros | Conditional compilation |
| Resource IDs | `constexpr int ID_FOO = 101;` | Keep macros | Resource compiler requires `#define` |

**Key insight:** Not all macros should be converted. Windows resource files (.rc) require `#define` for resource IDs.

## Common Pitfalls

### Pitfall 1: TCHAR Incompatibility

**What goes wrong:** `constexpr` cannot use `TEXT()` macro in all contexts
**Why it happens:** `TEXT()` is a preprocessor macro that expands differently based on `_UNICODE`
**How to avoid:** Keep `TEXT()`-wrapped strings as macros, or provide separate char/wchar_t versions
**Warning signs:** Compile errors about constexpr initialization

### Pitfall 2: Resource File Dependencies

**What goes wrong:** Converting resource ID macros breaks the resource compiler
**Why it happens:** Windows resource compiler (.rc) only understands `#define`, not C++ constants
**How to avoid:** Keep all `IDD_*`, `IDC_*`, `IDS_*`, `IDB_*` macros in resource.h files
**Warning signs:** Resource compiler errors about undefined identifiers

### Pitfall 3: Header ODR Violations

**What goes wrong:** `const` in header causes "multiple definition" linker errors
**Why it happens:** Non-inline const variables have external linkage by default
**How to avoid:** Use `inline constexpr` (C++17+) or `static constexpr` at file scope
**Warning signs:** LNK2005 errors during linking

### Pitfall 4: Macro Concatenation (##)

**What goes wrong:** Converting macros that use token pasting breaks functionality
**Why it happens:** Token concatenation `##` is preprocessor-only, no C++ equivalent
**How to avoid:** Keep macros that use `##` operator
**Example:** `ERRORTOTEXT(ERROR)` macro in CertificateValidation.cpp uses `#ERROR` stringification

### Pitfall 5: Conditional Compilation

**What goes wrong:** Converting macros used in `#if` directives
**Why it happens:** `#if` can only evaluate preprocessor macros, not C++ constants
**How to avoid:** Keep any macro used in `#ifdef`, `#ifndef`, `#if` directives
**Warning signs:** Compile errors in preprocessor conditionals

## Code Examples

### Category 1: Safe to Convert (Do Convert)

**Numeric constants in source files:**
```cpp
// From EIDCardLibrary/smartcardmodule.cpp
// Before:
#define CHECK_DWORD(_X) {...}  // Keep - function-like

// From EIDCardLibrary/CertificateUtilities.cpp
// Before (local constant):
#define BITLEN_TO_CHECK 2048

// After:
static constexpr DWORD BITLEN_TO_CHECK = 2048;
```

**String constants in source files:**
```cpp
// From EIDCardLibrary/CContainer.cpp
// Before:
#define REMOVALPOLICYKEY TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Removal Policy")

// After:
static constexpr TCHAR REMOVALPOLICYKEY[] = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Removal Policy");
```

**Version information:**
```cpp
// From EIDCardLibrary/EIDAuthenticateVersion.h
// Before:
#define EIDAuthenticateVersionText "EIDAuthenticateVersionText_mytext"

// After:
constexpr const char* EIDAuthenticateVersionText = "EIDAuthenticateVersionText_mytext";
```

**Local helper macros (CompleteProfile.cpp):**
```cpp
// These are defined and undefined within function scope
// Can be converted to lambda or helper function
// But given scope, may be "won't fix" due to complexity
#define MYMACRO(MYSTRING1,MYSTRING2) \
    MYSTRING1.Length = (USHORT) wcslen(MYSTRING2)*sizeof(WCHAR);\
    MYSTRING1.MaximumLength = (MYSTRING1.Length?MYSTRING1.Length+2:0);
```

### Category 2: Cannot Convert (Won't Fix)

**Windows header configuration macros:**
```cpp
// These MUST remain as macros - they configure Windows SDK headers
#define WIN32_NO_STATUS 1        // Controls ntstatus.h behavior
#define SECURITY_WIN32           // Controls sspi.h behavior
#define WIN32_LEAN_AND_MEAN      // Controls windows.h includes
#define _CRTDBG_MAP_ALLOC        // Configures CRT debug allocation
```

**Resource ID definitions:**
```cpp
// Resource compiler requires #define - cannot use constexpr
#define IDD_01MAIN          67
#define IDC_USERNAME        1001
#define IDS_04TRUSTOK       39
#define IDB_TILE_IMAGE      101
#define _APS_NEXT_RESOURCE_VALUE  101
```

**Function-like macros:**
```cpp
// These use preprocessor features (variadic args, token pasting, __FILE__)
#define EIDCardLibraryTrace(dwLevel, ...) \
    EIDCardLibraryTraceEx(__FILE__,__LINE__,__FUNCTION__, dwLevel, __VA_ARGS__)

#define EIDAlloc(value) EIDAllocEx(__FILE__,__LINE__,__FUNCTION__,value)

#define ERRORTOTEXT(ERROR) case ERROR: pszName = TEXT(#ERROR); break;

#define CHECK_DWORD(_X) { \
    if (ERROR_SUCCESS != (status = (_X))) { \
        EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,TEXT("%s"), TEXT(#_X)); \
        __leave; \
    } \
}
```

**Include guards:**
```cpp
// Must remain as macros
#ifndef __CREDENTIALMANAGEMENT_H__
#define __CREDENTIALMANAGEMENT_H__
// ...
#endif
```

**TCHAR string concatenation:**
```cpp
// TEXT() is a macro, so this must be a macro for concatenation
#define AUTHENTICATIONPACKAGENAMET TEXT("EIDAuthenticationPackage")
```

**Windows API macros:**
```cpp
// From include/cardmod.h - Windows SDK file, do not modify
#define MAX_CONTAINER_NAME_LEN  39
#define MAX_PINS                8
// etc.
```

### Category 3: Evaluate Case-by-Case

**Local scope macros with token pasting:**
```cpp
// From EIDConfigurationWizard/CContainerHolder.cpp
#define ERRORTOTEXT(ERROR) case ERROR: LoadString(g_hinst,IDS_##ERROR, szName, dwSize); break;
// Uses ## for token concatenation with IDS_ - must stay as macro
```

**Window message definitions:**
```cpp
// From EIDConfigurationWizardPage05.cpp
#define WM_MYMESSAGE WM_USER + 10
// Could become: constexpr UINT WM_MYMESSAGE = WM_USER + 10;
// But WM_USER is a macro - evaluate based on usage context
```

**Array size helper:**
```cpp
// From EIDConfigurationWizardPage04.cpp
#define COLUMN_NUM ARRAYSIZE(Columns)
// Could become: constexpr size_t COLUMN_NUM = std::size(Columns);
// But depends on Columns being defined first - evaluate order
```

## Macro Category Analysis

Based on codebase review, here are the macro categories and conversion decisions:

| Category | Count (Est.) | Decision | Rationale |
|----------|--------------|----------|-----------|
| Resource IDs (IDD_, IDC_, IDS_, IDB_) | ~50 | Won't Fix | Resource compiler requires #define |
| Windows config macros (WIN32_NO_STATUS, etc.) | ~10 | Won't Fix | Controls header behavior |
| Function-like macros (tracing, alloc, checks) | ~15 | Won't Fix | Uses __FILE__, __LINE__, variadic args |
| Include guards | ~10 | Won't Fix | Preprocessor requirement |
| Simple numeric constants | ~15 | Convert | Safe to use constexpr |
| Simple string constants | ~5 | Convert | Safe to use constexpr |
| Token-pasting macros (##) | ~3 | Won't Fix | No C++ equivalent |
| Conditional compilation | ~3 | Won't Fix | Preprocessor requirement |

**Estimated conversions:** ~20 macros
**Estimated won't fix:** ~91 macros (documented with justifications)

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `#define X 100` | `constexpr int X = 100;` | C++11 | Type safety, scoping, debugger visibility |
| `#define X "str"` | `constexpr const char* X = "str";` | C++11 | Type safety, works with namespaces |
| Header `const` | `inline constexpr` | C++17 | ODR-safe, no storage needed |
| `enum { X = 1 }` | `constexpr int X = 1;` | C++11 | Explicit typing |

**Deprecated/outdated:**
- Anonymous enums for constants: Use `constexpr` instead
- Global `const` in headers: Use `inline constexpr` instead (C++17+)

## LSASS Safety Considerations

| Consideration | Impact on constexpr Usage |
|---------------|---------------------------|
| No exceptions | No impact - constexpr is compile-time |
| Static CRT | No impact - constexpr has no runtime allocation |
| Memory allocation | No impact - constants stored in read-only section |
| Type safety | Positive - constexpr provides type checking |

**Key insight:** `constexpr` is purely a compile-time feature with zero runtime overhead. It is completely safe for LSASS context and provides type safety benefits.

## Open Questions

1. **TCHAR Macro String Constants**
   - What we know: `TEXT()` is a macro that expands based on `_UNICODE`
   - What's unclear: Whether to provide separate char/wchar_t versions or keep as macro
   - Recommendation: For strings used with Windows API TCHAR functions, keep as macro. For strings used internally, provide `const char*` and `const wchar_t*` versions.

2. **WM_MYMESSAGE Custom Messages**
   - What we know: `WM_USER` is a macro defined by Windows
   - What's unclear: Whether `constexpr UINT WM_MYMESSAGE = WM_USER + 10;` is acceptable
   - Recommendation: Convert where the constant is only used in C++ code, not passed to Windows messaging macros.

3. **Version Numeric Format**
   - What we know: `#define EIDAuthenticateVersionNumeric 1.1.1.1` is not valid C++ syntax
   - What's unclear: How version is actually used in code
   - Recommendation: Investigate usage and convert to appropriate type (string or struct).

## Sources

### Primary (HIGH confidence)
- C++ Standard (C++11 onwards) - `constexpr` specification
- `.planning/sonarqube-analysis.md` - Issue count and categorization (111 "replace with const/constexpr" issues)
- Codebase analysis - Identified macro patterns and existing constexpr conversions

### Secondary (MEDIUM confidence)
- `.planning/phases/21-sonarqube-style-issues/21-RESEARCH.md` - Format reference
- `.planning/STATE.md` - Project constraints and prior decisions

### Tertiary (LOW confidence)
- SonarQube rule documentation - Not accessible (RSPEC rules)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - `constexpr` is well-documented C++11 feature, patterns clear
- Architecture: HIGH - Conversion patterns well-established, straightforward
- Pitfalls: HIGH - Common C++ knowledge about preprocessor vs constexpr limitations

**Research date:** 2026-02-17
**Valid until:** Stable - C++ `constexpr` and preprocessor semantics don't change frequently
