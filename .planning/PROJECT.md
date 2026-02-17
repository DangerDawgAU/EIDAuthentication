# EIDAuthentication C++23 Modernization

## What This Is

A Windows smart card authentication package providing smart card login for local accounts on non-domain joined computers, integrating with LSA (Local Security Authority) and the Credential Provider framework. The codebase has been modernized from C++14 to C++23.

## Core Value

A clean, maintainable, and secure codebase with zero static analysis issues, leveraging modern C++23 features while preserving all existing authentication functionality.

## Current Milestone: v1.1 SonarQube Quality Remediation

**Goal:** Close all 1,167 SonarQube issues through code improvement or explicit "not applicable" marking

**Target features:**
- Zero open SonarQube security hotspots
- Zero open SonarQube bugs/reliability issues
- Modern C++ patterns (const correctness, smart pointers, std::string)
- Reduced code complexity and duplication
- Iterative verification with re-scanning after each phase

## Requirements

### Validated (v1.0 Shipped)
- ✓ Build System — All 7 projects with `/std:c++23`
- ✓ Error Handling — `std::expected` internal, C API boundaries
- ✓ Compile-Time — `constexpr` validation, `std::to_underlying`
- ✓ Code Quality — `std::format`, `std::span`
- ✓ Documentation — README, BUILD.md updated
- ✓ Build Verification — Passed, runtime pending

### Active (v1.1 Scope)
- [ ] Resolve 2 security hotspots (strlen safety)
- [ ] Fix 3 reliability bugs (type punning, dead code)
- [ ] Const correctness for globals, pointers, functions, parameters
- [ ] Modern C++ types (std::string, std::array, enum class, nullptr)
- [ ] Code simplification (auto, constexpr, merge ifs, separate decls)
- [ ] Complexity reduction (nesting, RAII for memory)
- [ ] Modern diagnostics (std::source_location)
- [ ] Resolve 17 code duplication blocks
- [ ] Final verification: 0 open issues

### Out of Scope

- Windows Vista support — minimum is now Windows 7+
- x86 (32-bit) builds — x64 only
- Domain-joined computer support — local accounts only
- New features beyond C++23 modernization
- Code signing (separate concern)

## Context

**Codebase History:**
- Originally developed 2009 by Vincent Le Toux
- Extensively modernized in 2026 (143 security fixes, ~2000 lines dead code removed)
- Currently on C++14 with no explicit `/std:` flag (compiler default)

**Technical Environment:**
- Visual Studio 2025 (v143/v145 toolset)
- Windows 10 SDK (10.0.26100.0+)
- NSIS installer
- CPDK (Cryptographic Provider Development Kit)

**Security Context:**
- LSA package runs in LSASS process (security-critical)
- Must handle credentials securely (`SecureZeroMemory`, DPAPI encryption)
- Challenge-response protocol for smart card auth
- Private keys never leave smart card

## Constraints

- **Compatibility:** Windows 7+ support required (drop Vista)
- **Architecture:** x64 only
- **Toolset:** VS 2025 (v143/v145)
- **Security:** All existing security hardening must be preserved
- **Functionality:** No regressions in authentication flow
- **Build:** Must compile with MSVC (no Clang/GCC)

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Target C++23 | Latest standard with mature compiler support in VS 2025 | ✓ Good |
| Drop Vista support | Vista EOL, reduces testing burden, enables some C++23 library features | ✓ Good |
| Preserve C-style APIs | LSA/Credential Provider interfaces require C-compatible exports | ✓ Good |
| Incremental modernization | Fix compile errors first, refactor to use new features incrementally | ✓ Good |
| SonarQube remediation phase-by-phase | Group issues by category, fix iteratively with re-scan verification | — Pending |

---
*Last updated: 2026-02-17 after v1.1 milestone start*
