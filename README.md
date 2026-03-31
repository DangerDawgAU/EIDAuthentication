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

**Password Backup (Optional):** During enrollment, the Windows password can be encrypted with the smart card's public key and stored in LSA private data. This enables passwordless operation while maintaining DPAPI compatibility.

---

## Security Configuration

### Blank Password Accounts

**Warning:** By default, Windows allows local accounts with blank passwords to log on at the physical console without credentials. This bypasses smart card authentication.

**Default Windows Behavior:**

| Logon Type | Blank Password Account |
|------------|------------------------|
| Physical console | ⚠️ Allowed - can log in without credentials |
| Remote Desktop | Blocked |
| Network access (SMB) | Blocked |

**Required Group Policy:**

To enforce smart-card-only logon with passwordless accounts:

```
Computer Configuration > Windows Settings > Security Settings >
Local Policies > Security Options
```

**Policy:** `Accounts: Limit local account use of blank passwords to console logon only`

**Setting:** `Disabled` (blocks ALL blank password logons)

**Registry:**
```
HKLM\SYSTEM\CurrentControlSet\Control\Lsa\limitblankpassworduse = 0
```

### Deployment Strategies

**Option 1: Smart-Card-Only (Recommended)**

1. Disable blank password logon via Group Policy (above)
2. Remove Windows passwords: `net user <username> ""`
3. Enroll with blank password in Configuration Wizard
4. Result: Smart card required for all logons

**Option 2: Smart Card + Password Backup**

1. Keep strong Windows passwords on accounts
2. Enter real password during enrollment
3. Result: Smart card required; password enables DPAPI/network auth

### Password Validation Behavior

| Phase | Password Validation |
|-------|-------------------|
| Enrollment | YES - wizard validates against Windows password |
| Authentication | NO - token created from certificate only |
| Post-Logon | Stored password used for DPAPI, network auth |

---

## Components

| Executable | Purpose |
|------------|---------|
| **EIDConfigurationWizard.exe** | Enrollment wizard for certificate creation, validation, and credential storage |
| **EIDConfigurationWizardElevated.exe** | UAC elevation helper for policy operations requiring admin rights |
| **EIDLogManager.exe** | ETW trace control and crash dump configuration for diagnostics |
| **EIDMigrate.exe** | Command-line tool for bulk credential export and import |
| **EIDMigrateUI.exe** | GUI wizard for credential backup and migration |

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

| Policy | Type | Default | Purpose |
|--------|------|---------|---------|
| `AllowSignatureOnlyKeys` | DWORD | 0 | Accept signature-only keys |
| `AllowCertificatesWithNoEKU` | DWORD | 0 | Accept certificates without Smart Card Logon EKU |
| `AllowTimeInvalidCertificates` | DWORD | 0 | Accept expired certificates |
| `EnforceCSPWhitelist` | DWORD | 0 | Block non-whitelisted CSP providers |

**Note:** `0` = Disabled, `1` = Enabled. These policies are **disabled by default** for security. Only enable if specifically required for your environment.

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

## Bulk User Import/Export

The EIDMigrate tools enable bulk backup and migration of smart card credentials between machines. Credentials are exported to an encrypted `.eid` file format and can be restored to the same or a different machine.

### Tools

| Tool | Purpose |
|------|---------|
| **EIDMigrate.exe** | Command-line interface for export/import operations |
| **EIDMigrateUI.exe** | GUI wizard for step-by-step migration |

### Command-Line Usage

```cmd
# Export all local credentials to encrypted file
EIDMigrate.exe export -local -output backup.eid -password "passphrase-16-chars-min" -v

# Import credentials from file
EIDMigrate.exe import -local -input backup.eid -password "passphrase-16-chars-min" -v

# List all stored credentials
EIDMigrate.exe list -local -v

# Validate exported file
EIDMigrate.exe validate -input backup.eid -password "passphrase-16-chars-min" -v
```

### GUI Wizard

The EIDMigrateUI wizard provides four migration flows:

1. **Export** - Select credentials, confirm, and export to encrypted file
2. **Import** - Select file, preview credentials, and import to local machine
3. **List** - View all stored credentials (local machine or from file)
4. **Validate** - Verify file integrity and credential count

### File Format

Export files use the `.eid` extension with the following security:

- **Encryption:** AES-256-GCM (PBKDF2-HMAC-SHA256 key derivation)
- **Password:** Minimum 16 characters required
- **Integrity:** HMAC-SHA256 signature
- **Content:** Credentials, groups, and metadata in JSON format

### Important Notes

- **Password Requirement:** Export/import passwords must be at least 16 characters
- **Certificate Installation:** Imported certificates are automatically installed to the user's MY store
- **LSA Secret Format:** Uses double underscore format: `L$_EID__<HEX_RID>`
- **Cross-Machine:** Exported files can be imported to different machines

---

## Code Signing

By default, Windows enforces code signing requirements for LSA Authentication Packages and Credential Providers. To load unsigned DLLs during development or testing, you must disable this protection.

### Disable Code Signing Enforcement

**Registry Key:**
```
HKLM\SYSTEM\CurrentControlSet\Control\Lsa\RestrictBlankPasswordUse
```

**Setting:** `0` (Disabled)

Alternatively, for development builds, use:
```
HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System\DisableLsaSigning
```

**Setting:** `1` (Allow unsigned LSA packages)

### Production Deployment

For production deployments, all binaries must be code-signed:
- `EIDAuthenticationPackage.dll` (LSA Authentication Package)
- `EIDCredentialProvider.dll` (Credential Provider)
- `EIDPasswordChangeNotification.dll` (Password Filter)

Code signing ensures:
- OS verifies binary integrity at load time
- Users can verify software publisher
- Tamper detection via digital signatures

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
