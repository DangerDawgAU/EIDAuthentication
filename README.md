# EID Authentication for Windows

**LSA-based smart card authentication for Windows standalone/local accounts.**

Compatible with Aventura MyEID 4.5 cards using the Aventura Minidriver.

---

## Concept of Operations

EID Authentication provides standalone Windows logon using smart card certificates instead of passwords. The system integrates with Windows LSA (Local Security Authority) to intercept authentication at the kernel security level, enabling smart card-based login for local (non-domain) accounts.

**Authentication Flow:**
1. User inserts smart card at Windows logon screen
2. Credential Provider displays PIN entry field
3. LSA Authentication Package validates PIN against smart card
4. Certificate extracted from card and verified against trust chain
5. User logged on with token generated from certificate-mapped local account

**Password Backup (Optional):** During enrollment, the Windows password can be encrypted with the smart card's public key and stored in LSA private data. This enables passwordless operation while maintaining recovery capability.

---

## Components

| Executable | Purpose |
|------------|---------|
| **EIDConfigurationWizard.exe** | Enrollment wizard for certificate creation, validation, and credential storage |
| **EIDConfigurationWizardElevated.exe** | UAC elevation helper for policy operations requiring admin rights |
| **EIDLogManager.exe** | ETW trace control and crash dump configuration for diagnostics |

| DLL | Purpose |
|-----|---------|
| **EIDAuthenticationPackage.dll** | LSA Authentication Package - core authentication logic running in LSASS |
| **EIDCredentialProvider.dll** | Credential Provider v2 - integrates with Windows logon screen |
| **EIDPasswordChangeNotification.dll** | Password Filter - synchronizes Windows password changes with stored credentials |

| Library | Purpose |
|---------|---------|
| **EIDCardLibrary.lib** | Static library providing smart card I/O, certificate management, and LSA IPC |

---

## Software Architecture

### Authentication Package (EIDAuthenticationPackage.dll)

Implements the Windows LSA Authentication Package interface (`SpLsaModeInitialize`).

**Key Functions:**
- `LsaApLogonUserEx2` - Primary authentication entry point
- `LsaApCallPackage` - Trusted IPC (challenge-response protocol)
- `LsaApCallPackageUntrusted` - Untrusted IPC (credential CRUD operations)

**IPC Message Types:**
| Message | Purpose |
|---------|---------|
| `EIDCMCreateStoredCredential` | Store encrypted password backup |
| `EIDCMUpdateStoredCredential` | Update stored password after Windows password change |
| `EIDCMRemoveStoredCredential` | Remove stored credential |
| `EIDCMHasStoredCredential` | Check if credential exists |
| `EIDCMGetStoredCredentialRid` | Lookup user RID from certificate hash |
| `EIDCMEIDGinaAuthenticationChallenge` | Initiate challenge-response |
| `EIDCMEIDGinaAuthenticationResponse` | Submit challenge response |

**LSA Private Data Keys:** Format `L$_EID_<RID_HEX>` (e.g., `L$_EID_000003E9` for RID 1001)

### Credential Provider (EIDCredentialProvider.dll)

Implements `ICredentialProvider` and `ICredentialProviderCredential` interfaces.

**COM Registration:**
- CLSID: `{B4866A0A-DB08-4835-A26F-414B46F3244C}`
- Usage Scenarios: `CPUS_LOGON`, `CPUS_UNLOCK_WORKSTATION`, `CPUS_CREDUI`

**Classes:**
| Class | Interface | Purpose |
|-------|-----------|---------|
| `CEIDProvider` | `ICredentialProvider` | Main provider, enumerates credentials |
| `CEIDCredential` | `ICredentialProviderCredential` | Smart card credential tile with PIN entry |
| `CMessageCredential` | `ICredentialProviderCredential` | Status message when no card present |
| `CEIDFilter` | `ICredentialProviderFilter` | Filters credential providers |

### Smart Card Library (EIDCardLibrary)

**Core API - SmartCardModule.h:**
```cpp
MgScCardAcquireContext()     // Connect to card via minidriver
MgScCardAuthenticatePin()    // Verify PIN
MgScCardReadFile()           // Read card file
MgScCardWriteFile()          // Write card file
MgScCardDeauthenticate()     // End authenticated session
MgScCardDeleteContext()      // Cleanup
```

