# EID Authentication - Deployment Guide

## Overview
EID Authentication is a Windows smart card authentication solution for standalone computers and local accounts on domain computers.

**Platform:** Windows Vista SP1+ (64-bit only)
**Installation:** Requires Administrator privileges and reboot
**Dependencies:** Smart card reader, compatible smart card with certificate

---

## Installation

### Prerequisites
1. Windows 64-bit (Vista SP1 or later)
2. Administrator account
3. Smart card reader installed
4. Smart card with valid certificate

### Installation Steps

#### 1. Run Installer
```
Right-click EIDInstallx64.exe → Run as Administrator
```

#### 2. Handle Windows Security Warning
**Expected Behavior:** Windows may block the DLL with "Program Compatibility Assistant"

**Message:**
```
This module is blocked from loading into the Local Security Authority.
\Device\HarddiskVolume3\Windows\System32\EIDAuthenticationPackage.dll
```

**Why This Happens:**
- The DLL is not digitally signed
- Windows protects LSA from unsigned code
- This is a custom-built authentication package

**Workaround Options:**

**Option A: Disable LSA Protection Temporarily (Recommended for Testing)**
```powershell
# Run as Administrator
reg add "HKLM\SYSTEM\CurrentControlSet\Control\Lsa" /v RunAsPPL /t REG_DWORD /d 0 /f
```
Then reboot and install.

**Option B: Code Signing (Recommended for Production)**
1. Obtain a code signing certificate
2. Sign all DLLs with `signtool.exe`:
   ```cmd
   signtool sign /f certificate.pfx /p password /t http://timestamp.digicert.com EIDAuthenticationPackage.dll
   signtool sign /f certificate.pfx /p password /t http://timestamp.digicert.com EIDCredentialProvider.dll
   signtool sign /f certificate.pfx /p password /t http://timestamp.digicert.com EIDPasswordChangeNotification.dll
   ```
3. Rebuild installer with signed DLLs

**Option C: Disable Driver Signature Enforcement (NOT Recommended - Test VMs Only)**
```
1. Restart Windows in Advanced Boot Options (Shift + Restart)
2. Troubleshoot → Advanced → Startup Settings → Restart
3. Press F7 for "Disable driver signature enforcement"
4. Install EIDInstallx64.exe
5. Reboot normally
```

