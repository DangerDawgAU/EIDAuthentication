# Phase 4: Code Quality - Research

**Researched:** 2026-02-15
**Domain:** C++23 code quality improvements (std::format, std::span, std::string::contains, deducing this)
**Confidence:** HIGH

## Summary

This phase modernizes string and buffer handling using C++23/C++20 utilities. The codebase currently uses C-style functions (`sprintf`, `snprintf`, `StringCchPrintf`, `wcscpy_s`, etc.) throughout EIDCardLibrary and EIDConfigurationWizard. The key constraint is LSASS context: `std::format` can throw exceptions, so it must ONLY be used in non-LSASS code (EIDConfigurationWizard EXE). For LSASS components (EIDCardLibrary, EIDCredentialProvider, EIDNativeLogon), use `std::span` for buffer views at internal boundaries and `std::string::contains()` for string operations.

CRITICAL: The codebase has NO CRTP patterns in actual source code - only mentioned in planning documents. QUAL-02 (deducing `this`) is NOT APPLICABLE to this codebase. Focus research on the other three features.

**Primary recommendation:** Apply `std::format` only in EIDConfigurationWizard. Use `std::span` and `std::string::contains()` throughout. Mark QUAL-02 as not applicable after codebase verification.

## User Constraints (from CONTEXT.md)

*No CONTEXT.md exists for this phase. Using requirements from REQUIREMENTS.md and prior phase decisions:*

### Locked Decisions (from prior phases)
- Target C++23 with `/std:c++23preview` flag
- Use v143 toolset to maintain Windows 7+ compatibility
- All constexpr validation functions must also be `noexcept` for LSASS compatibility
- `if consteval` not applicable - no dual-path functions in codebase
- `std::unreachable` not applicable - defensive programming required in LSASS

### Implicit Constraints
- LSASS context: no exceptions, no console I/O, static CRT required
- C-style APIs: preserve HRESULT/BOOL at exported boundaries
- Windows 7 support via v143 toolset

### Out of Scope
- `std::print`/`std::println` - no console in LSASS; UTF-8 buffer bugs exist
- Exceptions in any form
- Dynamic CRT (`/MD`)

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| `<format>` | C++20 (VS 2022 17.2+) | Type-safe string formatting | Replaces error-prone printf family |
| `<span>` | C++20 (VS 2022 17.0+) | Non-owning buffer view | Bounds-safe alternative to pointer+size |
| `<string>` | C++23 | `contains()` method | Simpler than `find() != npos` |

### Supporting
| Library | Purpose | When to Use |
|---------|---------|-------------|
| `<string_view>` | Non-owning string view | Read-only string parameters (NOT in LSASS class members) |
| `<array>` | Fixed-size buffer wrapper | Compile-time sized buffers |

### Feature Availability (MSVC)
| Feature | Minimum VS Version | Notes |
|---------|-------------------|-------|
| `std::format` | VS 2022 17.2 (MSVC 19.32) | C++20 feature, fully implemented |
| `std::span` | VS 2022 17.0 (MSVC 19.30) | C++20 feature |
| `std::string::contains()` | VS 2022 17.1 (MSVC 19.31) | C++23 feature (P1679R3) |
| `std::wstring::contains()` | VS 2022 17.1 (MSVC 19.31) | C++23 feature (P1679R3) |
| Deducing `this` | VS 2022 17.2 (MSVC 19.32) | **NOT APPLICABLE** - no CRTP in codebase |

**Header requirements:**
```cpp
#include <format>    // For std::format, std::format_to, std::vformat
#include <span>      // For std::span<T>, std::as_bytes
#include <string>    // For std::string::contains(), std::wstring::contains()
```

## Architecture Patterns

### Pattern 1: std::format in Non-LSASS Code (EIDConfigurationWizard)
**What:** Replace `sprintf`/`snprintf`/`StringCchPrintf` with `std::format` in the Configuration Wizard.
**When to use:** All string formatting in EIDConfigurationWizard (EXE, not in LSASS).

```cpp
// Source: C++20 standard pattern
// BEFORE: C-style formatting (current codebase pattern)
WCHAR wszFullPath[MAX_PATH];
if (FAILED(StringCchPrintfW(wszFullPath, ARRAYSIZE(wszFullPath),
                            L"%s\\%s", wszSystem32Path, wszModulePath))) {
    return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
}

// AFTER: std::format (non-LSASS only)
#include <format>
#include <string>

std::wstring wszFullPath = std::format(L"{}\\{}", wszSystem32Path, wszModulePath);
```

