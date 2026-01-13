# EID Authentication for Windows

**Smart card authentication for Windows standalone/local accounts**

[![License](https://img.shields.io/badge/license-GPL%20v3-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows%20Vista%2B-lightgrey.svg)]()
[![Build](https://img.shields.io/badge/build-Visual%20Studio%202025-purple.svg)]()

---

## Overview

EID Authentication enables **smart card-based login** for Windows local accounts (non-domain joined computers). Originally created by certified security experts, it integrates deeply with Windows security mechanisms while providing a user-friendly configuration interface.

### What It Does

- **Replace passwords with smart cards** for Windows local account login
- **LSA Authentication Package** for deep Windows security integration
- **Credential Provider v2** for modern Windows login screen integration
- **Password Change Notification** for synchronized credential management
- **Configuration Wizard** for easy smart card enrollment

### What's New (2026)

This fork has been **extensively modernized and cleaned up**:

- ✅ Windows XP/GINA support removed (Vista SP1+ only)
- ✅ Upgraded to Visual Studio 2025 (Platform Toolset v145)
- ✅ Migrated from WiX to NSIS installer
- ✅ Dead code cleanup (~2,000 lines removed)
- ✅ Automatic Smart Card service configuration
- ✅ Improved error messages and user guidance
- ✅ Build automation scripts
- ✅ Comprehensive documentation

---

## Quick Start

### Prerequisites

- **Windows Vista SP1 or later** (x64)
- **Visual Studio 2025** (Community, Professional, or Enterprise)
- **NSIS 3.x** (for building installer)
- **Cryptographic Provider Development Kit (CPDK)** - [Download](https://www.microsoft.com/en-us/download/details.aspx?id=30688)
- **Smart card reader** (physical or virtual TPM-based)

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

**Output:** `Installer\EIDInstallx64.exe` (699 KB)

### Installation

1. **Run installer as Administrator:** `Installer\EIDInstallx64.exe`
2. **Reboot** (required for LSA package to load)
3. **Run Configuration Wizard** from desktop shortcut
4. **Enroll your smart card** following the wizard

**Note:** For testing without a physical smart card reader, run `Installer\InstallVirtualSmartCard.ps1` as Administrator to create a TPM-based virtual smart card.

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
| **EIDTest** | C++ Test Suite | Component unit tests |

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

---

## Development

### Project Structure

```
EIDAuthentication/
├── EIDAuthenticationPackage/    # LSA authentication package
├── EIDCardLibrary/               # Shared smart card library
├── EIDConfigurationWizard/       # User configuration GUI
├── EIDConfigurationWizardElevated/  # Elevated policy helpers
├── EIDCredentialProvider/        # Credential Provider v2
├── EIDLogManager/                # Logging management tool
├── EIDPasswordChangeNotification/   # Password filter DLL
├── EIDTest/                      # Test suite
├── Installer/                    # NSIS installer scripts
├── build.bat                     # Build automation script
├── notes.md                      # Development notes and history
├── DEPLOYMENT.md                 # Deployment guide
└── README.md                     # This file
```

### Build Configuration

- **Platform:** x64 only
- **Configurations:** Debug, Release
- **Toolset:** v145 (Visual Studio 2025)
- **SDK:** Windows 10 SDK (10.0.26100.0+)
- **Language Standard:** C++14
- **Runtime:** Multi-threaded DLL (/MD)

### Building Individual Projects

```cmd
# Build specific project
"C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\devenv.com" ^
    EIDCredentialProvider.sln /Build "Release|x64" /Project EIDCardLibrary

# Build all
build.bat Release x64
```

### Testing

```cmd
# Run test suite (requires admin and smart card)
x64\Release\EIDTest.exe

# Enable diagnostic logging
x64\Release\EIDLogManager.exe
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
3. Check Event Viewer → Windows Logs → System for LSA errors

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

## Security Considerations

### LSA Protection (RunAsPPL)

- **Enabled:** Maximum security, requires signed DLLs
- **Disabled:** Testing only, allows unsigned DLLs

### Smart Card PIN Security

- PINs are **never stored** - they're used once to unlock the private key
- Private keys **never leave the smart card**
- All crypto operations happen on-card

### Credential Storage

- Encrypted with DPAPI (Data Protection API)
- Tied to user profile
- Removed on uninstall via `DllUnregister`

---

## Known Limitations

- **Local accounts only** - Not for domain-joined computers
- **x64 only** - No 32-bit support
- **Windows Vista SP1+** - No XP support
- **Single smart card per user** - Cannot enroll multiple cards
- **No offline unlock** - Requires smart card to log in

---

## Contributing

Contributions welcome! Areas for improvement:

- [ ] Fix compiler warnings (C4996, C4311, C4302, C4005)
- [ ] Translate French comments to English
- [ ] Add multi-card support
- [ ] Implement biometric authentication (removed in this fork)
- [ ] Support Windows Hello integration
- [ ] Improve error messages and logging
- [ ] Add automated tests

---

## Credits

- **Original Author:** Vincent Le Toux (2009)
- **Fork Maintainer:** @uberlinuxguy (GitHub)
- **Modernization:** @andrysky (2026 updates)
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
- **VSCode Setup:** [Microsoft Docs](https://code.visualstudio.com/docs/cpp/config-msvc)

---

## Changelog

### 2026-01-11 - Major Modernization

- ✅ Removed Windows XP/GINA support
- ✅ Upgraded to Visual Studio 2025 (v145 toolset)
- ✅ Migrated from WiX to NSIS installer
- ✅ Removed ~2,000 lines of dead code (Belgian EID SDK, WinBio, etc.)
- ✅ Added automatic Smart Card service configuration
- ✅ Improved error messages with user guidance
- ✅ Created build automation (`build.bat`)
- ✅ Added virtual smart card support script
- ✅ Comprehensive documentation overhaul

### 2009-2020 - Original Development

- Initial release by Vincent Le Toux
- LSA package and Credential Provider implementation
- GINA support for Windows XP
- WiX installer

---

**Made with ❤️ for Windows security enthusiasts**
