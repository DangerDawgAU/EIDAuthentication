# Phase 3: Compile-Time Enhancements - Research

**Researched:** 2026-02-15
**Domain:** C++23 compile-time features (`if consteval`, `constexpr` extensions, `std::to_underlying`, `std::unreachable`) for Windows authentication (LSASS)
**Confidence:** HIGH

## Summary

This phase adopts four C++23 compile-time features to improve code safety, enable compile-time validation, and optimize runtime performance. The codebase contains 14 enum definitions that can benefit from `std::to_underlying()` to replace unsafe casts. `if consteval` enables compile-time vs runtime path differentiation in validation routines. `std::unreachable()` provides optimization hints but requires extreme caution in security-critical LSASS code.

The key technical challenges are: (1) `if consteval` support was added in VS 2022 17.14 (May 2025), (2) `std::unreachable()` triggers undefined behavior if executed - must only be used on truly unreachable paths, (3) `std::to_underlying()` is library-only (VS 2022 17.0+) and straightforward to adopt, (4) constexpr extensions require identifying validation routines that can be made compile-time evaluable.

**Primary recommendation:** Start with `std::to_underlying()` (safe, easy win), then extend `constexpr` to validation routines, use `if consteval` where compile-time paths differ from runtime, and use `std::unreachable()` ONLY on paths that are provably impossible (never on security validation failure paths).

## Standard Stack

### Core
| Feature | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| `std::to_underlying()` | VS 2022 17.0 | Convert enum to underlying type | Type-safe, replaces `static_cast<underlying_type_t<E>>` |
| `if consteval` | VS 2022 17.14 | Distinguish compile-time from runtime | Cleaner than `std::is_constant_evaluated()` |
| `constexpr` (extended) | C++23 | More constexpr contexts | Compile-time validation |
| `std::unreachable()` | VS 2022 17.2 | Mark impossible code paths | Optimization hint for compiler |

### Supporting
| Component | Purpose | When to Use |
|-----------|---------|-------------|
| `<utility>` header | `std::to_underlying()` | Enum-to-integer conversions |
| `<utility>` header | `std::unreachable()` | Truly unreachable switch defaults |
| `constexpr` specifier | Compile-time evaluation | Validation routines, simple calculations |
| `consteval` specifier | Force compile-time evaluation | Immediate functions |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `std::to_underlying(e)` | `static_cast<std::underlying_type_t<decltype(e)>>(e)` | More verbose, error-prone |
| `if consteval` | `if (std::is_constant_evaluated())` | Less readable, C++20 pattern |
| `std::unreachable()` | `__assume(0)` (MSVC intrinsic) | Non-portable, less clear intent |
| `std::unreachable()` | `std::terminate()` | Runtime overhead, not optimization |

**Header requirements:**
```cpp
#include <utility>  // std::to_underlying, std::unreachable

// No header needed for if consteval (language feature)
```

## Architecture Patterns

### Target Enum Locations
```
EIDCardLibrary/
├── EIDCardLibrary.h
│   ├── EID_INTERACTIVE_LOGON_SUBMIT_TYPE  // enum (line 61)
│   ├── EID_PROFILE_BUFFER_TYPE            // enum (line 85)
│   ├── EID_CREDENTIAL_PROVIDER_READER_STATE // enum (line 132)
│   ├── EID_CALLPACKAGE_MESSAGE            // enum (line 140)
│   ├── EID_MESSAGE_STATE                  // enum (line 245)
│   ├── EID_MESSAGE_TYPE                   // enum (line 254)
│   └── EID_SSP_CALLER                     // enum (line 265)
├── StoredCredentialManagement.h
│   └── EID_PRIVATE_DATA_TYPE              // enum (line 23)
└── GPO.h
    └── GPOPolicy                          // enum (line 23)

EIDCredentialProvider/
├── common.h
│   ├── SAMPLE_FIELD_ID                    // enum (line 35)
│   └── SAMPLE_MESSAGE_FIELD_ID            // enum (line 47)
└── CMessageCredential.h
    └── CMessageCredentialStatus           // enum (line 32)

EIDConfigurationWizard/
└── CContainerHolder.h
    └── CheckType                          // enum (line 26)
```