### Pattern 2: std::span for Internal Buffer Boundaries (LSASS-safe)
**What:** Use `std::span` for passing buffers at internal function boundaries.
**When to use:** Functions that receive raw pointers with size - convert to span for bounds checking.

```cpp
// Source: C++20 standard pattern for buffer handling
// BEFORE: Raw pointer + size (current pattern in codebase)
void ProcessBuffer(BYTE* pBuffer, size_t bufferSize) noexcept;

// AFTER: std::span provides bounds safety
#include <span>

void ProcessBuffer(std::span<BYTE> buffer) noexcept {
    if (buffer.empty()) return;
    // Bounds-safe access - buffer.size() always available
    for (auto& byte : buffer) {
        byte = 0; // Example operation
    }
}

// Calling code:
BYTE data[256];
ProcessBuffer(data); // Deduces size automatically

// At API boundary, convert back:
void ExportedFunction(BYTE* pBuffer, size_t bufferSize) noexcept {
    ProcessBuffer(std::span<BYTE>(pBuffer, bufferSize));
}
```

### Pattern 3: std::string::contains() for String Operations
**What:** Replace `find() != std::string::npos` with clearer `contains()` method.
**When to use:** All string containment checks.

```cpp
// Source: C++23 P1679R3
// BEFORE: Verbose find pattern (current codebase pattern)
if (readerName.find(L"\\\\?PNP?\\") != std::wstring::npos) {
    // Handle PNP reader
}

// AFTER: Clearer contains() method
if (readerName.contains(L"\\\\?PNP?\\")) {
    // Handle PNP reader
}

// Also works with char strings
if (certUsage.contains("smartcard")) {
    // Smart card usage found
}
```

### Pattern 4: API Boundary Conversion (Maintaining C-style Exports)
**What:** Convert modern C++ types to C-style types at exported function boundaries.
**When to use:** Every LSA/Credential Provider exported function.

```cpp
// Source: Pattern for Windows API compatibility
// Internal function using modern C++
void ProcessDataInternal(std::span<BYTE> buffer) noexcept;

// Exported function maintaining C-style API
HRESULT ExportedProcessData(
    __in_bcount(bufferSize) BYTE* pBuffer,
    __in size_t bufferSize) noexcept
{
    if (pBuffer == nullptr || bufferSize == 0) {
        return E_INVALIDARG;
    }

    ProcessDataInternal(std::span<BYTE>(pBuffer, bufferSize));
    return S_OK;
}
```

### Anti-Patterns to Avoid
- **Using std::format in LSASS code:** Can throw `std::bad_alloc` - LSASS crashes
- **Storing std::span in class members:** Dangling reference risk when container reallocates
- **Using std::string_view in class members in LSASS:** Use-after-free credential exposure
- **Passing std::span across DLL boundaries:** ABI considerations, not universally free

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| String formatting | Custom snprintf wrapper | `std::format` (non-LSASS) | Type-safe, no buffer overflows |
| Buffer view | Pointer + size parameters | `std::span<T>` | Bounds-safe, size always available |
| String containment | `find() != npos` | `contains()` | Clearer intent, less error-prone |
| Fixed buffer formatting | Manual size calculation | `std::format_to_n` | Prevents overflow automatically |

**Key insight:** The C++ standard library provides safer alternatives for all common patterns in this codebase. Custom solutions add maintenance burden and security risk.

## Common Pitfalls

### Pitfall 1: std::format in LSASS Context
**What goes wrong:** `std::format` allocates memory and can throw `std::bad_alloc`. In LSASS, uncaught exceptions crash the security subsystem.
**Why it happens:** Developer doesn't realize component runs in LSASS context.
**How to avoid:**
- ONLY use `std::format` in EIDConfigurationWizard (EXE, not LSASS)
- NEVER use in EIDCardLibrary, EIDCredentialProvider, EIDNativeLogon, EIDPasswdChangeNotification
- Keep using `StringCchPrintf` and `swprintf_s` in LSASS components
**Warning signs:** `#include <format>` in any file under EIDCardLibrary, EIDCredentialProvider, etc.

### Pitfall 2: std::span Lifetime Issues
**What goes wrong:** `std::span` is a non-owning view. Storing it in a class member creates dangling references when the underlying container is modified or destroyed.
**Why it happens:** Developer treats span like a string rather than a view.
**How to avoid:**
- Use `std::span` only as function parameter type
- Never store `std::span` in class members
- Convert to owning container if data must persist
**Warning signs:** `std::span<T> m_` member variable naming pattern

