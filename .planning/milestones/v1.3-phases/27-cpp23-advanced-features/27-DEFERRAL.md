---
phase: 27-cpp23-advanced-features
document_type: deferral
created: 2026-02-18
requirements: CPP23-01, CPP23-02, CPP23-03
decision: deferred
milestone_target: future (post-v1.3)
---

# Phase 27: C++23 Advanced Features - Deferral Documentation

**Document Type:** Formal Deferral Record
**Created:** 2026-02-18
**Decision:** All three C++23 features deferred with justification

## Section 1: Deferral Summary

| Requirement ID | Feature | MSVC Status | Decision | Alternative |
|----------------|---------|-------------|----------|-------------|
| CPP23-01 | `import std;` modules | Partial support, immature tooling, CMake experimental | Won't-fix - Defer to future milestone | Continue using traditional `#include` headers |
| CPP23-02 | `std::flat_map` / `std::flat_set` | NOT IMPLEMENTED (only GCC 15+/Clang 20+) | Won't-fix - Defer until MSVC implements | Continue using `std::map` / `std::set` |
| CPP23-03 | `std::stacktrace` | Partial/buggy in MSVC 19.34+ | Won't-fix - Use CaptureStackBackTrace Win32 API | Phase 28 will implement `CaptureStackBackTrace` |

## Section 2: Detailed Justifications

### CPP23-01: C++23 Modules (`import std;`)

**MSVC Status:** Partial support in 19.35+, marked "partial" in cppreference compiler support table

**CMake Status:** Experimental in 3.28+, still evolving with breaking changes possible

**Conformance:** Incomplete implementation, not production-ready for security-critical code

**Risk Assessment:** Build system instability and conformance gaps outweigh potential build time benefits

**Decision:** Defer until MSVC conformance is complete and CMake support is non-experimental

**Re-evaluation triggers:**
- CMake module support becomes non-experimental
- MSVC marks modules as fully conformant
- VS 2027 or later major version release

### CPP23-02: Flat Containers (`std::flat_map`, `std::flat_set`)

**MSVC Status:** NOT IMPLEMENTED - cppreference compiler support table shows empty MSVC column

**GCC/Clang:** GCC 15+ and Clang 20+ have implementations available

**Timeline:** No Microsoft implementation timeline has been announced

**Current State:** Existing `std::map` and `std::set` containers perform adequately for this codebase

**Decision:** Defer until MSVC implements; no custom replacement warranted

**Re-evaluation triggers:**
- MSVC announces flat container implementation
- Performance profiling indicates cache locality issues with current containers

### CPP23-03: Stack Trace (`std::stacktrace`)

**MSVC Status:** Partial in 19.34+, known bugs with Windows symbol handling

**Reliability:** Produces corrupted or empty traces in some scenarios

**LSASS Impact:** Unreliable stack traces are unacceptable in security-critical LSASS context

**Alternative:** `CaptureStackBackTrace` Win32 API with `SymFromAddr` for symbol resolution

**Decision:** Won't-fix for `std::stacktrace`; Phase 28 will implement `CaptureStackBackTrace` for diagnostics

**Implementation pattern for Phase 28:**
```cpp
#include <windows.h>
#include <dbghelp.h>

constexpr USHORT MAX_STACK_FRAMES = 32;

void CaptureStackTrace() noexcept {
    PVOID stack[MAX_STACK_FRAMES];
    USHORT frameCount = CaptureStackBackTrace(
        2,  // Skip this function and caller
        MAX_STACK_FRAMES,
        stack,
        nullptr
    );
    // Use SymFromAddr for symbol resolution if needed
}
```

## Section 3: Phase 28 Recommendations

Phase 28 (Diagnostics Improvements) can proceed without any C++23 advanced features:

| Capability | Recommended Approach | Rationale |
|------------|---------------------|-----------|
| Stack traces | `CaptureStackBackTrace` Win32 API | Reliable, Windows-native, LSASS-safe |
| Structured logging | `OutputDebugString` / ETW | Windows-native, well-supported |
| Error handling | `std::expected` (already adopted) | C++23 feature already in use |
| String formatting | `std::format` (non-LSASS code) | C++23 feature available where safe |
| Headers | Traditional `#include` headers | Mature tooling, full MSVC support |

**No blocking dependencies:** All Phase 28 diagnostics work can proceed with existing tooling and Windows APIs.

## Section 4: Re-evaluation Triggers

This deferral should be re-evaluated when any of the following occur:

### Modules (`import std;`)
- [ ] CMake module support becomes non-experimental (stable API)
- [ ] MSVC marks C++23 modules as fully conformant
- [ ] VS 2027 or later major version released
- [ ] Build time becomes a significant development bottleneck

### Flat Containers (`std::flat_map` / `std::flat_set`)
- [ ] MSVC announces implementation timeline
- [ ] Performance profiling indicates cache locality issues
- [ ] MSVC implementation appears in cppreference compiler support table

### Stack Trace (`std::stacktrace`)
- [ ] Microsoft resolves symbol handling bugs in MSVC
- [ ] cppreference shows MSVC implementation as full (not partial)
- [ ] Community reports indicate production reliability

## References

- `.planning/phases/27-cpp23-advanced-features/27-RESEARCH.md` - Full research findings
- [cppreference C++23 Compiler Support](https://en.cppreference.com/w/cpp/compiler_support/23)
- [Microsoft Learn - MSVC C++23 Conformance](https://learn.microsoft.com/en-us/cpp/overview/msvc-conformance-improvements)

---

*Document created: 2026-02-18*
*Phase: 27-cpp23-advanced-features*
*Milestone: v1.3 Deep Modernization*