### Pattern 1: std::to_underlying for Enum Conversions
**What:** Replace verbose static_cast with `std::to_underlying()`.
**When to use:** Any enum-to-integer conversion.

```cpp
// Source: C++23 standard P1682R3
#include <utility>

enum class EID_MESSAGE_TYPE : int {
    Message = 1,
    CardInserted = 2,
    CardRemoved = 3,
    // ...
};

// BEFORE (C++20):
int typeValue = static_cast<std::underlying_type_t<EID_MESSAGE_TYPE>>(messageType);

// AFTER (C++23):
int typeValue = std::to_underlying(messageType);

// Works with both scoped and unscoped enums
enum EID_PROFILE_BUFFER_TYPE { EIDInteractiveProfile = 2 };
int profileValue = std::to_underlying(EIDInteractiveProfile);  // Returns 2
```

### Pattern 2: if consteval for Dual Compile/Runtime Paths
**What:** Execute different code at compile-time vs runtime.
**When to use:** When compile-time evaluation needs different approach than runtime.

```cpp
// Source: P1938R3 (Microsoft C++ Blog, May 2025)
// https://devblogs.microsoft.com/cppblog/c-language-updates-in-msvc-in-visual-studio-2022-17-14/

#include <cmath>

constexpr double compute_value(double x) {
    if consteval {
        // Compile-time path: use constexpr-friendly operations
        // May use different algorithm or simplified calculation
        return x * x;  // Simplified for compile-time
    } else {
        // Runtime path: can use non-constexpr operations
        return std::sqrt(x);  // std::sqrt not constexpr in all contexts
    }
}

// Validation example for this codebase
constexpr bool validate_pin_length(size_t length) {
    if consteval {
        // Compile-time: strict validation for constant expressions
        return length >= 4 && length <= 16;
    } else {
        // Runtime: can call Windows APIs for policy checks
        return length >= 4 && length <= 16;  // Or read from registry
    }
}

static_assert(validate_pin_length(6));  // Compile-time check
bool runtime_check = validate_pin_length(user_input);  // Runtime check
```

### Pattern 3: Extended constexpr for Validation Routines
**What:** Mark validation functions as constexpr for compile-time checks.
**When to use:** Pure validation logic that can run at compile-time.

```cpp
// Source: C++23 extended constexpr (P2448R2 relaxed restrictions)

// BEFORE (C++20): Could mark constexpr, but restrictions applied
bool is_valid_message_type(int type) {
    return type >= 1 && type <= 10;
}

// AFTER (C++23): More relaxed constexpr rules
constexpr bool is_valid_message_type(int type) noexcept {
    return type >= 1 && type <= 10;
}

// Can now use in static_assert
static_assert(is_valid_message_type(static_cast<int>(EID_MESSAGE_TYPE::Message)));

// More complex validation
constexpr bool validate_buffer_size(size_t size, size_t max_size) noexcept {
    return size > 0 && size <= max_size;
}

// Compile-time validation of constants
constexpr size_t MAX_PIN_LENGTH = 16;
static_assert(validate_buffer_size(8, MAX_PIN_LENGTH));
```

### Pattern 4: std::unreachable for Truly Impossible Paths (CAUTION)
**What:** Mark code paths that are provably never executed.
**When to use:** ONLY on paths that are logically impossible - NEVER on error handling paths.

