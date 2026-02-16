# Feature Research: C++23 Language/Library Features

**Domain:** C++23 features for Windows security-critical codebase (LSASS context)
**Researched:** 2026-02-15
**Confidence:** HIGH (verified with Microsoft Learn, cppreference, Microsoft C++ Blog)

---

## Feature Landscape

### Table Stakes (Must Adopt - Safety/Correctness)

These features provide measurable safety improvements. Missing them in security-critical code is a liability.

| Feature | Why Expected | MSVC Status | Complexity | Notes |
|---------|--------------|-------------|------------|-------|
| `std::expected<T, E>` | Exception-less error handling for LSASS | **VS 2022 17.3+** (MSVC 19.33) | MEDIUM | Replace HRESULT patterns with typed error handling; monadic operations in 17.6+ |
| `if consteval` | Compile-time vs runtime branching | **VS 2022 17.14.3+** (bug-fixed) | LOW | Distinguishes constant evaluation context |
| `constexpr` enhancements | More compile-time validation | **VS 2022 17.13+** | LOW | Virtual functions in constexpr, relaxed restrictions |
| `std::to_underlying` | Safer enum conversions | **VS 2022 17.3+** | LOW | Replaces `static_cast<std::underlying_type_t<T>>` |
| `std::unreachable()` | Mark impossible code paths | **VS 2022 17.3+** | LOW | Helps optimizer + static analysis; use carefully in security code |
| `#warning` directive | Compile-time deprecation notices | **VS 2026 18.0** (MSVC 14.50) | LOW | Standardized migration warnings |
| Deducing `this` (explicit object) | Cleaner CRTP patterns | **VS 2022 17.2+** | MEDIUM | Reduces template metaprogramming complexity |

### Differentiators (Nice to Have - Code Quality)

These improve code quality but aren't essential for security. Adopt incrementally.

| Feature | Value Proposition | MSVC Status | Complexity | Notes |
|---------|-------------------|-------------|------------|-------|
| `std::format` / `std::format_string` | Type-safe formatting | **VS 2022 17.2+** | LOW | Replaces `sprintf`/`snprintf`; avoid in LSASS (logging only) |
| `std::print` / `std::println` | Modern console output | **VS 2022 17.12+** | LOW | **AVOID in LSASS** - no console; use for test/debug builds only |
| `auto(x)` decay-copy | Cleaner template code | **VS 2026 18.0+** | LOW | Simplifies explicit decay operations |
| Multidimensional `operator[]` | Cleaner array access | **VS 2022 17.3+** | LOW | `arr[i, j]` instead of `arr(i, j)` |
| `std::string::contains()` | Simpler string searching | **VS 2022 17.1+** | LOW | Note: also added `contains` to `string_view` |
| `std::is_scoped_enum` | Type trait for enums | **VS 2022 17.3+** | LOW | Template metaprogramming helper |
| Static `operator[]` | Type-erased subscript | **VS 2022 17.3+** | MEDIUM | Enables more generic code |
| `std::mdspan` | Multidimensional array view | **VS 2022 17.10+** | MEDIUM | Useful for buffer handling; verify memory safety |
| `consteval` (improved) | Guaranteed compile-time | **VS 2022 17.14+** | MEDIUM | Stricter than `constexpr`; had MSVC bugs, fixed in 17.14 |

### Anti-Features (Deliberately NOT Use)

Features to explicitly avoid in this security-critical Windows LSASS context.