### Pitfall 3: std::string_view for Credential Data
**What goes wrong:** Similar to span, string_view can dangle. For credential data, this creates use-after-free security vulnerabilities.
**Why it happens:** Developer optimizes for performance without considering lifetime.
**How to avoid:**
- Use `std::wstring` for all credential data that crosses boundaries
- Only use `std::wstring_view` for immediate function parameters
- Never store credential data in string_view
**Warning signs:** `std::wstring_view` member variables in credential handling classes

### Pitfall 4: CRTP/Deducing This When Not Needed
**What goes wrong:** Implementing deducing `this` patterns when no CRTP exists adds unnecessary complexity.
**Why it happens:** Planning documents mention "evaluating CRTP" without verifying codebase.
**How to avoid:**
- Codebase has NO CRTP patterns - QUAL-02 is NOT APPLICABLE
- Skip deducing `this` modernization entirely
- Document this in phase verification
**Warning signs:** Adding template complexity where none existed before

## Code Examples

### std::format Replacement (EIDConfigurationWizard only)

```cpp
// Source: Current codebase pattern in Tracing.cpp
// BEFORE: C-style formatting with fixed buffer
void EIDCardLibraryTraceEx(LPCSTR szFile, DWORD dwLine, LPCSTR szFunction,
                           UCHAR dwLevel, PCWSTR szFormat,...) {
    WCHAR Buffer[256];
    WCHAR Buffer2[356];
    va_list ap;
    va_start(ap, szFormat);
    ret = _vsnwprintf_s(Buffer, 256, 256, szFormat, ap);
    va_end(ap);
    // ... rest of function
}

// AFTER (if converted to non-LSASS): std::format
// NOTE: This specific function is in LSASS code - CANNOT use std::format
// Example for ConfigurationWizard equivalent:
void LogMessage(std::wstring_view format, auto&&... args) {
    // Only in non-LSASS code (ConfigurationWizard)
    std::wstring message = std::format(format, std::forward<decltype(args)>(args)...);
    OutputDebugStringW(message.c_str());
}
```

### std::span for Buffer Handling

```cpp
// Source: Current codebase pattern in StoredCredentialManagement.cpp
// BEFORE: Raw pointer handling
BOOL CStoredCredentialManager::StorePrivateData(__in DWORD dwRid,
    __in_opt PBYTE pbSecret, __in_opt USHORT usSecretSize) {
    // Manual null checks and size tracking
    if (pbSecret == nullptr && usSecretSize > 0) return FALSE;
    // ... use pbSecret with usSecretSize
}

// AFTER: std::span provides automatic bounds
#include <span>

// Internal function with span
void ProcessSecretInternal(std::span<const BYTE> secret) noexcept {
    // Bounds-safe access
    for (const auto& byte : secret) {
        // Process each byte safely
    }
}

// Exported boundary maintains C-style API
BOOL CStoredCredentialManager::StorePrivateData(__in DWORD dwRid,
    __in_opt PBYTE pbSecret, __in_opt USHORT usSecretSize) noexcept {

    if (pbSecret == nullptr) {
        ProcessSecretInternal({}); // Empty span
    } else {
        ProcessSecretInternal(std::span<const BYTE>(pbSecret, usSecretSize));
    }
    return TRUE;
}
```

### std::string::contains() Usage

