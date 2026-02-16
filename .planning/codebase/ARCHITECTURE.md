# Architecture

**Analysis Date:** 2026-02-15

## Pattern Overview

**Overall:** Windows Authentication Subsystem Integration (LSA-based Multi-DLL Architecture)

**Key Characteristics:**
- Layered authentication architecture using Windows LSA (Local Security Authority)
- Challenge-response protocol for smart card-based authentication
- COM-based Credential Provider framework integration for UI
- Singleton pattern for shared credential management
- Callback-driven smart card event notification system

## Layers

**Authentication Package Layer (LSA Integration):**
- Purpose: Core authentication logic that integrates with Windows security subsystem
- Location: `EIDAuthenticationPackage/`
- Contains: LSA protocol handlers, credential verification, logon session management
- Depends on: EIDCardLibrary (shared utilities), Windows LSA APIs (`ntsecapi.h`, `ntsecpkg.h`)
- Used by: LSASS (Local Security Authority Subsystem Service)

**Smart Card Library Layer (Shared Utilities):**
- Purpose: Common smart card operations, certificate handling, credential storage
- Location: `EIDCardLibrary/`
- Contains: Certificate validation, stored credential management, smart card I/O, crypto operations, tracing utilities
- Depends on: Windows CryptoAPI (`wincrypt.h`), Smart Card API (`winscard.h`), DPAPI
- Used by: All other components (authentication package, credential provider, configuration wizard)

**Credential Provider Layer (UI Integration):**
- Purpose: Windows login screen integration, credential collection from users
- Location: `EIDCredentialProvider/`
- Contains: COM credential provider implementation, smart card connection monitoring, PIN input handling
- Depends on: EIDCardLibrary, Credential Provider framework (`credentialprovider.h`)
- Used by: Winlogon.exe

**Configuration Layer (User Management):**
- Purpose: Smart card enrollment, user account management, policy configuration
- Location: `EIDConfigurationWizard/`, `EIDConfigurationWizardElevated/`
- Contains: Multi-page wizard for certificate enrollment, policy management helpers (UAC-elevated)
- Depends on: EIDCardLibrary, Windows shell APIs
- Used by: End users (interactive), system administrators (via elevated helper)

**Password Change Notification Layer:**
- Purpose: Synchronize password changes with smart card credentials
- Location: `EIDPasswordChangeNotification/`
- Contains: Password filter DLL for Windows password change events
- Depends on: EIDCardLibrary
- Used by: LSASS (on password change events)

**Diagnostics Layer:**
- Purpose: Event tracing and log management
- Location: `EIDLogManager/`
- Contains: ETW (Event Tracing for Windows) control utility
- Depends on: Windows tracing APIs
- Used by: Administrators for troubleshooting

## Data Flow

**Smart Card Logon Flow:**

1. **Credential Provider Initialization** - `CEIDProvider::SetUsageScenario()` called by Winlogon
2. **Smart Card Detection** - `CSmartCardConnectionNotifier` monitors card insertion/removal via `ISmartCardConnectionNotifierRef` callback
3. **Credential Enumeration** - `CEIDProvider::GetCredentialCount()` returns credentials based on available cards
4. **User Interaction** - `CEIDCredential` collects PIN from user via `GetSerialization()`
5. **Certificate Extraction** - Certificate retrieved from smart card key container via `GetCertificateFromCspInfo()`
6. **Challenge Generation** - LSA package generates random challenge data (for signature-based auth)
7. **Challenge Signing** - Smart card signs challenge with private key (on-card crypto operation)
8. **LSA Authentication** - `LsaApLogonUserEx2()` in `EIDAuthenticationPackage.cpp` validates signature, decrypts stored password
9. **Token Creation** - `UserNameToToken()` creates Windows access token
10. **Session Creation** - `CreateLogonSession()` establishes user session

**Credential Enrollment Flow:**

1. **Wizard Launch** - User runs `EIDConfigurationWizard.exe`
2. **Card/Certificate Selection** - `CContainerHolderFactory` enumerates available certificates
3. **User Mapping** - Certificate mapped to local user account via RID
4. **Password Encryption** - DPAPI encrypts user password using card-derived key material
5. **Credential Storage** - `CStoredCredentialManager::CreateCredential()` stores encrypted credential
6. **LSA Communication** - `LsaApCallPackageUntrusted()` with `EIDCMCreateStoredCredential` message

**State Management:**
- Credentials stored encrypted per-user in LSA-managed storage
- Certificate hash (SHA-256) used as primary key for credential lookup
- PIN never stored - used only to unlock smart card private key for single operation

## Key Abstractions

**CContainer (Smart Card Abstraction):**
- Purpose: Represents a single smart card with its cryptographic container
- Examples: `EIDCardLibrary/CContainer.h`, `EIDCardLibrary/CContainer.cpp`
- Pattern: Encapsulates reader name, card name, provider, container, and certificate context; provides CSP info allocation

**CStoredCredentialManager (Credential Storage Singleton):**
- Purpose: Manages encrypted credential storage and retrieval
- Examples: `EIDCardLibrary/StoredCredentialManagement.h`
- Pattern: Singleton with instance access via `Instance()`; handles challenge generation/response validation