| Anti-Feature | Why Avoid | MSVC Status | What to Do Instead |
|--------------|-----------|-------------|-------------------|
| `std::print` / `std::println` in LSASS | No console in LSASS; UTF-8 buffer bugs | Implemented | Use ETW logging or `OutputDebugString` |
| Exceptions | LSASS stability; stack unwinding overhead | N/A | `std::expected<T, E>` with error codes |
| `import std;` (modules) | Immature MSVC support; CMake issues | Partial (VS 2022 17.5+) | Traditional `#include` headers |
| `std::stacktrace` | Known bugs; underlying API issues | Partial (buggy) | `CaptureStackBackTrace` Win32 API |
| `std::flat_map` / `std::flat_set` | **NOT IMPLEMENTED** in MSVC | Missing | `std::map`, `std::unordered_map` |
| `std::generator` | **NOT IMPLEMENTED** in MSVC | Missing | Callbacks, ranges, iterators |
| Coroutines | Stack complexity in LSASS; memory overhead | Implemented | Synchronous patterns |
| Dynamic CRT (`/MD`) | LSASS isolation issues | N/A | Static CRT (`/MT`) |
| `/std:c++23` flag | **NOT YET STABLE** | Missing | `/std:c++23preview` or `/std:c++latest` |
| v145 toolset | Drops Windows 7/8/8.1 support | VS 2026 | v143 toolset (VS 2022) |

---

## Feature Dependencies

```
C++23 Preview (/std:c++23preview)
    |
    +-- std::expected<T, E>
    |       |
    |       +-- requires: C++23 monadic operations (and_then, transform)
    |
    +-- Deducing this
    |       |
    |       +-- replaces: CRTP patterns
    |
    +-- if consteval
            |
            +-- requires: consteval functions
            +-- conflicts-with: runtime-only code paths

C++20 Foundation
    |
    +-- std::format
    +-- std::span
    +-- Concepts
    +-- std::string::starts_with / ends_with (C++20, not C++23)

Windows 7 Compatibility
    |
    +-- requires: v143 toolset
    +-- conflicts-with: v145 toolset (VS 2026)
    +-- conflicts-with: Some newer Windows SDK APIs
```

### Dependency Notes

- **`std::expected` requires C++23 preview**: Full monadic interface (`and_then`, `transform`, `or_else`) available in VS 2022 17.6+
- **`if consteval` requires consteval context**: Only meaningful when combined with `consteval` functions or `constexpr` if branches
- **Deducing `this` replaces CRTP**: Eliminates need for curiously recurring template pattern in many cases
- **v143 toolset required for Windows 7**: v145 (VS 2026) drops Windows 7/8/8.1 support per MSVC 14.50 release notes

---

## MVP Definition

### Phase 1: Foundation (Adopt First)

Essential safety improvements - prioritize these.

- [x] **`std::expected<T, E>`** - Replace HRESULT error handling with typed errors; adopt monadic interface
- [x] **`if consteval`** - Enable compile-time vs runtime branching
- [x] **`std::to_underlying`** - Safer enum to integer conversions
- [x] **`std::unreachable()`** - Mark impossible code paths (with extreme care in security code)
- [x] **Deducing `this`** - Modernize CRTP patterns in existing code

**Rationale:** These features provide the highest safety value with lowest risk. All are stable in MSVC 19.43+.

### Phase 2: Quality Improvements (After Foundation)

Code quality improvements - adopt after Phase 1 is complete.

- [ ] **`std::format`** - Replace `sprintf`/`snprintf` in non-LSASS code (Configuration Wizard, logging)
- [ ] **`constexpr` enhancements** - Move more computations to compile-time
- [ ] **`auto(x)` decay-copy** - Simplify template code
- [ ] **`std::string::contains()`** - Cleaner string operations
- [ ] **Multidimensional `operator[]`** - If applicable to buffer handling

**Rationale:** These improve code quality but require more extensive code changes.

### Phase 3: Future Consideration (Not Yet)

Features to defer until MSVC support matures or requirements change.

- [ ] **`import std;`** - Wait for stable MSVC modules support
- [ ] **`std::stacktrace`** - Wait for MSVC bug fixes
- [ ] **`std::flat_map`** - Wait for MSVC implementation
- [ ] **`/std:c++23` stable flag** - Wait for official MSVC release
- [ ] **v145 toolset** - Only if Windows 7 support is dropped

**Rationale:** These features either have incomplete/buggy implementations or require breaking compatibility changes.

---

## Feature Prioritization Matrix