#### 3. Complete Installation
1. Installer copies DLLs to `C:\Windows\System32\`
2. Registers LSA authentication package
3. Registers credential provider (COM)
4. Registers password change notification
5. Creates desktop shortcut
6. **Reboot is required**

#### 4. First Use
1. After reboot, insert smart card
2. Launch "EID Authentication Configuration" from desktop
3. Follow wizard to enroll certificate
4. Configure authentication options

---

## Uninstallation

### Complete Removal Steps

#### 1. Run Uninstaller
```
Control Panel → Programs → Uninstall a program → EID Authentication → Uninstall
```
OR
```
C:\Windows\System32\EIDUninstall.exe
```

#### 2. What Gets Removed

**Files Deleted:**
- `C:\Windows\System32\EIDAuthenticationPackage.dll`
- `C:\Windows\System32\EIDCredentialProvider.dll`
- `C:\Windows\System32\EIDPasswordChangeNotification.dll`
- `C:\Windows\System32\EIDConfigurationWizard.exe`
- `C:\Windows\System32\EIDUninstall.exe`
- Desktop shortcut

**Registry Keys Removed:**
- LSA Security Packages registration
- LSA Notification Packages registration
- Credential Provider CLSID registration
- Credential Provider Filter registration
- Configuration Wizard Control Panel entry
- WMI Autologger settings
- Crash dump configuration (lsass.exe)
- Smart card removal policies
- Uninstall entry

**User Data Removed:**
- All stored encrypted credentials (LSA secrets)
- Authentication settings
- Smart card enrollment data

**Note:** Some files may require reboot to delete (LSA locks DLLs)

#### 3. Reboot
Reboot required to complete uninstallation and unload DLLs from LSA.

---

## Troubleshooting

### "Authentication package not available" Error
**Cause:** LSA package not loaded or not registered

**Solutions:**
1. Verify installation completed successfully
2. Check if DLL exists: `C:\Windows\System32\EIDAuthenticationPackage.dll`
3. Verify registry entry:
   ```
   HKLM\SYSTEM\CurrentControlSet\Control\Lsa\Security Packages
   Should contain: EIDAuthenticationPackage
   ```
4. Ensure you rebooted after installation
5. Check Event Viewer → Windows Logs → System for LSA errors

### DLL Blocked by Windows
**Cause:** Unsigned DLL blocked by LSA protection

**Solution:** See "Option A" or "Option B" in Installation Steps above

### Smart Card Not Detected
**Cause:** Reader not installed or card not inserted

**Solutions:**
1. Verify reader appears in Device Manager
2. Insert smart card before launching wizard
3. Restart Smart Card service:
   ```cmd
   net stop scardsvr
   net start scardsvr
   ```

### Cannot Uninstall - Files Locked
**Cause:** LSA has DLLs loaded

**Solution:**
1. Run uninstaller (marks files for deletion on reboot)
2. Reboot Windows
3. Files will be deleted during boot

### Credential Provider Not Appearing
**Cause:** COM registration failed or credential provider not enabled

**Solutions:**
1. Verify registry:
   ```
   HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\Credential Providers\{B4866A0A-DB08-4835-A26F-414B46F3244C}
   ```
2. Re-register:
   ```cmd
   rundll32.exe EIDAuthenticationPackage.dll,DllRegister
   ```
3. Reboot

---

## Architecture

### Components

**LSA Authentication Package** (`EIDAuthenticationPackage.dll`)
- Loaded by lsass.exe at boot
- Handles smart card authentication
- Manages stored credentials
- Implements challenge-response protocol

**Credential Provider** (`EIDCredentialProvider.dll`)
- COM DLL loaded by LogonUI.exe
- Provides login/unlock UI
- Filters and presents credentials
- CLSID: `{B4866A0A-DB08-4835-A26F-414B46F3244C}`

**Password Change Notification** (`EIDPasswordChangeNotification.dll`)
- Loaded by lsass.exe
- Synchronizes stored credentials when password changes
- Updates encrypted password storage

**Configuration Wizard** (`EIDConfigurationWizard.exe`)
- User-mode configuration tool
- Smart card enrollment
- Certificate management
- Policy configuration

### Security Model

**Credential Storage:**
- Passwords encrypted using certificate-based encryption
- Stored in LSA secrets (same as Windows password cache)
- Certificate hash used as index
- DPAPI-like security model

**Authentication Flow:**
1. User presents smart card at login
2. Credential provider reads certificate
3. LSA package validates certificate chain
4. Challenge-response authentication:
   - Signature-based (asymmetric crypto), OR
   - Encryption-based (symmetric crypto)
5. Stored password decrypted and used for Windows logon

---

## Configuration

### Group Policy Settings

Configure via Local Group Policy Editor (`gpedit.msc`) or Active Directory GPO:

**Registry Location:** `HKLM\SOFTWARE\Policies\EIDAuthentication\`

**Available Policies:**

| Policy | Type | Description |
|--------|------|-------------|
| `AllowSignatureOnlyKeys` | DWORD | Accept signature-only certificates (cannot decrypt) |
| `AllowCertificatesWithNoEKU` | DWORD | Accept certificates without Smart Card Logon EKU |
| `AllowTimeInvalidCertificates` | DWORD | Accept expired certificates (NOT recommended) |
| `AllowIntegratedUnblock` | DWORD | Enable PIN unblock feature |
| `ReverseSubject` | DWORD | Display name formatting (reverse DN) |
| `X509HintsNeeded` | DWORD | Certificate selection hints |
| `FilterDuplicateCertificates` | DWORD | Hide duplicate certificates |
| `ForceReadingAllCertificates` | DWORD | Read all certs (slower) vs optimized |

**Example: Allow Expired Certificates**
```reg
Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Policies\EIDAuthentication]
"AllowTimeInvalidCertificates"=dword:00000001
```

### Logging

**Enable ETW Tracing:**
```cmd
reg import EnableLog.reg
```
(Located in `Installer\EnableLog.reg`)

**Disable ETW Tracing:**
```cmd
reg import DisableLog.reg
```

**View Logs:**
1. Launch `EIDLogManager.exe`
2. Or use Event Viewer:
   - Windows Logs → Application
   - Filter by Source: EIDCredentialProvider

**Trace Levels:**
- Critical (1)
- Error (2)
- Warning (3)
- Info (4)
- Verbose (5)

---

## Certificate Requirements

### Smart Card Certificate Must Have:

**Required Extensions:**
1. **Subject Alternative Name (SAN):**
   - User Principal Name (UPN), OR
   - RFC822 email address

2. **Enhanced Key Usage (EKU):**
   - Smart Card Logon (1.3.6.1.4.1.311.20.2.2)
   - Client Authentication (1.3.6.1.5.5.7.3.2) - optional

3. **Key Usage:**
   - Digital Signature (required)
   - Key Encipherment (optional - for encryption-based auth)

**Certificate Chain:**
- Must chain to trusted root CA
- Intermediate certificates must be available
- Root CA must be in Trusted Root Certification Authorities store

**Validity:**
- Certificate must be within valid date range (unless policy override)
- Not revoked (CRL/OCSP check if configured)

### Example Certificate Setup

**Issue certificate with:**
```
Subject: CN=John Doe,OU=Users,O=Company,C=US
SAN: UPN=john.doe@company.com
EKU: Smart Card Logon, Client Authentication
Key Usage: Digital Signature, Key Encipherment
```

---

## Development & Building

### Prerequisites
- Visual Studio 2025 (or VS 2022 with Platform Toolset v145)
- Windows SDK 10
- CPDK (Cryptographic Provider Development Kit)
- NSIS 3.10+ (for installer)

### Build Commands

**Build Solution:**
```cmd
msbuild EIDCredentialProvider.sln /p:Configuration=Release /p:Platform=x64 /t:rebuild
```

**Build Installer:**
```cmd
"C:\Program Files (x86)\NSIS\makensis.exe" Installer\Installerx64.nsi
```

**Output:**
- Binaries: `x64\Release\`
- Installer: `Installer\EIDInstallx64.exe`

### Code Signing (Production)

**Sign DLLs:**
```cmd
signtool sign /f certificate.pfx /p password /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 x64\Release\*.dll
signtool sign /f certificate.pfx /p password /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 x64\Release\*.exe
```

**Verify Signature:**
```cmd
signtool verify /pa x64\Release\EIDAuthenticationPackage.dll
```

---

## Support & Resources

### Documentation
- [notes.md](notes.md) - Refactoring notes and project analysis
- [README.md](README.md) - Project overview
- [How to compile.txt](How to compile.txt) - Build instructions (legacy)

### Source Code
- GitHub: https://github.com/andrysky/EIDAuthentication
- Original: https://sourceforge.net/projects/eidauthenticate/

### License
GNU Lesser General Public License (LGPL) v2.1

### Known Limitations
- Windows 64-bit only (no x86 support)
- Vista SP1 minimum (XP removed)
- Requires reboot for installation/uninstallation
- Not compatible with LSA protection (RunAsPPL) without code signing
- Smart card required (no certificate file fallback)

---

## Security Considerations

### Production Deployment Checklist
- [ ] Code sign all DLLs and EXEs
- [ ] Test installation on clean VM
- [ ] Test uninstallation completely removes all artifacts
- [ ] Verify certificate validation (chain, EKU, expiration)
- [ ] Configure appropriate GPO policies
- [ ] Test authentication with various smart cards
- [ ] Enable ETW logging for troubleshooting
- [ ] Document certificate issuance requirements
- [ ] Train users on enrollment process
- [ ] Plan for certificate renewal

### Security Best Practices
1. **Use code signing** - Essential for LSA integration
2. **Validate certificates** - Don't disable EKU or expiration checks
3. **Audit stored credentials** - Review what's stored in LSA secrets
4. **Monitor authentication** - Enable logging and review regularly
5. **Test in VM first** - Don't deploy to production without testing
6. **Plan rollback** - Have uninstaller tested and ready
7. **Document everything** - Certificate requirements, policies, procedures

---

*Last Updated: 2026-01-11*
*Version: 1.0 (Post-XP-Removal)*
*Platform: Windows Vista+ (64-bit)*
