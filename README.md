# EID Authentication for Windows

**Smart card authentication for Windows standalone/local accounts**

[![License](https://img.shields.io/badge/license-GPL%20v3-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows%207+-lightgrey.svg)]()
[![Build](https://img.shields.io/badge/build-VS%202022%20(v143)-purple.svg)]()

---

## Overview

EID Authentication provides **smart card-based login** for Windows local accounts on non-domain joined computers. This solution integrates with Windows security mechanisms including LSA (Local Security Authority) and the Credential Provider framework, enabling organizations and individuals to replace password-based authentication with smart card authentication.

### Key Features

- **Smart card login** for Windows local accounts
- **LSA Authentication Package** for Windows security integration
- **Credential Provider v2** for Windows login screen support
- **Password Change Notification** for credential synchronization
- **Configuration Wizard** for smart card enrollment

### 2026 Updates

This fork has been **extensively modernized**:

- Windows XP/GINA support removed (now Vista SP1+ only)
- Upgraded to Visual Studio 2025 (Platform Toolset v145)
- Migrated from WiX to NSIS installer
- Dead code cleanup (~2,000 lines removed)
- Automatic Smart Card service configuration
- Improved error messages and user guidance
- Build automation scripts
- Comprehensive documentation
- Security hardening with 143 issues remediated

### 2026 C++23 Modernization

The codebase has been upgraded to C++23 with the following improvements:

- **Language Standard:** C++23 with `/std:c++23preview` flag
- **Toolset:** v143 (for Windows 7+ compatibility)
- **Error Handling:** `std::expected<T, E>` for internal error handling
- **Compile-Time:** Enhanced `constexpr` validation and `std::to_underlying()` for enums
- **Code Quality:** `std::format` for text formatting, `std::span` for buffer handling
- **All 7 projects** configured for C++23 compilation
- **Zero regressions** in authentication functionality

See [.planning/ROADMAP.md](.planning/ROADMAP.md) for complete modernization details.

---

## Quick Start

### Prerequisites

- **Windows 7 or later** (x64)
- **Visual Studio 2022 or later** (Community, Professional, or Enterprise)
- **Platform Toolset v143** (required for Windows 7+ compatibility)
- **NSIS 3.x** (for building installer)
- **Cryptographic Provider Development Kit (CPDK)** - [Download](https://www.microsoft.com/en-us/download/details.aspx?id=30688)
- **Smart card reader** (physical or virtual TPM-based)

### Dependencies

**Windows APIs:**
- LSA (Local Security Authority) - `ntsecapi.h`, `ntsecpkg.h`
- CryptoAPI/CNG - `wincrypt.h`, `Crypt32.lib`
- Smart Card API - `winscard.h`, `Winscard.lib`
- SSPI - `sspi.h`, `Secur32.lib`
- DPAPI - Data Protection API for credential encryption

**Runtime Requirements:**
- Smart Card Resource Manager service (`SCardSvr`)
- Smart Card Device Enumeration service (`ScDeviceEnum`)
- LSASS (Local Security Authority Subsystem Service)

### Building

```cmd
# Clone the repository
git clone https://github.com/yourusername/EIDAuthentication.git
cd EIDAuthentication

# Build the solution
build.bat

# Build installer
cd Installer
"C:\Program Files (x86)\NSIS\makensis.exe" Installerx64.nsi
```

**Output:** `Installer\EIDInstallx64.exe`

### Installation

1. **Run installer as Administrator:** `Installer\EIDInstallx64.exe`
2. **Reboot** (required for LSA package to load)
3. **Run Configuration Wizard** from desktop shortcut
4. **Enroll your smart card** following the wizard

For testing without a physical smart card reader, run `Installer\InstallVirtualSmartCard.ps1` as Administrator to create a TPM-based virtual smart card.

---

## Architecture

### Components

| Component | Type | Description |
|-----------|------|-------------|
| **EIDAuthenticationPackage** | LSA Package DLL | Core authentication logic, integrates with LSASS |
| **EIDCredentialProvider** | Credential Provider v2 DLL | Login screen UI and credential gathering |
| **EIDPasswordChangeNotification** | Password Filter DLL | Synchronizes password changes with smart card |
| **EIDConfigurationWizard** | C++ GUI Application | User-facing tool for smart card enrollment |
| **EIDConfigurationWizardElevated** | C++ Elevated Helper | UAC elevation helper for admin tasks |
| **EIDLogManager** | C++ Diagnostic Tool | Enable/disable event tracing |
| **EIDCardLibrary** | C++ Static Library | Shared smart card and crypto utilities |

### Installation Layout

```
C:\Program Files\EID Authentication\     # Main installation
├── EIDAuthenticationPackage.dll
├── EIDCredentialProvider.dll
├── EIDPasswordChangeNotification.dll
├── EIDConfigurationWizard.exe
└── EIDUninstall.exe

C:\Windows\System32\                     # System integration
├── EIDAuthenticationPackage.dll         # Copied here for LSA
├── EIDCredentialProvider.dll            # Copied here for Winlogon
└── EIDPasswordChangeNotification.dll    # Copied here for password filter
```

### Authentication Flow

The system uses a challenge-response protocol for smart card authentication:

1. **Credential Capture** - CEIDProvider detects smart card insertion and prompts for PIN
2. **Certificate Extraction** - Certificate read from smart card key container
3. **Challenge Generation** - LSA package generates random challenge data
4. **Response Verification** - Smart card signs/encrypts challenge with private key
5. **Credential Decryption** - Stored password decrypted using smart card's key material
6. **Windows Logon** - Decrypted credentials passed to Windows for authentication

**Challenge Types:**
- **Signature-based** - RSA signature on random data (most common)
- **Encryption-based** - RSA decryption of encrypted challenge
- **DPAPI-based** - Fallback for AT_SIGNATURE keys without encryption support

### LSA Package Interface

**Exported Functions** (EIDAuthenticationPackage.dll):

| Function | Purpose |
|----------|---------|
| `LsaApInitializePackage` | Package initialization at system boot |
| `LsaApLogonUserEx2` | Primary authentication entry point |
| `LsaApCallPackage` | Trusted IPC for credential management |
| `LsaApCallPackageUntrusted` | Untrusted credential operations |
| `LsaApLogonTerminated` | Session cleanup on logoff |
| `DllRegister` / `DllUnRegister` | Installation and removal |

**IPC Message Types** (for LsaApCallPackage):
- `EIDCMCreateStoredCredential` - Enroll new credential
- `EIDCMRemoveStoredCredential` - Remove enrolled certificate
- `EIDCMHasStoredCredential` - Check if certificate exists
- `EIDCMGetStoredCredentialRid` - Find user from certificate hash

### Credential Provider Interface

**COM Registration:**
- **CLSID:** `{B4866A0A-DB08-4835-A26F-414B46F3244C}`
- **Implements:** `ICredentialProvider`, `ICredentialProviderCredential`

**Key Classes:**

| Class | Purpose |
|-------|---------|
| `CEIDProvider` | Main credential provider interface |
| `CEIDCredential` | Individual smart card credential |
| `CEIDFilter` | Credential filtering logic |
| `CMessageCredential` | Status/error message display |

**Usage Scenarios:** Login, Unlock, Change Password, Credential UI

---

## Development

### Project Structure

```
EIDAuthentication/
├── EIDAuthenticationPackage/    # LSA authentication package
├── EIDCardLibrary/              # Shared smart card library
├── EIDConfigurationWizard/      # User configuration GUI
├── EIDConfigurationWizardElevated/  # Elevated policy helpers
├── EIDCredentialProvider/       # Credential Provider v2
├── EIDLogManager/               # Logging management tool
├── EIDPasswordChangeNotification/   # Password filter DLL
├── Installer/                   # NSIS installer scripts
├── build.bat                    # Build automation script
├── notes.md                     # Development notes and history
├── DEPLOYMENT.md                # Deployment guide
└── README.md                    # This file
```

### Build Configuration

- **Platform:** x64 only
- **Configurations:** Debug, Release
- **Toolset:** v143 (VS 2022 Build Tools, for Windows 7+ compatibility)
- **SDK:** Windows 10 SDK (10.0.26100.0+)
- **Language Standard:** C++23 (/std:c++23preview)
- **Runtime:** Multi-threaded Static (/MT for Release, /MTd for Debug)

### Building Individual Projects

```cmd
# Build specific project
"C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\devenv.com" ^
    EIDCredentialProvider.sln /Build "Release|x64" /Project EIDCardLibrary

# Build all
build.bat Release x64
```

---

## Deployment

### Test Environment Setup

For testing in a VM without production code signing:

1. **Disable LSA Protection** (allows unsigned DLLs):
   ```reg
   Windows Registry Editor Version 5.00

   [HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Lsa]
   "RunAsPPL"=dword:00000000
   ```

2. **Install and reboot**

3. **Configure smart card via wizard**

### Production Deployment

**CRITICAL:** Production deployments require **code signing** all DLLs.

1. **Sign binaries** with a trusted certificate:
   ```cmd
   signtool sign /f cert.pfx /p password /t http://timestamp.digicert.com ^
       EIDAuthenticationPackage.dll ^
       EIDCredentialProvider.dll ^
       EIDPasswordChangeNotification.dll
   ```

2. **Enable LSA Protection** for security:
   ```reg
   [HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Lsa]
   "RunAsPPL"=dword:00000001
   ```

3. **Deploy via Group Policy** or installer

See [DEPLOYMENT.md](DEPLOYMENT.md) for complete deployment instructions.

---

## Troubleshooting

### Smart Card Service Not Running

**Error:** `0x80100001D - The Smart Card Resource Manager is not running`

**Cause:** No smart card reader installed.

**Solution:**
- Connect a physical smart card reader, OR
- Run `Installer\InstallVirtualSmartCard.ps1` (requires TPM)

### DLL Blocked from Loading into LSA

**Error:** Windows Program Compatibility Assistant blocks DLL

**Cause:** DLL not digitally signed or LSA protection enabled.

**Solution (Test):** Disable LSA protection (see above)
**Solution (Production):** Code sign the DLLs

### Configuration Wizard Shows "Package Not Available"

**Cause:** LSA package not registered or LSASS hasn't loaded it yet.

**Solution:**
1. Verify registration: Check `HKLM\SYSTEM\CurrentControlSet\Control\Lsa\Authentication Packages`
2. Reboot if just installed
3. Check Event Viewer -> Windows Logs -> System for LSA errors

### Build Fails with "Platform Toolset Not Found"

**Cause:** Wrong Visual Studio version.

**Solution:** Install Visual Studio 2025 or edit `build.bat` to use your VS path

---

## Registry Keys

### Installation

- `HKLM\Software\EIDAuthentication\InstallPath` - Installation directory

### LSA Integration

- `HKLM\SYSTEM\CurrentControlSet\Control\Lsa\Authentication Packages` - Contains `EIDAuthenticationPackage`
- `HKLM\SYSTEM\CurrentControlSet\Control\Lsa\Notification Packages` - Contains `EIDPasswordChangeNotification`

### Credential Provider

- `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\Credential Providers\{B4866A0A-DB08-4835-A26F-414B46F3244C}`
- `HKCR\CLSID\{B4866A0A-DB08-4835-A26F-414B46F3244C}`

### Configuration Wizard

- `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\ControlPanel\NameSpace\{F5D846B4-14B0-11DE-B23C-27A355D89593}`
- `HKCR\CLSID\{F5D846B4-14B0-11DE-B23C-27A355D89593}`

---

## Group Policy Configuration

Policy settings are stored in: `HKLM\SOFTWARE\Policies\Microsoft\Windows\SmartCardCredentialProvider`

| Policy | Type | Description |
|--------|------|-------------|
| `AllowSignatureOnlyKeys` | DWORD | Accept certificates with signature-only keys |
| `AllowCertificatesWithNoEKU` | DWORD | Accept certificates without Smart Card Logon EKU |
| `AllowTimeInvalidCertificates` | DWORD | Accept expired certificates (NOT recommended) |
| `AllowIntegratedUnblock` | DWORD | Enable PIN unblock functionality |
| `FilterDuplicateCertificates` | DWORD | Hide duplicate certificates from selection |
| `ForceReadingAllCertificates` | DWORD | Read all certs vs. optimized reading |
| `ReverseSubject` | DWORD | Reverse certificate subject display order |
| `X509HintsNeeded` | DWORD | Certificate selection hints |
| `EnforceCSPWhitelist` | DWORD | Block non-whitelisted CSP providers (security feature) |

**Additional Policy Locations:**
- `HKLM\Software\Microsoft\Windows\CurrentVersion\Policies\System\scforceoption` - Force smart card logon
- `HKLM\Software\Microsoft\Windows NT\CurrentVersion\Winlogon\scremoveoption` - Smart card removal behavior (0=no action, 1=lock, 2=logoff, 3=disconnect)

---

## Security Considerations

### Security Hardening (2026)

This fork underwent comprehensive security assessment with **143 issues** identified and remediated:
- 27 CRITICAL severity (all fixed)
- 38 HIGH severity (all fixed)
- 62 MEDIUM severity (all fixed)
- 16 LOW severity (all fixed)

### Key Security Features

**Authentication Security:**
- Private keys never leave smart card (all crypto operations on-card)
- PINs never stored - used once per logon then securely erased
- Challenge-response protocol prevents credential replay attacks
- Smart card enforces PIN retry limits (typically 3-5 attempts)

**Credential Protection:**
- DPAPI encryption for stored credentials (tied to user profile)
- Certificate hash-based indexing (SHA-256)
- `SecureZeroMemory()` on all sensitive data after use
- Removed on uninstall via `DllUnregister`

**Code Security:**
- CSP provider whitelist (deny-by-default via `EnforceCSPWhitelist` policy)
- TOCTOU prevention via `ImpersonateClient()` during authorization checks
- Integer overflow validation on all buffer operations
- Input validation at all system boundaries
- Safe string functions (`StringCchPrintfW`, `memcpy_s`)

### LSA Protection (RunAsPPL)

- **Enabled (Production):** Maximum security, requires code-signed DLLs
- **Disabled (Testing):** Allows unsigned DLLs for development

### Smart Card PIN Security

- PINs are **never stored** - used once to unlock private key
- Private keys **never leave the smart card**
- All crypto operations happen on-card
- `SecureZeroMemory()` erases PIN from memory immediately after use

---

## Known Limitations

- **Local accounts only** - Not for domain-joined computers
- **x64 only** - No 32-bit support
- **Windows 7+** - No Vista/XP support
- **Single smart card per user** - Cannot enroll multiple cards
- **No offline unlock** - Requires smart card to log in

---

## Contributing

Contributions welcome! Areas for improvement:

- [ ] Fix compiler warnings (C4996, C4311, C4302, C4005)
- [ ] Translate French comments to English
- [ ] Add multi-card support
- [ ] Implement biometric authentication
- [ ] Support Windows Hello integration
- [ ] Improve error messages and logging
- [ ] Add automated tests

---

## Credits

- **Original Author:** Vincent Le Toux (2009)
- **Fork Maintainer:** Jason Williams (@uberlinuxguy)
- **2026 Modernization:** DangerDawgAU (77 commits)
- **Contributors:** Andry, chantzish
- **Source:** [SourceForge](https://sourceforge.net/projects/eidauthenticate/)

---

## License

GNU General Public License v3.0 (GPL v3)

See [LICENSE](LICENSE) for full text.

---

## Resources

- **Deployment Guide:** [DEPLOYMENT.md](DEPLOYMENT.md)
- **Development Notes:** [notes.md](notes.md)
- **Issue Tracker:** [GitHub Issues](https://github.com/uberlinuxguy/EIDAuthentication/issues)
- **Discussions:** [GitHub Discussions](https://github.com/uberlinuxguy/EIDAuthentication/discussions)
- **CPDK Download:** [Microsoft](https://www.microsoft.com/en-us/download/details.aspx?id=30688)

---

## Changelog

### 2026 - Major Modernization (DangerDawgAU)

- Removed Windows XP/GINA support
- Upgraded to Visual Studio 2025 (v145 toolset)
- Migrated from WiX to NSIS installer
- Removed ~2,000 lines of dead code (Belgian EID SDK, WinBio, etc.)
- Added automatic Smart Card service configuration
- Improved error messages with user guidance
- Created build automation (`build.bat`)
- Added virtual smart card support script
- Comprehensive documentation overhaul
- Security hardening: 143 vulnerabilities fixed

### 2025 - Fork Revival (Jason Williams)

- Forked from SourceForge to GitHub
- Initial modernization efforts
- Build system updates

### 2023 - Updates (Andry)

- Minor fixes and updates

### 2019 - Contribution (chantzish)

- Initial GitHub contribution

### 2009 - Original Development (Vincent Le Toux)

- Initial release
- LSA package and Credential Provider implementation
- GINA support for Windows XP
- WiX installer