**CEIDProvider (COM Credential Provider):**
- Purpose: Main entry point for Windows credential provider framework
- Examples: `EIDCredentialProvider/CEIDProvider.h`
- Pattern: COM object implementing `ICredentialProvider`; aggregates `CEIDCredential` objects per detected card

**CContainerHolderFactory (Template Factory):**
- Purpose: Creates and manages container objects for different contexts
- Examples: `EIDCardLibrary/CContainerHolderFactory.h`, `EIDConfigurationWizard/CContainerHolder.h`
- Pattern: Template factory managing `CContainer` lifecycle with thread-safe reference counting

**CSmartCardConnectionNotifier (Event Monitor):**
- Purpose: Monitors smart card reader events and notifies registered callbacks
- Examples: `EIDCardLibrary/CSmartCardNotifier.h`
- Pattern: Callback-based observer pattern using `ISmartCardConnectionNotifierRef` interface

## Entry Points

**EIDAuthenticationPackage.dll (LSA Authentication Package):**
- Location: `EIDAuthenticationPackage/EIDAuthenticationPackage.cpp`
- Triggers: System boot (LSA loads it), logon attempts (Winlogon calls LSA), credential management operations (wizard)
- Responsibilities:
  - `LsaApInitializePackage()` - Package initialization at system boot
  - `LsaApLogonUserEx2()` - Primary authentication entry point, validates smart card credentials
  - `LsaApCallPackage()` - Trusted IPC for privileged operations (challenge/response)
  - `LsaApCallPackageUntrusted()` - Untrusted credential operations (enrollment/removal)
  - `LsaApLogonTerminated()` - Session cleanup on logoff
  - `DllRegister()` / `DllUnRegister()` - Installation and removal

**EIDCredentialProvider.dll (Credential Provider v2):**
- Location: `EIDCredentialProvider/Dll.cpp` (COM DLL exports), `EIDCredentialProvider/CEIDProvider.cpp` (main provider)
- Triggers: Winlogon initialization, user activity at login screen
- Responsibilities:
  - `DllGetClassObject()` - COM class factory for credential provider
  - `CEIDProvider::SetUsageScenario()` - Scenario detection (login, unlock, etc.)
  - `CEIDProvider::GetCredentialAt()` - Credential enumeration
  - `CEIDCredential::GetSerialization()` - Credential collection and transmission to LSA

**EIDPasswordChangeNotification.dll (Password Filter):**
- Location: `EIDPasswordChangeNotification/EIDPasswordChangeNotification.cpp`
- Triggers: User password change events
- Responsibilities:
  - `InitializeChangeNotify()` - DLL initialization
  - `PasswordFilter()` - Password acceptance (always returns TRUE)
  - `PasswordChangeNotify()` - Update stored credential when password changes

**EIDConfigurationWizard.exe (User Configuration Tool):**
- Location: `EIDConfigurationWizard/EIDConfigurationWizard.cpp`
- Triggers: User launch (desktop shortcut, Control Panel)
- Responsibilities: Multi-page wizard flow for smart card enrollment, testing, and policy configuration

**EIDConfigurationWizardElevated.exe (Elevated Helper):**
- Location: `EIDConfigurationWizardElevated/EIDConfigurationWizardElevated.cpp`
- Triggers: UAC elevation from main wizard
- Responsibilities: Policy management operations requiring admin rights (`forcepolicy.cpp`, `removepolicy.cpp`)

**EIDLogManager.exe (Diagnostic Tool):**
- Location: `EIDLogManager/EIDLogManager.cpp`
- Triggers: Administrator launch for troubleshooting
- Responsibilities: Enable/disable ETW tracing for authentication components

## Error Handling

**Strategy:** NTSTATUS return codes from LSA layer, HRESULT from COM layer, Win32 GetLastError() elsewhere

**Patterns:**
- LSA functions return `NTSTATUS` values (defined in `ntstatus.h`)
- COM interfaces return `HRESULT` values with `S_OK`, `E_NOINTERFACE`, `E_INVALIDARG`
- `__try/__except` blocks wrap exception-prone code with `EIDExceptionHandler()`
- Security audit events logged via `EIDSecurityAudit()` for authentication failures/success
- LastError preserved via `SetLastError()` before returning from helper functions
- `SecureZeroMemory()` used to erase sensitive data (PINs, passwords) from memory

## Cross-Cutting Concerns

**Logging:** ETW-based event tracing via `EIDCardLibraryTrace()` in `EIDCardLibrary/Tracing.h`; controlled by `EIDLogManager` utility

**Validation:** Certificate chain validation via `IsTrustedCertificate()` in `EIDCardLibrary/CertificateValidation.h`; CSP provider whitelist enforcement via `IsAllowedCSPProvider()`

**Authentication:** LSA protocol implementation in `EIDAuthenticationPackage/`; impersonation via `ImpersonateClient()` for authorization checks; TOCTOU prevention via locked security context during `MatchUserOrIsAdmin()`

---

*Architecture analysis: 2026-02-15*