```cpp
// Source: P0627R6, Microsoft Learn
// https://learn.microsoft.com/en-us/cpp/standard-library/utility-functions

#include <utility>

enum class EID_OPERATION_RESULT {
    Success,
    Failure,
    Pending
};

void handle_result(EID_OPERATION_RESULT result) {
    switch (result) {
        case EID_OPERATION_RESULT::Success:
            // Handle success
            break;
        case EID_OPERATION_RESULT::Failure:
            // Handle failure
            break;
        case EID_OPERATION_RESULT::Pending:
            // Handle pending
            break;
        default:
            // UNREACHABLE: All enum cases covered
            // Only safe because we handle ALL possible values above
            std::unreachable();
    }
}

// DANGEROUS - NEVER DO THIS IN SECURITY CODE:
void validate_certificate(PCCERT_CONTEXT pCert) {
    if (!pCert) {
        // WRONG: This path CAN be reached!
        // std::unreachable();  // NEVER mark failure paths as unreachable!
        return;
    }
    // ...
}
```

### Anti-Patterns to Avoid
- **Using std::unreachable on error paths:** If code CAN be reached, even for "impossible" errors, it's undefined behavior
- **Using std::unreachable in security validation:** Attackers may find ways to trigger "unreachable" paths
- **Assuming compiler validates unreachable:** The compiler TRUSTS you - if wrong, undefined behavior
- **Forgetting to cover all enum cases:** Default case with unreachable only safe if ALL values handled

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Enum to underlying type | `static_cast<std::underlying_type_t<E>>(e)` | `std::to_underlying(e)` | Simpler, type-deduced, less error-prone |
| Compile-time detection | `std::is_constant_evaluated()` | `if consteval` | Cleaner syntax, more intuitive |
| Unreachable marker | `__assume(0)` or `__builtin_unreachable()` | `std::unreachable()` | Standard, portable, self-documenting |
| Custom compile-time assert | `static_assert(condition, msg)` | `constexpr` functions + `static_assert` | Standard mechanism |

**Key insight:** All four features are standard C++23 with full MSVC support (VS 2022 17.14+). No custom implementations needed.

## Common Pitfalls

### Pitfall 1: std::unreachable Triggers Undefined Behavior
**What goes wrong:** If `std::unreachable()` is actually executed, the program has undefined behavior - anything can happen including silent data corruption.
**Why it happens:** Developer assumes a path is impossible but it isn't (e.g., memory corruption, unexpected input, security attack).
**How to avoid:**
1. NEVER use on error handling paths
2. NEVER use on paths that depend on external input
3. ONLY use on switch defaults where ALL enum cases are handled
4. In LSASS context: Prefer explicit error handling over unreachable

**Warning signs:** Any `std::unreachable()` in code that handles user input, certificates, or authentication data.

### Pitfall 2: if consteval Requires VS 2022 17.14+
**What goes wrong:** `if consteval` won't compile on older MSVC versions.
**Why it happens:** Feature was implemented in VS 2022 17.14 (May 2025).
**How to avoid:** Verify `_MSC_VER >= 1944` or use feature test macro `__cpp_lib_is_constant_evaluated` for detection.
**Warning signs:** Compiler error "expected '(' before 'consteval'" or similar syntax errors.

### Pitfall 3: constexpr with External Dependencies
**What goes wrong:** Functions marked `constexpr` can't call Windows APIs or use non-constexpr code.
**Why it happens:** Compile-time functions must be fully evaluable by compiler.
**How to avoid:** Use `if consteval` to separate compile-time and runtime paths, or mark only the pure-logic parts as constexpr.
**Warning signs:** "constexpr function cannot result in a constant expression" errors.

### Pitfall 4: Forgetting noexcept on constexpr Functions
**What goes wrong:** Constexpr functions in LSASS context should also be noexcept.
**Why it happens:** constexpr and noexcept are orthogonal specifiers.
**How to avoid:** Add both specifiers to validation functions that don't allocate.
**Warning signs:** Code review finds `constexpr` without `noexcept` in EIDCardLibrary.

### Pitfall 5: std::to_underlying Requires <utility>
**What goes wrong:** Using `std::to_underlying` without including `<utility>` causes compile error.
**Why it happens:** New C++23 function, developers may not know the header.
**How to avoid:** Always include `<utility>` when using `std::to_underlying` or `std::unreachable`.
**Warning signs:** "'to_underlying': is not a member of 'std'" error.

