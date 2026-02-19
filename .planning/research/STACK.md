# Stack Research: SonarQube Issue Remediation for C++23 Windows LSASS Codebase

**Domain:** C++23 SonarQube code quality remediation tooling
**Researched:** 2026-02-18
**Confidence:** HIGH (verified with official clang-tidy docs, Microsoft Learn, LLVM documentation)

## Executive Summary

Remediating ~730 SonarQube issues in a C++23 Windows LSASS codebase requires a **multi-layered tooling approach**:

1. **Clang-tidy** (via Visual Studio 2022 native integration) for automated fix suggestions
2. **MSVC Code Analysis** (`/analyze`) for C++ Core Guidelines compliance (already enabled)
3. **Manual refactoring** guided by IDE tooling for complex issues (cognitive complexity, nesting)

**CRITICAL CONSTRAINTS for LSASS context:**
- No dynamic memory allocation patterns that could trigger heap fragmentation
- No exceptions (use `std::expected<T, E>` pattern)
- No external library dependencies beyond Windows SDK
- Must compile with `/MT` (static CRT)

---

## Recommended Stack

### Primary Development Tools

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| Visual Studio 2022 | 17.13+ | IDE with native clang-tidy | Built-in clang-tidy integration, MSVC debugger, refactoring tools |
| Clang-tidy | 18.x (bundled) | Static analysis + auto-fixes | Direct mapping to SonarQube rules, many fixes can be auto-applied |
| MSVC Code Analysis | v143 (`/analyze`) | PREfast + C++ Core Guidelines | Already enabled in Release builds, complements clang-tidy |
| SonarLint | VS 2022 Extension | Real-time SonarQube feedback | IDE-integrated issue detection before CI scan |

### Supporting Libraries (Already in Project)

| Library | Purpose | Use Case |
|---------|---------|----------|
| `std::array` | Fixed-size containers | Replace C-style arrays (SonarQube: 28 issues) |
| `std::span` (C++20) | Non-owning view | Safe array access without pointer decay |
| `gsl::span` (if needed) | Pre-C++20 span | Backward-compatible bounds-safe access |

### Helper Scripts (Recommended)

| Script | Purpose | Implementation |
|--------|---------|----------------|
| `run-clang-tidy.py` | Batch clang-tidy execution | Bundled with LLVM, auto-fix with `-fix` flag |
| `clang-format` | Code style consistency | Run before/after refactoring |
| PowerShell wrapper | Filter LSASS-safe fixes | Custom script to exclude dangerous patterns |

---

## clang-tidy Configuration for SonarQube Issues

### Recommended `.clang-tidy` File

```yaml
---
Checks: >
  -*,
  bugprone-*,
  cppcoreguidelines-pro-type-*,
  modernize-loop-convert,
  modernize-use-auto,
  modernize-use-nullptr,
  modernize-use-override,
  modernize-use-using,
  modernize-macro-to-enum,
  modernize-use-default-member-init,
  readability-const-return-type,
  readability-non-const-parameter,
  readability-else-after-return,
  readability-braces-around-statements,
  readability-identifier-naming,
  readability-isolate-declaration,
  misc-const-correctness
WarningsAsErrors: ''
HeaderFilterRegex: '.*'
CheckOptions:
  - key: readability-identifier-naming.VariableCase
    value: lower_case
  - key: modernize-use-auto.MinTypeNameLength
    value: 5
  - key: modernize-loop-convert.MinConfidence
    value: reasonable
  - key: readability-function-cognitive-complexity.Threshold
    value: 25
```

### Check-to-SonarQube Issue Mapping

| clang-tidy Check | SonarQube Issue Type | Count | Auto-fixable |
|------------------|---------------------|-------|--------------|
| `modernize-use-auto` | "Replace redundant type with auto" | 126 | YES |
| `modernize-macro-to-enum` | "Replace macro with const/constexpr/enum" | 111 | YES |
| `misc-const-correctness` | "Global variables should be const" | 71 | PARTIAL |
| `readability-braces-around-statements` | "Fill compound statement" | 17 | YES |
| `readability-isolate-declaration` | "Define each identifier separately" | 50 | YES |
| `modernize-loop-convert` | Range-based for loops | N/A | YES |
| `cppcoreguidelines-pro-type-cstyle-cast` | C-style cast warnings | N/A | YES |
| `readability-else-after-return` | "Merge if with enclosing one" | 17 | YES |

