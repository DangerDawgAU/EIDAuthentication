# EIDAuthentication C++23 Modernization

## What This Is

A project to upgrade the EIDAuthentication Windows smart card authentication codebase from C++14 to C++23, enabling modern language features while maintaining backward compatibility with Windows 7+.

This security-critical Windows authentication package provides smart card login for local accounts on non-domain joined computers, integrating with LSA (Local Security Authority) and the Credential Provider framework.

## Core Value

Successfully compile the entire codebase with `/std:c++23` and leverage modern C++23 features to improve code quality, safety, and maintainability without breaking existing functionality.

## Requirements

### Validated

(Existing codebase capabilities - already working)
- ✓ Smart card login for Windows local accounts
- ✓ LSA Authentication Package integration
- ✓ Credential Provider v2 for Windows login screen
- ✓ Password Change Notification for credential sync
- ✓ Configuration Wizard for smart card enrollment
- ✓ x64 build support
- ✓ Security hardening (143 issues remediated)

### Active

- [ ] All 7 Visual Studio projects updated with `/std:c++23` flag
- [ ] Code compiles cleanly with C++23 (no errors)
- [ ] Leverage `constexpr`/`consteval` for compile-time operations where beneficial
- [ ] Adopt `std::expected` for error handling where appropriate
- [ ] Use `std::format` / `std::print` for text handling improvements
- [ ] Update README.md to reflect C++23 requirement
- [ ] All existing functionality preserved (no regressions)

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
| Target C++23 | Latest standard with mature compiler support in VS 2025 | — Pending |
| Drop Vista support | Vista EOL, reduces testing burden, enables some C++23 library features | — Pending |
| Preserve C-style APIs | LSA/Credential Provider interfaces require C-compatible exports | — Pending |
| Incremental modernization | Fix compile errors first, refactor to use new features incrementally | — Pending |

---
*Last updated: 2026-02-15 after initialization*