## Code Examples

### Complete std::to_underlying Migration Example

```cpp
// Source: Pattern for enum conversion modernization

// BEFORE (Current codebase pattern):
// If any enum casts exist, they look like:
int GetMessageTypeValue(EID_MESSAGE_TYPE type) {
    return static_cast<int>(type);  // Simple for int-based enum
}

// After complex scenarios with underlying_type:
template<typename E>
auto get_enum_value(E e) {
    return static_cast<std::underlying_type_t<E>>(e);
}

// AFTER (C++23 with std::to_underlying):
#include <utility>

int GetMessageTypeValue(EID_MESSAGE_TYPE type) noexcept {
    return std::to_underlying(type);
}

// Generic helper no longer needed - std::to_underlying does it
```

### Compile-Time Validation Pattern

```cpp
// Source: Pattern for constexpr validation in LSASS context
#pragma once

#include <utility>
#include <cstddef>

namespace EID {

// Compile-time validated constants
constexpr size_t MIN_PIN_LENGTH = 4;
constexpr size_t MAX_PIN_LENGTH = 16;
constexpr size_t CERT_HASH_LENGTH = 32;  // SHA-256

// Compile-time validation function
constexpr bool is_valid_pin_length(size_t length) noexcept {
    return length >= MIN_PIN_LENGTH && length <= MAX_PIN_LENGTH;
}

// Compile-time buffer size validation
constexpr bool is_valid_buffer_size(size_t size, size_t max_size) noexcept {
    return size > 0 && size <= max_size;
}

// Dual-path validation (compile-time vs runtime)
constexpr bool validate_hash_length(size_t length) noexcept {
    if consteval {
        // Compile-time: strict constant check
        return length == CERT_HASH_LENGTH;
    } else {
        // Runtime: could read from config (but keeping simple for LSASS)
        return length == CERT_HASH_LENGTH;
    }
}

} // namespace EID

// Compile-time assertions
static_assert(EID::is_valid_pin_length(6), "Default PIN length must be valid");
static_assert(EID::validate_hash_length(32), "SHA-256 hash must be 32 bytes");
```

### Safe std::unreachable Usage (Exhaustive Switch)