---

## LSASS-Safe Remediation Patterns

### What IS Safe for LSASS

| Pattern | SonarQube Rule | Why Safe |
|---------|---------------|----------|
| `constexpr` constants | "Replace macro with const" | Compile-time, no runtime allocation |
| `enum class` | "Replace macro with enum" | Type-safe, no heap usage |
| `std::array<T, N>` | "Use std::array instead of C array" | Stack-allocated, bounds-checked |
| `const` globals | "Global variables should be const" | Read-only memory, no modification |
| Range-based for | Loop modernization | No iterator invalidation |
| `auto` for iterators | "Replace redundant type with auto" | No runtime impact |

### What is DANGEROUS for LSASS

| Pattern | Why Dangerous | Use Instead |
|---------|---------------|-------------|
| `std::string` (dynamic) | Heap allocation | `std::array<char, N>` or fixed buffers |
| `std::vector` | Heap allocation | `std::array` or pre-sized buffers |
| `std::make_unique` | Heap allocation | Stack allocation or static buffers |
| Exceptions | SEH issues in LSASS | Return codes, `std::expected<T, E>` |
| STL algorithms that allocate | Hidden heap usage | Manual loops with stack buffers |

### Windows API Interop (Do NOT Change)

| Pattern | SonarQube Flag | Why Keep |
|---------|---------------|----------|
| `LSA_UNICODE_STRING` | C-style struct | Windows API requirement |
| `SecBuffer` arrays | C-style array | SSPI API contract |
| `BYTE*` buffers | Pointer decay | CryptoAPI pattern |
| `__FUNCTION__` macro | "Replace macro" | ETW tracing requirement |

---

## Installation & Setup

### Visual Studio 2022 Configuration

```powershell
# Enable clang-tidy in VS 2022
# Tools > Options > Text Editor > C/C++ > Advanced
# Set "Enable Clang-Tidy" = True
# Set "Clang-Tidy Checks" to the checks list above

# Or via project properties:
# Configuration Properties > Code Analysis > Clang-Tidy
```

### Run clang-tidy from Command Line

```bash
# Using LLVM bundled with VS 2022
set PATH=%PATH%;C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin

# Single file with auto-fix
clang-tidy.exe -p .\x64\Release\ --fix EIDCardLibrary\StringConversion.cpp

# Batch processing with compilation database
# First generate compile_commands.json via CMake or VS
run-clang-tidy.py -p .\ -fix -config .\.clang-tidy
```

### PowerShell Helper Script for Safe Fixes

```powershell
# run-safe-clang-tidy.ps1
# Runs only LSASS-safe clang-tidy fixes

$safeChecks = @(
    "modernize-use-auto",
    "modernize-loop-convert",
    "modernize-use-nullptr",
    "readability-braces-around-statements",
    "readability-isolate-declaration"
)

$checkList = $safeChecks -join ","

Get-ChildItem -Recurse -Filter "*.cpp" | ForEach-Object {
    Write-Host "Processing: $($_.FullName)"
    clang-tidy.exe -p .\x64\Release\ --fix --checks="-$checkList" $_.FullName
}
```

---

## What NOT to Use

| Tool/Pattern | Why Avoid | Use Instead |
|--------------|-----------|-------------|
| clang-tidy `modernize-make-shared` | Heap allocation | Stack allocation |
| clang-tidy `modernize-make-unique` | Heap allocation | Stack allocation |
| clang-tidy `modernize-use-string-view` | May introduce `std::string` | Fixed char arrays |
| Auto-apply ALL fixes | Some may break LSASS constraints | Review each fix category |
| C++20 ranges | Complex template code | Traditional loops |
| External static analysis tools (CppDepend, PVS-Studio) | License cost, redundancy | clang-tidy + MSVC analysis |

---

## Batch Refactoring Workflow

### Phase 1: Automated Fixes (Est. 300 issues)

```bash
# 1. Create .clang-tidy file at solution root
# 2. Run clang-tidy with auto-fix on each project

# Auto-fix safe patterns:
# - modernize-use-auto (126 issues)
# - readability-isolate-declaration (50 issues)
# - readability-braces-around-statements (17 issues)
# - readability-else-after-return (17 issues)

clang-tidy -p compile_commands.json --fix --checks="-*,modernize-*,readability-*" <files>
```

### Phase 2: Semi-Automated Fixes (Est. 200 issues)

