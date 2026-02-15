# External Integrations

**Analysis Date:** 2026-02-15

## APIs & External Services

**Windows Security APIs:**
- Local Security Authority (LSA) - Authentication package integration
  - Interface: `ntsecapi.h`, `ntsecpkg.h` (from CPDK)
  - Exports: `LsaApInitializePackage`, `LsaApLogonUserEx2`, `LsaApCallPackage`
  - Location: `C:/Users/user/Documents/EIDAuthentication/EIDAuthenticationPackage/`

- Credential Provider Framework - Login screen integration
  - Interface: `credentialprovider.h`, `ICredentialProvider`, `ICredentialProviderCredential`
  - COM Registration: `{B4866A0A-DB08-4835-A26F-414B46F3244C}`
  - Location: `C:/Users/user/Documents/EIDAuthentication/EIDCredentialProvider/`

- SSPI (Security Support Provider Interface) - Security context management
  - Interface: `sspi.h`
  - Library: `secur32.lib`
  - Usage: Authentication token management

## Data Storage

**Databases:**
- None (no external database)

**Credential Storage:**
- Windows LSA Secrets - Encrypted credential storage
  - API: `LsaStorePrivateData`, `LsaRetrievePrivateData`
  - Purpose: Store encrypted passwords keyed by certificate hash
  - Location: `C:/Users/user/Documents/EIDAuthentication/EIDCardLibrary/StoredCredentialManagement.cpp`

**File Storage:**
- Local filesystem only - No cloud storage
  - Configuration: Registry
  - Logs: Windows Event Log (ETW)
  - Binaries: `C:\Windows\System32\` (system integration)

**Certificate Storage:**
- Windows Certificate Store - System certificate management
  - APIs: `CertOpenStore`, `CertEnumCertificatesInStore`, `CertAddCertificateContextToStore`
  - Stores: MY, Root, CA
  - Usage: Certificate validation and enrollment

## Authentication & Identity

**Auth Provider:**
- Custom - Smart card-based authentication for local accounts
  - Implementation: Challenge-response protocol with on-card private keys
  - No external identity provider integration
  - No domain/Active Directory support (local accounts only)

**Smart Card Integration:**
- PC/SC (Personal Computer/Smart Card) interface
  - Headers: `winscard.h`, `cardmod.h`
  - Library: `winscard.lib`
  - Service: `SCardSvr` (Smart Card Resource Manager)
  - Device enumeration: `ScDeviceEnum` service

**Certificate Validation:**
- Windows CryptoAPI certificate chain validation
  - API: `CertVerifyCertificateChainPolicy`
  - EKU Requirement: Smart Card Logon (1.3.6.1.4.1.311.20.2.2)
  - Location: `C:/Users/user/Documents/EIDAuthentication/EIDCardLibrary/CertificateValidation.cpp`

## Monitoring & Observability

**Error Tracking:**
- None (no external error reporting)

**Logs:**
- ETW (Event Tracing for Windows) - Native Windows logging
  - Provider GUID: Not specified (uses WMI Autologger registration)
  - Levels: Critical (1), Error (2), Warning (3), Info (4), Verbose (5)
  - Tool: `C:/Users/user/Documents/EIDAuthentication/EIDLogManager/`
  - Registry: `HKLM\SYSTEM\CurrentControlSet\Control\WMI\Autologger\EIDAuthentication`

## CI/CD & Deployment

**Hosting:**
- GitHub - Source code repository
  - Actions: Automated build and security scanning
  - Workflows: `C:/Users/user/Documents/EIDAuthentication/.github/workflows/`

**CI Pipeline:**
- GitHub Actions
  - Build: `C:/Users/user/Documents/EIDAuthentication/.github/workflows/windows-build.yaml`
  - Security Scan: `C:/Users/user/Documents/EIDAuthentication/.github/workflows/codeql.yml`
  - Platform: `windows-latest` runner
  - Triggers: Push to main, pull requests, scheduled (weekly)

**Deployment:**
- NSIS Installer - Windows installer package
  - Script: `C:/Users/user/Documents/EIDAuthentication/Installer/Installerx64.nsi`
  - Output: `EIDInstallx64.exe`
  - Location: `C:/Users/user/Documents/EIDAuthentication/Installer/`
  - Size: ~870 KB

## Environment Configuration

**Required env vars:**
- None (configuration via registry only)

**Secrets location:**
- LSA Secrets (Windows internal protected storage)
- DPAPI-protected user profile data

## Webhooks & Callbacks

**Incoming:**
- None (no web server or API endpoints)

**Outgoing:**
- None (no external API calls)
- All telemetry/database features removed (see `C:/Users/user/Documents/EIDAuthentication/notes.md`)

## Registry Integration

**Installation Keys:**
- `HKLM\Software\EIDAuthentication\InstallPath` - Installation directory
- `HKLM\SYSTEM\CurrentControlSet\Control\Lsa\Authentication Packages` - LSA package registration
- `HKLM\SYSTEM\CurrentControlSet\Control\Lsa\Notification Packages` - Password filter registration
- `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\Credential Providers\{B4866A0A-DB08-4835-A26F-414B46F3244C}` - Credential Provider registration
- `HKCR\CLSID\{B4866A0A-DB08-4835-A26F-414B46F3244C}` - COM registration

**Group Policy Keys:**
- `HKLM\SOFTWARE\Policies\Microsoft\Windows\SmartCardCredentialProvider\` - Smart card policies
- `HKLM\Software\Microsoft\Windows\CurrentVersion\Policies\System\scforceoption` - Force smart card logon
- `HKLM\Software\Microsoft\Windows\CurrentVersion\Winlogon\scremoveoption` - Card removal behavior

---

*Integration audit: 2026-02-15*
