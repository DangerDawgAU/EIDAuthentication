# Pitfalls Research

**Domain:** C++14 to C++23 Standard Upgrade (Windows/MSVC)
**Researched:** 2026-02-15
**Confidence:** HIGH

---

## Critical Pitfalls

Mistakes that cause rewrites, security vulnerabilities, or major deployment failures.

### Pitfall 1: Windows 7 Support Elimination

**What goes wrong:**
Visual Studio 2026 (MSVC Build Tools 14.50/v145) has **completely removed** the ability to target Windows 7, Windows 8, Windows Server 2008 R2, and Windows Server 2012. Projects requiring Windows 7 support will fail to build or deploy after upgrading to the latest toolset.

**Why it happens:**
Microsoft's official policy now states that "Build tools support for Windows 7, 8.0 and 8.1 ended in Visual Studio 2026." The minimum supported target operating systems for MSVC Build Tools 14.50 are Windows 10 or Windows Server 2016.

**Consequences:**
- Complete inability to deploy to Windows 7 systems
- Potential business continuity failures for organizations still on Windows 7
- Forced architecture decision: drop Windows 7 or stay on older MSVC

**How to avoid:**
- Evaluate Windows 7 requirement BEFORE starting upgrade
- If Windows 7 is required: stay on VS 2022 (v143) or VS 2019 (v142) toolset
- If Windows 7 can be dropped: document this as a breaking change
- Consider using older toolset versions (14.30-14.43) available in VS 2026 installer for compatibility

**Warning signs:**
- Project requirements specify "Windows 7+"
- Customer base includes Windows 7 systems
- WINVER macro set to 0x0601 (Windows 7) or lower

**Phase to address:** Pre-upgrade assessment (Phase 0)

