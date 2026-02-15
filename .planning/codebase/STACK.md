# Technology Stack

**Analysis Date:** 2026-02-15

## Languages

**Primary:**
- C++14 - Windows system components, DLLs, static libraries
- C - Legacy API compatibility layers (cardmod.h)

**Secondary:**
- PowerShell 5.0+ - Install/uninstall scripts, service configuration
- NSIS Script - Installer configuration
- Batch - Build automation

**Scripting:**
- YAML - GitHub Actions CI/CD workflows

## Runtime

**Environment:**
- Windows Vista SP1+ (x64 only) - Minimum supported platform
- Windows 10/11 (64-bit) - Primary deployment targets

**Build Environment:**
- Visual Studio 2025 (v18) with Platform Toolset v143
- Windows 10 SDK (10.0.22621.0 or later)
- MSBuild - CLI build tool

**Package Manager:**
- vcpkg - Not present (project uses native Windows SDKs)
- NuGet - Not used

## Frameworks

**Core:**
- Microsoft Visual C++ (MSVC) v145 - C++ compiler and standard library
- Windows Platform SDK - Core Windows APIs

**Security/Crypto:**
- Cryptographic Provider Development Kit (CPDK) - Smart card and cryptography headers
- CryptoAPI - Certificate management (`C:/Users/user/Documents/EIDAuthentication/EIDCardLibrary/CertificateUtilities.cpp`)
- CNG (Cryptography Next Generation) - Modern crypto operations (`bcrypt.h`)

**Testing:**
- None - No automated test framework detected

**Build/Dev:**
- MSBuild - Solution build system via `EIDCredentialProvider.sln`
- devenv.com - Visual Studio CLI (used in `C:/Users/user/Documents/EIDAuthentication/build.bat`)

**Installer:**
- NSIS 3.x - Nullsoft Scriptable Install System for installer creation
  - Script: `C:/Users/user/Documents/EIDAuthentication/Installer/Installerx64.nsi`

## Key Dependencies

**Windows System Libraries:**
- `advapi32.dll` - LSA (Local Security Authority) integration, registry access
- `crypt32.dll` - Certificate operations, CryptoAPI
- `winscard.dll` - Smart card API (PC/SC interface)
- `secur32.dll` - SSPI (Security Support Provider Interface)
- `kernel32.dll` - Core Windows APIs
- `user32.dll` - GUI components
- `ole32.dll` - COM (for Credential Provider)

**Static Libraries:**
- `EIDCardLibrary` - Project static library (`C:/Users/user/Documents/EIDAuthentication/EIDCardLibrary/`)

**External Dependencies:**
- None (all dependencies are Windows SDK or Visual Studio built-ins)

## Configuration

**Environment:**
- Registry-based configuration (`HKLM\SOFTWARE\Policies\EIDAuthentication`)
- Group Policy support for enterprise deployment

**Build:**
- Solution file: `C:/Users/user/Documents/EIDAuthentication/EIDCredentialProvider.sln`
- Platform: x64 only (Win32 configurations exist but are deprecated)
- Configurations: Debug, Release
- Character set: Unicode (TCHAR)

**Runtime Configuration:**
- Windows Registry keys for behavior control
- ETW (Event Tracing for Windows) for logging

## Platform Requirements

**Development:**
- Visual Studio 2025 (or VS 2022 with v143 toolset)
- "Desktop development with C++" workload
- Windows SDK 10.0.22621.0+
- CPDK (Cryptographic Provider Development Kit)
- NSIS 3.x (for installer builds)

**Production:**
- Windows Vista SP1 or later (64-bit)
- Smart card reader driver
- Smart card with certificate
- Administrator privileges for installation
- Reboot required post-installation

---

*Stack analysis: 2026-02-15*
