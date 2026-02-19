# EIDAuthentication C++23 Modernization

## What This Is

A Windows smart card authentication package providing smart card login for local accounts on non-domain joined computers, integrating with LSA (Local Security Authority) and the Credential Provider framework. The codebase has been modernized from C++14 to C++23.

## Core Value

A clean, maintainable, and secure codebase with zero static analysis issues, leveraging modern C++23 features while preserving all existing authentication functionality.

## Current Milestone: v1.4 SonarQube Zero

**Goal:** Eliminate all fixable SonarQube issues and document remaining won't-fix with justifications

**Target features:**
- Fix ~380 fixable SonarQube issues (P2 and P3 priorities)
- Review ~351 "needs review" issues to determine fixability
- Document ~280 won't-fix issues in SonarQube with proper justifications
- Achieve minimal remaining SonarQube debt

### Completed Milestones

**v1.0 C++23 Modernization** — Build system, error handling, compile-time features ✓
**v1.1 SonarQube Quality Remediation** — Zero security hotspots, zero reliability bugs ✓
**v1.2 Code Modernization** — ~55 SonarQube issues fixed, ~1,000 documented as Won't Fix ✓
**v1.3 Deep Modernization** — Style, macros, const, nesting, complexity, diagnostics ✓

## Requirements

### Validated (v1.0 & v1.1 Shipped)
- ✓ Build System — All 7 projects with `/std:c++23`
- ✓ Error Handling — `std::expected` internal, C API boundaries
- ✓ Compile-Time — `constexpr` validation, `std::to_underlying`
- ✓ Code Quality — `std::format`, `std::span`
- ✓ Documentation — README, BUILD.md updated
- ✓ Build Verification — Passed, runtime pending
- ✓ Security Hotspots — 0 open (2 fixed)
- ✓ Reliability Bugs — 0 open (3 fixed)

### Active (v1.4 Scope)
- [ ] Global variable const correctness (review ~60 issues)
- [ ] Cognitive complexity reduction (~30 issues)
- [ ] Deep nesting reduction (~25 issues)
- [ ] Macro to constexpr conversion (~35 issues)
- [ ] Rule of Five/Three violations (~15 issues)
- [ ] C-style cast review (~8 issues)
- [ ] Ellipsis notation review (~3 issues)
- [ ] Replace redundant types with auto (~40 issues)
- [ ] Use init-statements in if/switch (~80 issues)
- [ ] Convert to std::array where safe (~30 issues)
- [ ] Function const correctness (~15 issues)
- [ ] Initialization list usage (~10 issues)
- [ ] Remove redundant casts (~10 issues)
- [ ] Unused parameter handling (~8 issues)
- [ ] Convert to enum class (~8 issues)
- [ ] Range-based for loops (~8 issues)
- [ ] Document won't-fix issues in SonarQube (~280 issues)

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
| SonarQube remediation phase-by-phase | Group issues by category, fix iteratively with re-scan verification | ✓ Good |
| LSASS memory safety | std::string/std::vector avoided in LSASS context | ✓ Good |
| Windows API const compatibility | Non-const pointers required by many Windows APIs | ✓ Good |

---
*Last updated: 2026-02-18 after v1.4 milestone start*