```bash
# Manual review required but pattern-based:
# - misc-const-correctness (71 globals + 31 pointers)
# - modernize-macro-to-enum (111 macros)

# Generate fix suggestions, review in IDE:
clang-tidy -p compile_commands.json --checks="-*,misc-const-correctness,modernize-macro-to-enum" <files>
```

### Phase 3: Manual Refactoring (Est. 150 issues)

```bash
# Complex issues requiring human judgment:
# - Cognitive complexity reduction (52 functions)
# - Nesting depth reduction
# - C-style array to std::array (28 issues)

# Use IDE refactoring tools + manual editing
# Each function reviewed for LSASS safety
```

---

## IDE Integration Details

### Visual Studio 2022 Code Analysis Settings

```
Project Properties > Configuration Properties > Code Analysis:
  - Enable Code Analysis: Yes
  - Enable PREfast: Yes (already enabled in Release)
  - Code Analysis Rule Set: Microsoft Native Recommended Rules

Project Properties > Configuration Properties > Code Analysis > Clang-Tidy:
  - Enable Clang-Tidy: Yes
  - Use Custom clang-tidy Checks: (paste check list)
  - Run clang-tidy as background analysis: Yes
```

### SonarLint Configuration

```xml
<!-- sonarlint.json in solution root -->
{
  "sonarlint.connectedMode.project": {
    "projectKey": "EIDAuthentication"
  },
  "rules": {
    "cpp:S119": "OFF",  /* Naming convention - project has own style */
    "cpp:S5958": "OFF"  /* std::string requirement - LSASS unsafe */
  }
}
```

---

## Version Compatibility

| Tool | Minimum | Recommended | Notes |
|------|---------|-------------|-------|
| Visual Studio 2022 | 17.13 | 17.14+ | clang-tidy 18.x bundled |
| LLVM/Clang | 18.0 | 18.x (bundled) | Use VS-bundled version for MSVC compatibility |
| SonarLint | 7.0 | Latest | VS 2022 extension |
| MSVC | 19.43 | 19.44+ | Already using v143 toolset |

---

## Sources

- **Clang-Tidy Checks List:** https://clang.llvm.org/extra/clang-tidy/checks/list.html (HIGH confidence)
- **Microsoft Learn - C++ Core Guidelines Checker:** https://learn.microsoft.com/en-us/cpp/code-quality/code-analysis-for-cpp-corecheck (HIGH confidence)
- **Clang-Tidy modernize-use-auto:** https://clang.llvm.org/extra/clang-tidy/checks/modernize/use-auto.html (HIGH confidence)
- **Clang-Tidy modernize-loop-convert:** https://clang.llvm.org/extra/clang-tidy/checks/modernize/loop-convert.html (HIGH confidence)
- **Clang-Tidy readability-function-cognitive-complexity:** https://clang.llvm.org/extra/clang-tidy/checks/readability/function-cognitive-complexity.html (HIGH confidence)
- **Project SonarQube Analysis:** `.planning/sonarqube-analysis.md` (INTERNAL - HIGH confidence)
- **Existing Stack Research:** `.planning/research/STACK.md` (INTERNAL - HIGH confidence)

---

## Summary

For SonarQube issue remediation in the EIDAuthentication C++23 Windows LSASS codebase:

1. **Primary Tool:** Visual Studio 2022 with native clang-tidy integration (no additional installation)
2. **Complementary:** MSVC Code Analysis (`/analyze`) already enabled - continues to catch issues
3. **Real-time Feedback:** SonarLint VS extension for immediate issue visibility
4. **Auto-fix Strategy:** Use clang-tidy with `--fix` for safe patterns only (auto, nullptr, braces, isolated declarations)
5. **Manual Review Required:** Const correctness, macro-to-enum, cognitive complexity, any pattern involving memory allocation
6. **CRITICAL:** Never auto-apply patterns that introduce `std::string`, `std::vector`, or heap allocation in LSASS code

**Estimated Fix Distribution:**
- Auto-fixable: ~300 issues (40%)
- Semi-automated (review required): ~200 issues (27%)
- Manual refactoring: ~150 issues (20%)
- Won't Fix (Windows API constraints): ~80 issues (11%)

**Confidence Assessment:** HIGH - All tool recommendations verified against official LLVM/Microsoft documentation and mapped to specific SonarQube issue categories from project analysis.