**Sources:**
- [Microsoft Learn - Overview of Potential Upgrade Issues](https://learn.microsoft.com/en-us/cpp/porting/overview-of-potential-upgrade-issues-visual-cpp?view=msvc-170) - HIGH confidence
- [GitHub Issue - VS 2026 dropped Windows 7/8 support](https://github.com/zufuliu/notepad4/issues/829) - MEDIUM confidence

---

### Pitfall 2: std::string_view Dangling References in LSASS Code

**What goes wrong:**
`std::string_view` (C++17) is a non-owning view that can create dangling references when the underlying string is destroyed or reallocated. In security-critical LSASS code, this can lead to:
- Use-after-free vulnerabilities
- Heap-use-after-free bugs
- Stack-use-after-return bugs
- Credential data corruption or leakage

**Why it happens:**
`std::string_view` does not own its data. Common problematic patterns include:
- Returning `string_view` from functions that create temporaries
- Storing `string_view` in classes when the underlying data lifetime is unclear
- Using `string_view` with ternary expressions that create temporary strings
- Passing `string_view` across API boundaries where lifetime is unclear

**Consequences:**
- Silent memory corruption in authentication logic
- Potential credential exposure
- LSASS crashes requiring system restart
- Security vulnerabilities exploitable by attackers

**How to avoid:**
- NEVER store `string_view` in class members in LSASS code
- Use `std::string` for any data that crosses API boundaries
- Apply lifetime analysis tools before adopting `string_view`
- Review all `string_view` usage with security-focused code review
- Prefer `std::string` for credential data handling

**Warning signs:**
- Functions returning `std::string_view` from temporary operations
- Class members of type `std::string_view`
- `string_view` used in ternary expressions
- Passing `string_view` to functions that store or cache it

**Phase to address:** C++17 feature adoption (Phase 2)

**Sources:**
- [C++ Core Guidelines Issue #1038](https://github.com/isocpp/CppCoreGuidelines/issues/1038) - HIGH confidence
- [Medium - 70% of C++ Security Bugs from string_view Misuse](https://medium.com/@dikhyantkrishnadalai/70-of-c-security-bugs-stem-from-std-string-view-misuse-heres-how-to-prevent-them-4aca2ba9bb86) - MEDIUM confidence

---

### Pitfall 3: ABI Compatibility Breach at C-Style API Boundaries

**What goes wrong:**
EIDAuthentication uses C-style API boundaries for ABI compatibility with Windows. Mixing C++ standard library types across these boundaries with different MSVC versions or `/std:c++` flags can cause:
- Link failures (name decoration changes)
- Runtime crashes (type layout changes)
- Silent memory corruption (container ABI differences)

**Why it happens:**
While MSVC maintains binary compatibility between v140/v141/v142/v143/v145 toolsets, changing the `/std:c++` flag can break ABI for specific standard library types. The C++ ABI is NOT stable across standard versions - only the C ABI is.

**Consequences:**
- Authentication DLL fails to load in LSASS
- Credential data corruption across API boundaries
- Unpredictable behavior that evades testing

**How to avoid:**
- Maintain C-style interfaces (`extern "C"`) for all exported functions
- NEVER pass STL containers (`std::string`, `std::vector`, etc.) across API boundaries
- Use opaque pointers (`void*`) with C-style accessor functions
- Ensure all DLL components are rebuilt with identical compiler flags
- Avoid exceptions crossing DLL boundaries - use error codes

**Warning signs:**
- `std::string` or `std::vector` in function signatures crossing DLL boundaries
- Different `/std:c++` flags between DLL and consumer
- Linking against libraries built with different MSVC versions
- Exceptions being thrown across module boundaries

**Phase to address:** Build system configuration (Phase 1)

**Sources:**
- [Microsoft Learn - Library and Build Tools Dependencies](https://learn.microsoft.com/en-us/cpp/porting/overview-of-potential-upgrade-issues-visual-cpp?view=msvc-170) - HIGH confidence
- [Reddit - C++ Types Across DLL Boundaries](https://www.reddit.com/r/cpp/comments/1b1tcjx/how_often_do_people_use_c_types_across_dllso/) - MEDIUM confidence

---

### Pitfall 4: Removed C++17 Features (auto_ptr, register, trigraphs)

**What goes wrong:**
C++17 removed several deprecated features that may still exist in older codebases:
- `auto_ptr` (replaced by `unique_ptr` in C++11)
- `register` keyword (no effect since C++11)
- Trigraphs (e.g., `??=` for `#`)
- `++` operator for `bool`
- `bind1st`, `bind2nd`, `ptr_fun`

**Why it happens:**
Legacy code from pre-C++11 era may still use these features. The upgrade to C++17+ will cause compilation failures.

**Consequences:**
- Compilation errors blocking upgrade progress
- Need to refactor legacy code patterns
- Potential introduction of bugs during conversion

**How to avoid:**
- Audit codebase for `auto_ptr` usage before upgrade
- Replace `auto_ptr` with `std::unique_ptr`
- Remove all `register` keywords (they do nothing)
- Remove trigraph sequences
- Replace `bind1st`/`bind2nd` with lambdas

**MSVC-specific workaround:**
Define `_HAS_AUTO_PTR_ETC=1` as a preprocessor macro to temporarily restore removed features, but this is NOT recommended for production.

**Warning signs:**
- Code history shows pre-C++11 origins
- Usage of `std::auto_ptr` anywhere in codebase
- Any `register` keyword in variable declarations
- Trigraph sequences in string literals or comments

**Phase to address:** Pre-upgrade code audit (Phase 0)

**Sources:**
- [Microsoft DevBlogs - C++17 Feature Removals and Deprecations](https://devblogs.microsoft.com/cppblog/c17-feature-removals-and-deprecations/) - HIGH confidence
- [cppreference - C++17](https://en.cppreference.com/w/cpp/17.html) - HIGH confidence

---

### Pitfall 5: Stricter Compiler Conformance Breaking Previously Valid Code

**What goes wrong:**
MSVC's improved conformance to the C++ standard means code that compiled cleanly with older compiler versions may now fail. The compiler is correctly flagging errors that it previously ignored or explicitly allowed.

**Why it happens:**
MSVC has been progressively improving standard conformance. Examples include:
- Non-const arguments to const parameters (now errors)
- Narrowing conversions (C4838 warnings)
- Two-phase name lookup for templates
- Stricter `const` correctness enforcement

**Consequences:**
- Hundreds of compilation errors in previously working code
- Extended upgrade timeline due to unexpected fixes needed
- Risk of introducing bugs while fixing "correct" code

**How to avoid:**
- Expect and plan for conformance-related compilation errors
- Use `/permissive-` flag to catch non-conformant code early
- Run code analysis (`/analyze`) before upgrade to identify issues
- Fix issues incrementally - don't disable conformance checks
- Test thoroughly after fixing conformance errors

**Warning signs:**
- Code that relies on MSVC-specific non-standard behavior
- Template code with ambiguous name lookup
- Implicit conversions that "just work" in current version
- `/Za` (disable extensions) flag causes compilation errors

**Phase to address:** Compiler upgrade (Phase 1)

**Sources:**
- [Microsoft Learn - Overview of Potential Upgrade Issues](https://learn.microsoft.com/en-us/cpp/porting/overview-of-potential-upgrade-issues-visual-cpp?view=msvc-170) - HIGH confidence
- [Microsoft Learn - MSVC Conformance Improvements](https://learn.microsoft.com/en-us/cpp/overview/msvc-conformance-improvements?view=msvc-170) - HIGH confidence

---

### Pitfall 6: /GL (Whole Program Optimization) Incompatibility

**What goes wrong:**
Object files compiled with `/GL` (Whole Program Optimization) can ONLY be linked using the exact same build tools that produced them. Mixing `/GL` objects from different MSVC versions causes link failures.

**Why it happens:**
The internal data structures within `/GL` object files are not stable across major versions of the build tools. Newer linkers cannot understand older `/GL` formats.

**Consequences:**
- Link errors when mixing object files/libraries
- Forced full rebuild of all dependencies
- Potential hidden issues if `/GL` objects slip through

**How to avoid:**
- Rebuild ALL static libraries and object files when upgrading toolset
- Do NOT mix `/GL` compiled objects across MSVC versions
- Document `/GL` usage clearly in build documentation
- Consider disabling `/GL` during upgrade process, re-enable after

**Warning signs:**
- Use of `/GL` flag in project settings
- Static libraries from external sources
- Link errors referencing "incompatible object format"

**Phase to address:** Build system upgrade (Phase 1)

**Sources:**
- [Microsoft Learn - Build Tools Compatibility](https://learn.microsoft.com/en-us/cpp/porting/overview-of-potential-upgrade-issues-visual-cpp?view=msvc-170) - HIGH confidence

---

## Moderate Pitfalls

### Pitfall 7: std::span Lifetime Issues (C++20)

**What goes wrong:**
`std::span` (C++20) is a non-owning view similar to `string_view`. It provides no bounds checking by default (no `at()` method like `std::vector`) and can dangle if the underlying container is modified or destroyed.

**Prevention:**
- Treat `std::span` as a function parameter type, not a storage type
- Never store `std::span` in class members
- Ensure underlying data outlives all spans referencing it
- Use static analysis to detect lifetime issues

**Phase to address:** C++20 feature adoption (Phase 3)

---

### Pitfall 8: C++20 Modules Premature Adoption

**What goes wrong:**
C++20 modules have significant adoption challenges in existing codebases:
- IntelliSense issues in Visual Studio
- Circular import dependencies failing to compile
- Build system complexity (CMake support still evolving)
- Incompatible with traditional header-only libraries

**Prevention:**
- Defer modules adoption until tooling matures
- Start with header units for standard library headers only
- Test module adoption in isolation before production use
- Monitor [Visual Studio Developer Community](https://developercommunity.visualstudio.com/t/Currently-known-issues-with-IntelliSense/10738687) for known issues

**Phase to address:** Future phase (NOT recommended for initial upgrade)

**Sources:**
- [Visual Studio Developer Community - IntelliSense Issues](https://developercommunity.visualstudio.com/t/Currently-known-issues-with-IntelliSense/10738687) - HIGH confidence
- [Medium - Are C++ Modules There Yet?](https://medium.com/@nerudaj/are-c-modules-there-yet-77cde050afce) - MEDIUM confidence

---

### Pitfall 9: Silent Behavior Changes (Annex C)

**What goes wrong:**
The C++ standard documents breaking changes in **Annex C [diff]**. These can cause silent behavior changes - code compiles but behaves differently:
- Copy elision changes
- Move semantics interactions with existing code
- `auto` type deduction changes
- Order of evaluation changes

**Prevention:**
- Review cppreference's C++ standard version change logs
- Enable high warning levels (`/W4`)
- Run comprehensive test suite after each standard upgrade
- Use static analysis to detect potentially affected code

**Phase to address:** Each standard version upgrade

**Sources:**
- [Stack Overflow - Silent Behavior Changes](https://stackoverflow.com/questions/63290846/have-there-ever-been-silent-behavior-changes-in-c-with-new-standard-versions) - HIGH confidence
- [cppreference - Compiler Support](https://en.cppreference.com/w/cpp/compiler_support) - HIGH confidence

---

### Pitfall 10: Library Dependencies Rebuild Required

**What goes wrong:**
When upgrading MSVC, ALL static libraries must be rebuilt with the new toolset. This includes third-party libraries for which you may not have source code.

**Prevention:**
- Inventory all static library dependencies before upgrade
- Ensure source code access or vendor support for new toolset
- Check Vcpkg/Conan for updated binary packages
- Plan for potential vendor negotiations if source unavailable

**Phase to address:** Pre-upgrade assessment (Phase 0)

---

### Pitfall 11: /NODEFAULTLIB and CRT Refactoring

**What goes wrong:**
Projects using `/NODEFAULTLIB` (Ignore All Default Libraries) will fail because CRT library names changed in Visual Studio 2015+:
- `libcmt.lib` became `libcmt.lib`, `libucrt.lib`, `libvcruntime.lib`
- `msvcrt.lib` became `msvcrt.lib`, `ucrt.lib`, `vcruntime.lib`

**Prevention:**
- Update Additional Dependencies with new library names
- Or: remove `/NODEFAULTLIB` and let linker use defaults
- Add `legacy_stdio_definitions.lib` if linking older static libraries

**Phase to address:** Build system upgrade (Phase 1)

---

## Minor Pitfalls

### Pitfall 12: Deprecated but Not Removed Features

Features deprecated in newer standards that still compile but generate warnings:
- `std::iterator` (C++17) - use iterator traits directly
- `std::raw_storage_iterator` (C++17)
- `std::get_temporary_buffer` (C++17)
- Some `<codecvt>` facilities (C++17)

**Prevention:** Address deprecation warnings before they become removal errors.

---

### Pitfall 13: Unicode vs MBCS Warnings

Older MFC projects default to MBCS. Upgrading generates warnings to use Unicode.

**Prevention:** Either convert to Unicode or disable warning 4996 project-wide if MBCS is intentional.

---

### Pitfall 14: 32-bit to 64-bit Reveal Hidden Bugs

Compiling for 64-bit can reveal bugs hidden by 32-bit builds:
- Pointer truncation
- Size_t format specifier mismatches
- Time_t size differences

**Prevention:** Compile for 64-bit during upgrade testing to catch these early.

---

## Security-Specific Pitfalls for LSASS Code

| Pitfall | Risk | Prevention |
|---------|------|------------|
| `string_view` storing credential data | Use-after-free credential exposure | Use `std::string` for all credential data |
| STL types across API boundaries | Memory corruption, credential leakage | C-style interfaces only |
| Exception crossing DLL boundary | Uncaught exception in LSASS | Error codes only |
| Narrowing conversions | Silent data corruption | Treat C4838 as errors |
| Memory allocation mismatch | Heap corruption | Single allocator strategy |
| `/analyze` warnings ignored | Latent security defects | Fix all analysis warnings |

---

## Technical Debt Patterns

Shortcuts that seem reasonable but create long-term problems.

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Define `_HAS_AUTO_PTR_ETC=1` | Quick compile fix | Blocks future standard adoption | Never |
| Disable conformance checks | Faster upgrade | Accumulates non-standard code | Never |
| Skip static library rebuilds | Saves time | Runtime crashes | Never |
| Use `/std:c++latest` | Access newest features | Unexpected breaking changes | Only with pinned compiler version |
| Adopt modules early | Modern codebase | Tooling issues, build complexity | After ecosystem matures |

---

## "Looks Done But Isn't" Checklist

Things that appear complete after upgrade but are missing critical pieces.

- [ ] **Build succeeds:** But runtime crashes due to ABI mismatch - verify all DLLs load correctly
- [ ] **Tests pass:** But silent behavior changes affect edge cases - add regression tests
- [ ] **Compiler upgraded:** But `/std:c++` flag not changed - verify flag in all configurations
- [ ] **Flag updated:** But static libraries not rebuilt - verify all .lib files are new
- [ ] **Code compiles:** But IntelliSense broken (modules) - test IDE functionality
- [ ] **Local works:** But Windows 7 deployment fails - verify target OS support

---

## Recovery Strategies

When pitfalls occur despite prevention, how to recover.

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Windows 7 support lost | HIGH | Revert to older toolset, or drop Windows 7 requirement |
| ABI compatibility breach | HIGH | Rebuild all components with identical flags |
| `string_view` dangling bugs | MEDIUM | Replace with `std::string`, lifetime audit |
| Conformance errors | MEDIUM | Fix incrementally, can use `/permissive` temporarily |
| Removed features | LOW | Replace with modern equivalents |
| `/GL` incompatibility | LOW | Rebuild affected libraries |

---

## Pitfall-to-Phase Mapping

How roadmap phases should address these pitfalls.

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Windows 7 support elimination | Phase 0 (Assessment) | Verify target OS requirements documented |
| `string_view` dangling references | Phase 2 (C++17) | Code review, static analysis |
| ABI compatibility breach | Phase 1 (Build System) | ABI diff tools, load tests |
| Removed C++17 features | Phase 0 (Assessment) | Code audit, grep patterns |
| Stricter conformance | Phase 1 (Compiler) | `/permissive-` compilation succeeds |
| `/GL` incompatibility | Phase 1 (Build System) | Clean rebuild with no link errors |
| `std::span` lifetime | Phase 3 (C++20) | Static analysis, code review |
| Modules premature adoption | Defer | N/A |
| Silent behavior changes | All phases | Regression test suite |
| Library dependencies | Phase 0 (Assessment) | Dependency inventory complete |
| `/NODEFAULTLIB` issues | Phase 1 (Build System) | Link succeeds without errors |

---

## Sources

### Official Documentation (HIGH confidence)
- [Microsoft Learn - Overview of Potential Upgrade Issues](https://learn.microsoft.com/en-us/cpp/porting/overview-of-potential-upgrade-issues-visual-cpp?view=msvc-170)
- [Microsoft Learn - C++ Binary Compatibility](https://learn.microsoft.com/en-us/cpp/porting/binary-compat-2015-2017?view=msvc-170)
- [Microsoft Learn - MSVC Conformance Improvements](https://learn.microsoft.com/en-us/cpp/overview/msvc-conformance-improvements?view=msvc-170)
- [Microsoft DevBlogs - C++17 Feature Removals and Deprecations](https://devblogs.microsoft.com/cppblog/c17-feature-removals-and-deprecations/)
- [cppreference - C++17](https://en.cppreference.com/w/cpp/17.html)
- [cppreference - C++20](https://en.cppreference.com/w/cpp/20.html)
- [cppreference - C++23](https://en.cppreference.com/w/cpp/23.html)

### Community and Technical Resources (MEDIUM confidence)
- [C++ Core Guidelines Issue #1038 - string_view use-after-free](https://github.com/isocpp/CppCoreGuidelines/issues/1038)
- [Stack Overflow - Silent Behavior Changes](https://stackoverflow.com/questions/63290846/have-there-ever-been-silent-behavior-changes-in-c-with-new-standard-versions)
- [Stack Overflow - C++ Types Across DLL Boundaries](https://www.reddit.com/r/cpp/comments/1b1tcjx/how_often_do_people_use_c_types_across_dllso/)
- [GitHub Issue - VS 2026 Windows 7 Support Dropped](https://github.com/zufuliu/notepad4/issues/829)
- [Visual Studio Developer Community - IntelliSense Issues with Modules](https://developercommunity.visualstudio.com/t/Currently-known-issues-with-IntelliSense/10738687)

### Security-Specific Resources
- [Medium - string_view Security Bugs](https://medium.com/@dikhyantkrishnadalai/70-of-c-security-bugs-stem-from-std-string-view-misuse-heres-how-to-prevent-them-4aca2ba9bb86)
- [PVS-Studio - C++ Undefined Behavior Guide](https://pvs-studio.com/en/blog/posts/cpp/1149/)

---
*Pitfalls research for: C++14 to C++23 Standard Upgrade (Windows/MSVC)*
*Researched: 2026-02-15*