**Core API - StoredCredentialManagement.h:**
```cpp
CStoredCredentialManager::CreateCredential()    // Store encrypted password
CStoredCredentialManager::GetPassword()         // Retrieve via certificate+PIN
CStoredCredentialManager::GetChallenge()        // Generate auth challenge
CStoredCredentialManager::GetPasswordFromChallengeResponse()  // Decrypt via challenge
CStoredCredentialManager::UpdateCredential()    // Re-encrypt after password change
CStoredCredentialManager::RemoveStoredCredential()
```

**Core API - CContainer.h:**
```cpp
CContainer::GetUserName()       // Extract username from certificate
CContainer::GetRid()            // Extract Relative ID from certificate
CContainer::GetCertificate()    // Return certificate context
CContainer::GetCSPInfo()        // Build CSP info for authentication
```

**Data Structures - EIDCardLibrary.h:**
```cpp
struct EID_INTERACTIVE_LOGON {
    EID_INTERACTIVE_LOGON_SUBMIT_TYPE MessageType;  // KerbCertificateLogon = 13
    UNICODE_STRING LogonDomainName;
    UNICODE_STRING UserName;
    UNICODE_STRING Pin;
    ULONG Flags;
    ULONG CspDataLength;
    PUCHAR CspData;           // Smart card CSP data
};

struct EID_SMARTCARD_CSP_INFO {
    DWORD dwCspInfoLen;
    DWORD MessageType;
    DWORD flags;
    DWORD KeySpec;
    ULONG nCardNameOffset;
    ULONG nReaderNameOffset;
    ULONG nContainerNameOffset;
    ULONG nCSPNameOffset;
    TCHAR bBuffer[...];
};
```

### Password Change Notification (EIDPasswordChangeNotification.dll)

Windows Password Filter API implementation:
- `InitializeChangeNotify()` - DLL initialization
- `PasswordFilter()` - Pre-change validation (accepts all)
- `PasswordChangeNotify()` - Post-change handler - calls `CStoredCredentialManager::UpdateCredential()`

---

## Registry Integration

### LSA
- `HKLM\SYSTEM\CurrentControlSet\Control\Lsa\Authentication Packages` = `EIDAuthenticationPackage`
- `HKLM\SYSTEM\CurrentControlSet\Control\Lsa\Notification Packages` = `EIDPasswordChangeNotification`

### Credential Provider
- `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\Credential Providers\{B4866A0A-DB08-4835-A26F-414B46F3244C}`
- `HKCR\CLSID\{B4866A0A-DB08-4835-A26F-414B46F3244C}`

### Configuration Wizard
- `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\ControlPanel\NameSpace\{F5D846B4-14B0-11DE-B23C-27A355D89593}`

### Group Policy
`HKLM\SOFTWARE\Policies\Microsoft\Windows\SmartCardCredentialProvider`

| Policy | Type | Purpose |
|--------|------|---------|
| `AllowSignatureOnlyKeys` | DWORD | Accept signature-only keys |
| `AllowCertificatesWithNoEKU` | DWORD | Accept certificates without Smart Card Logon EKU |
| `AllowTimeInvalidCertificates` | DWORD | Accept expired certificates |
| `EnforceCSPWhitelist` | DWORD | Block non-whitelisted CSP providers |

---

## Smart Card Compatibility

**Supported:** Aventura MyEID 4.5 cards via Aventura Minidriver

**Minidriver Integration:**
- Uses `SCardGetCardTypeProviderName()` to locate minidriver DLL
- Dynamically loads via `CardAcquireContext` (Card Module API)
- All crypto operations performed on-card (private keys never leave card)

---

## Security Properties

| Property | Implementation |
|----------|----------------|
| PIN Storage | Never stored - used once per auth, erased via `SecureZeroMemory()` |
| Private Keys | Never leave smart card - all crypto operations on-card |
| Credential Storage | Encrypted with certificate public key, stored in LSA private data |
| Certificate Validation | Full chain validation with CRL checking |
| Hash Algorithm | SHA-256 (32 bytes) |
| Encryption | Certificate-based or DPAPI fallback |
| Replay Protection | Challenge-response protocol |
| CSP Injection Protection | Whitelist enforcement via `EnforceCSPWhitelist` policy |

---

## System Requirements

- **OS:** Windows 7 SP1+ (x64 only)
- **Build:** Visual Studio 2022, Platform Toolset v143, C++23
- **Runtime:** Smart Card Resource Manager (SCardSvr), Smart Card Device Enumeration (ScDeviceEnum)

---

## Building

```cmd
build.bat
cd Installer
"C:\Program Files (x86)\NSIS\makensis.exe" Installerx64.nsi
```

Output: `Installer\EIDInstallx64.exe`

---

## License

GNU General Public License v3.0 - See [LICENSE](LICENSE)