| Feature | Security Value | Implementation Cost | MSVC Stability | Priority |
|---------|---------------|---------------------|----------------|----------|
| `std::expected<T, E>` | HIGH | MEDIUM | STABLE | **P1** |
| `if consteval` | MEDIUM | LOW | STABLE | **P1** |
| `std::to_underlying` | MEDIUM | LOW | STABLE | **P1** |
| `std::unreachable()` | MEDIUM | LOW | STABLE | **P1** |
| Deducing `this` | LOW | MEDIUM | STABLE | **P1** |
| `std::format` | LOW | MEDIUM | STABLE | P2 |
| `constexpr` enhancements | MEDIUM | MEDIUM | STABLE | P2 |
| `auto(x)` decay-copy | LOW | LOW | STABLE | P2 |
| `std::string::contains()` | LOW | LOW | STABLE | P2 |
| `import std;` | LOW | HIGH | IMMATURE | P3 |
| `std::stacktrace` | LOW | MEDIUM | BUGGY | P3 |
| `std::flat_map` | LOW | MEDIUM | MISSING | P3 |

**Priority key:**
- **P1:** Must adopt - safety/correctness improvements
- **P2:** Should adopt - code quality improvements
- **P3:** Defer - immature or missing implementations

---

## Security-Critical Considerations

### LSASS Context Constraints

Running in LSASS imposes specific constraints on C++23 feature adoption:

| Constraint | Impact | Guidance |
|------------|--------|----------|
| No exceptions allowed | Critical | Use `std::expected<T, E>` exclusively; mark functions `noexcept` |
| No console I/O | High | `std::print`/`std::println` will fail; use ETW or debug output |
| Memory pressure | Medium | Prefer stack allocation; avoid dynamic containers in hot paths |
| Stack complexity | Medium | Avoid coroutines; prefer synchronous patterns |
| Static CRT required | High | Use `/MT` not `/MD` for runtime isolation |

### C-Style API Boundaries

The project has LSA and Credential Provider interfaces requiring C-compatible exports:

| Pattern | C++23 Solution |
|---------|----------------|
| `HRESULT` returns | `std::expected<T, HRESULT>` at internal boundaries; convert to `HRESULT` at API boundary |
| `BOOL` returns | `std::expected<T, error_code>` internally; convert to `BOOL` at API boundary |
| String parameters | `std::wstring_view` internally; `LPCWSTR` at API boundary |
| Buffer parameters | `std::span<T>` internally; `T*` + `size_t*` at API boundary |
| Error information | `std::expected` error type; convert to `HRESULT` or `GetLastError()` pattern |

### Exception Safety Pattern

```cpp
// BEFORE (C++14 pattern with HRESULT)
HRESULT ReadCardData(_Out_ CardData* data) {
    if (!data) return E_POINTER;
    HRESULT hr = ValidateCard();
    if (FAILED(hr)) return hr;
    hr = ReadFromCard(data);
    if (FAILED(hr)) return hr;
    return S_OK;
}

// AFTER (C++23 pattern with std::expected)
std::expected<CardData, CardError> ReadCardData() noexcept {
    auto validation = ValidateCard();
    if (!validation) {
        return std::unexpected(validation.error());
    }
    return ReadFromCard();  // Returns expected<CardData, CardError>
}

// API boundary conversion
HRESULT LSA_ReadCardData(_Out_ CardData* data) noexcept {
    if (!data) return E_POINTER;
    auto result = ReadCardData();
    if (!result) {
        return result.error().ToHRESULT();
    }
    *data = *result;
    return S_OK;
}
```

---

## Compiler Support Reference

### MSVC Version Requirements