```cpp
// Source: Current codebase pattern (hypothetical - no contains usage found)
// BEFORE: find() pattern
bool HasSmartCardEKU(const std::wstring& usage) {
    return usage.find(L"SmartCard") != std::wstring::npos ||
           usage.find(L"smartcard") != std::wstring::npos;
}

// AFTER: contains() pattern (C++23)
bool HasSmartCardEKU(const std::wstring& usage) {
    // Case-insensitive check still needs transform, but simpler logic
    std::wstring lower = usage;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
    return lower.contains(L"smartcard");
}

// Direct substring check
if (readerName.contains(L"\\\\?PNP?\\")) {
    // PNP notification reader
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `sprintf`/`snprintf` | `std::format` | C++20 (2020) | Type-safe, no buffer overflows |
| Pointer + size parameters | `std::span<T>` | C++20 (2020) | Bounds-safe views |
| `str.find(x) != npos` | `str.contains(x)` | C++23 (2023) | Clearer intent |
| CRTP patterns | Deducing `this` | C++23 (2023) | **NOT APPLICABLE** to this codebase |

**Deprecated/outdated:**
- `sprintf`/`sprintf_s`: Use `std::format` where exceptions are allowed
- Raw pointer + size: Use `std::span` for internal boundaries
- Manual substring checks: Use `contains()` for readability

## Open Questions

1. **Should we wrap std::format for EIDConfigurationWizard?**
   - What we know: `std::format` can throw `std::bad_alloc`
   - What's unclear: Whether Configuration Wizard should handle allocation failures
   - Recommendation: Use try/catch around `std::format` in wizard, or use `std::format_to` with pre-allocated buffer

2. **How extensively to apply std::span?**
   - What we know: Current code uses many pointer+size patterns
   - What's unclear: Scope of conversion vs. ROI
   - Recommendation: Convert high-risk buffer operations first (credential handling), defer others

3. **QUAL-02 (Deducing this) applicability?**
   - What we know: Codebase grep shows NO CRTP patterns in source files
   - What's unclear: Why requirement exists
   - Recommendation: Mark QUAL-02 as NOT APPLICABLE - no CRTP patterns to modernize

## Sources

### Primary (HIGH confidence)
- [Microsoft Learn - MSVC C++23 Conformance](https://learn.microsoft.com/en-us/cpp/overview/msvc-conformance-improvements?view=msvc-170) - Feature availability and version requirements
- [GitHub microsoft/STL Wiki - VS 2022 Changelog](https://github.com/microsoft/STL/wiki/VS-2022-Changelog) - Detailed feature implementation history
- [Microsoft Learn - What's new for C++ in Visual Studio](https://learn.microsoft.com/en-us/cpp/overview/what-s-new-for-visual-cpp-in-visual-studio?view=msvc-170) - C++23 preview features

### Secondary (MEDIUM confidence)
- [cppreference - std::format](https://en.cppreference.com/w/cpp/utility/format) - Standard reference
- [cppreference - std::span](https://en.cppreference.com/w/cpp/container/span) - Standard reference
- [cppreference - basic_string::contains](https://en.cppreference.com/w/cpp/string/basic_string/contains) - Standard reference

### Tertiary (LOW confidence)
- [TechForTalk - std::span C++20 Best Practices](https://techfortalk.co.uk/2025/12/30/stdspan-c20-when-to-use-and-not-use-for-safe-buffer-passing/) - Community guidance on span usage

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - All features well-documented in official Microsoft sources
- Architecture: HIGH - Clear patterns established, LSASS constraints documented
- Pitfalls: HIGH - Common C++20/23 pitfalls well-known in community
- QUAL-02 (Deducing this): HIGH confidence it's NOT APPLICABLE - codebase verified

**Research date:** 2026-02-15
**Valid until:** 90 days - C++20/23 features are stable standard features

## Appendix: Current Codebase Patterns

### sprintf/snprintf Family Usage (Files to Update)
Based on codebase grep, the following patterns need evaluation for `std::format` (non-LSASS) or retention (LSASS):

| File | Line | Pattern | Context | LSASS? |
|------|------|---------|---------|--------|
| EIDCardLibrary/Tracing.cpp | 71 | `swprintf_s` | Error message formatting | YES - Keep |
| EIDCardLibrary/Tracing.cpp | 151 | `_vsnwprintf_s` | Trace formatting | YES - Keep |
| EIDCardLibrary/Tracing.cpp | 157 | `swprintf_s` | Debug output | YES - Keep |
| EIDCardLibrary/smartcardmodule.cpp | 55 | `StringCchPrintfW` | Path construction | YES - Keep |
| EIDCardLibrary/StoredCredentialManagement.cpp | 1998 | `StringCchPrintfW` | LSA key name | YES - Keep |
| EIDCardLibrary/StringConversion.cpp | 61 | `vswprintf_s` | Format helper | YES - Keep |

**Conclusion:** ALL current formatting is in LSASS code. `std::format` adoption is LIMITED to new code in EIDConfigurationWizard only.

### std::string::contains() Opportunities
The codebase uses `find()` patterns that could be simplified with `contains()`:
- Current grep found NO uses of `.find(` with `!= npos` pattern
- However, the codebase uses C-style `strstr`/`wcsstr` which could be candidates
- Recommend evaluating on a case-by-case basis

### std::span Opportunities
The codebase has many `PBYTE`/`BYTE*` + size patterns that could use `std::span`:
- `StoredCredentialManagement.cpp` - credential buffer handling
- `Package.cpp` - serialization buffers
- `CContainer.cpp` - smart card data buffers

**Recommendation:** Prioritize credential-related buffer handling for span conversion.

## QUAL-02: Deducing `this` - NOT APPLICABLE

**Verification:** Codebase grep for `CRTP`, `crtp_base`, and template patterns with `Derived` parameter found ZERO matches in source files.

**Recommendation:** Update REQUIREMENTS.md to mark QUAL-02 as NOT APPLICABLE. No CRTP patterns exist in the codebase that could benefit from deducing `this` modernization.