```cpp
// Source: Pattern for safe std::unreachable on exhaustive enum switches
#include <utility>

enum class EID_CALLPACKAGE_MESSAGE {
    Message = 1,
    CardInserted = 2,
    CardRemoved = 3,
    // Only 3 valid values defined
};

const char* get_message_name(EID_CALLPACKAGE_MESSAGE msg) noexcept {
    switch (msg) {
        case EID_CALLPACKAGE_MESSAGE::Message:
            return "Message";
        case EID_CALLPACKAGE_MESSAGE::CardInserted:
            return "CardInserted";
        case EID_CALLPACKAGE_MESSAGE::CardRemoved:
            return "CardRemoved";
        default:
            // SAFE: All enum values handled above
            // This path is truly unreachable if enum is used correctly
            std::unreachable();
    }
}

// DANGEROUS - DO NOT USE:
// If enum can have arbitrary integer values cast to it:
void unsafe_handle(EID_CALLPACKAGE_MESSAGE msg) {
    // If someone does: (EID_CALLPACKAGE_MESSAGE)999
    // This becomes reachable!
    switch (msg) {
        // ... cases ...
        default:
            // WRONG: Not truly unreachable if enum value is cast from int
            // std::unreachable();  // DANGEROUS!
            return;  // Safe fallback
    }
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `static_cast<underlying_type_t<E>>(e)` | `std::to_underlying(e)` | C++23 (VS 2022 17.0) | Cleaner, type-deduced |
| `if (std::is_constant_evaluated())` | `if consteval` | C++23 (VS 2022 17.14) | More readable |
| `__assume(0)` (MSVC) | `std::unreachable()` | C++23 (VS 2022 17.2) | Standard, portable |
| Runtime validation only | Compile-time + runtime | C++23 | Earlier error detection |

**Deprecated/outdated:**
- `std::is_constant_evaluated()` in `if` conditions: Use `if consteval` instead
- Manual `static_cast` for enums: Use `std::to_underlying()` instead
- `__assume(0)`: Use standard `std::unreachable()` instead

## Open Questions

1. **Should std::unreachable be used in LSASS code at all?**
   - What we know: UB on execution, security risk if misused
   - What's unclear: Whether optimizer benefit outweighs risk
   - Recommendation: Use ONLY on exhaustive switch defaults with all cases handled. Consider using `__assume(0)` with debug builds check instead.

2. **Which validation routines should be made constexpr?**
   - What we know: Many simple checks (length validation, bounds checking) can be constexpr
   - What's unclear: Full inventory of candidate functions
   - Recommendation: Start with constants validation (PIN length, buffer sizes), expand incrementally

3. **How to handle if consteval in functions with Windows API calls?**
   - What we know: Windows APIs aren't constexpr-evaluable
   - What's unclear: Best pattern for dual-path functions
   - Recommendation: Use `if consteval` to separate compile-time logic from runtime API calls

## Sources

### Primary (HIGH confidence)
- [Microsoft C++ Blog - C++ Language Updates in MSVC in Visual Studio 2022 17.14](https://devblogs.microsoft.com/cppblog/c-language-updates-in-msvc-in-visual-studio-2022-17-14/) - `if consteval` implementation confirmation (May 2025)
- [Microsoft Learn - C++23 Standard library features](https://learn.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance) - Feature support table showing `std::to_underlying()` (VS 2022 17.0), `std::unreachable()` (VS 2022 17.2)
- [cppreference - std::to_underlying](https://en.cppreference.com/w/cpp/utility/to_underlying) - Standard reference
- [cppreference - std::unreachable](https://en.cppreference.com/w/cpp/utility/unreachable) - Standard reference with UB warning

### Secondary (MEDIUM confidence)
- [C++ Stories - Enum Class Improvements for C++17, C++20 and C++23](https://www.cppstories.com/2024/enum-improvements/) - `std::to_underlying` usage patterns
- [dev.to - std::unreachable: The Standard Function for Inserting Undefined Behavior](https://dev.to/pauljlucas/unreachable-the-standard-function-for-inserting-undefined-behavior-4c9o) - Safety discussion

### Tertiary (LOW confidence)
- [Sándor Dargo's Blog - C++23: Even more constexpr](https://www.sandordargo.com/blog/2023/05/24/cpp23-constexpr) - Constexpr extensions overview

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - All four features documented in Microsoft conformance tables
- Architecture: HIGH - Clear usage patterns from standard references
- Pitfalls: HIGH - Well-documented undefined behavior risks for `std::unreachable`

**Research date:** 2026-02-15
**Valid until:** 90 days - C++23 features are stable standard, MSVC support confirmed

## Appendix: MSVC Version Requirements

| Feature | Minimum MSVC | Minimum VS | Status |
|---------|--------------|------------|--------|
| `std::to_underlying()` | 19.30 | VS 2022 17.0 | Stable |
| `std::unreachable()` | 19.33 | VS 2022 17.2 | Stable |
| `if consteval` | 19.44 | VS 2022 17.14 | Stable (May 2025) |
| Extended constexpr | 19.43+ | VS 2022 17.13 | Stable |

**Current project configuration:** v143 toolset, `/std:c++23preview` - All features available.

## Appendix: Requirements Mapping

| Requirement | Feature | Implementation |
|-------------|---------|----------------|
| COMPILE-01 | `if consteval` | Distinguish compile-time vs runtime paths in validation routines |
| COMPILE-02 | `constexpr` extension | Mark validation functions constexpr where beneficial |
| COMPILE-03 | `std::to_underlying` | Replace unsafe enum casts with type-safe function |
| COMPILE-04 | `std::unreachable()` | Mark impossible code paths (with extreme caution) |