| Feature | Minimum VS Version | Minimum MSVC | Notes |
|---------|-------------------|--------------|-------|
| `std::expected` | VS 2022 17.3 | 19.33 | Monadic ops in 17.6 |
| `if consteval` | VS 2022 17.14.3 | 19.44 | Bug fixes required |
| Deducing `this` | VS 2022 17.2 | 19.32 | Full support |
| `std::format` | VS 2022 17.2 | 19.32 | Complete |
| `std::print` | VS 2022 17.12 | 19.40+ | UTF-8 bugs exist |
| `std::to_underlying` | VS 2022 17.3 | 19.33 | Simple utility |
| `std::unreachable` | VS 2022 17.3 | 19.33 | Optimization hint |
| `std::mdspan` | VS 2022 17.10 | 19.38+ | Multidimensional view |
| `auto(x)` decay-copy | VS 2026 18.0 | 19.50 | Latest only |
| `#warning` directive | VS 2026 18.0 | 19.50 | Latest only |

### Recommended Configuration

```xml
<!-- .vcxproj settings for C++23 security-critical code -->
<PropertyGroup>
  <LanguageStandard>stdcpp23preview</LanguageStandard>
  <PlatformToolset>v143</PlatformToolset>
  <RuntimeLibrary>MultiThreaded</RuntimeLibrary>  <!-- /MT for LSASS -->
  <SDLCheck>true</SDLCheck>
  <ControlFlowGuard>Guard</ControlFlowGuard>
</PropertyGroup>
```

---

## Sources

### Official Documentation
- [Microsoft Learn - MSVC C++23 Conformance](https://learn.microsoft.com/en-us/cpp/overview/msvc-conformance-improvements?view=msvc-170) (HIGH confidence)
- [Microsoft Learn - std::expected](https://devblogs.microsoft.com/cppblog/cpp23s-optional-and-expected/) (HIGH confidence)
- [Microsoft Learn - C++23 Deducing This](https://devblogs.microsoft.com/cppblog/cpp23-deducing-this/) (HIGH confidence)
- [Microsoft Learn - Compiler Options](https://learn.microsoft.com/en-us/cpp/build/reference/compiler-options) (HIGH confidence)

### Community References
- [cppreference - C++23 Compiler Support](https://en.cppreference.com/w/cpp/compiler_support/23) (HIGH confidence)
- [cppreference - std::expected](https://en.cppreference.com/w/cpp/utility/expected) (HIGH confidence)
- [C++ Stories - C++23 Library Features](https://www.cppstories.com/2024/cpp23_lib/) (MEDIUM confidence)
- [Modernes C++ - std::expected](https://www.modernescpp.com/index.php/c23-a-new-way-of-error-handling-with-stdexpected/) (MEDIUM confidence)

### Bug Reports and Issues
- [GitHub - MSVC std::stacktrace issues](https://developercommunity.visualstudio.com/t/10692305) (MEDIUM confidence)
- [GitHub - MSVC std::print UTF-8 bug](https://github.com/microsoft/STL/issues/5894) (HIGH confidence)
- [GitHub - modules-report MSVC status](https://github.com/royjacobson/modules-report) (MEDIUM confidence)

### Security References
- [Microsoft Security Blog - LSASS Protection](https://www.microsoft.com/en-us/security/blog/2022/10/05/detecting-and-preventing-lsass-credential-dumping-attacks/) (HIGH confidence)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) (HIGH confidence)

---

## Summary

For EIDAuthentication C++23 modernization in a security-critical LSASS context:

1. **Must Adopt (P1):** `std::expected`, `if consteval`, `std::to_underlying`, `std::unreachable`, Deducing `this`
2. **Should Adopt (P2):** `std::format`, `constexpr` enhancements, `auto(x)`, `std::string::contains()`
3. **Avoid:** Exceptions, `std::print` in LSASS, modules (`import std`), `std::stacktrace`, `std::flat_map`
4. **Toolset:** v143 (VS 2022) for Windows 7 compatibility; v145 drops Win7
5. **Flag:** `/std:c++23preview` (stable `/std:c++23` not yet available)

**Confidence Assessment:** HIGH - All findings verified against official Microsoft documentation, cppreference, and community sources.

---

*Feature research for: C++23 modernization of Windows security-critical codebase*
*Researched: 2026-02-15*
