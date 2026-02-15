# Codebase Structure

**Analysis Date:** 2026-02-15

## Directory Layout

```
EIDAuthentication/
├── EIDAuthenticationPackage/    # LSA authentication package DLL
├── EIDCardLibrary/              # Shared smart card utility library (static)
├── EIDConfigurationWizard/      # User-facing configuration wizard (GUI app)
├── EIDConfigurationWizardElevated/  # Elevated helper for admin operations
├── EIDCredentialProvider/       # Credential Provider v2 DLL for login screen
├── EIDLogManager/               # Event tracing control utility
├── EIDPasswordChangeNotification/   # Password filter DLL
├── Installer/                   # NSIS installer scripts and support files
├── include/                     # Public headers (cardmod.h from CPDK)
├── .github/workflows/           # CI/CD workflows
├── .planning/                  # Project planning and codebase documentation
├── x64/                        # Build output directory
├── .vs/                        # Visual Studio workspace files (generated)
├── EIDCredentialProvider.sln      # Visual Studio solution file
├── build.bat                    # Build automation script
├── build.ps1                    # PowerShell build script
├── BUILD.md                      # Build instructions
├── DEPLOYMENT.md                 # Deployment guide
├── notes.md                      # Development notes and history
├── LICENSE                       # GPL v3 license
└── README.md                     # Project overview and quick start
```

## Directory Purposes

**EIDAuthenticationPackage:**
- Purpose: LSA (Local Security Authority) authentication package - core authentication logic
- Contains: LSA protocol implementation, challenge-response handlers, logon session management
- Key files: `EIDAuthenticationPackage.cpp`, `EIDSecuritySupportProvider.cpp`, `EIDSecuritySupportProviderUserMode.cpp`, `EIDRundll32Commands.cpp`, `EIDAuthenticationPackage.def` (exports), `resources.rc` (resources), `resources.h`
- Build output: `EIDAuthenticationPackage.dll` (installed to System32 for LSA)

**EIDCardLibrary:**
- Purpose: Shared static library containing all smart card, certificate, and credential management utilities
- Contains: Certificate validation, stored credential management, smart card I/O, crypto operations, tracing, GPO policy handling, string conversion utilities
- Key files: `CContainer.cpp/h`, `CContainerHolderFactory.cpp/h`, `CSmartCardNotifier.cpp/h`, `CertificateUtilities.cpp/h`, `CertificateValidation.cpp/h`, `CompleteProfile.cpp/h`, `CompleteToken.cpp/h`, `CredentialManagement.cpp/h`, `GPO.cpp/h`, `Package.cpp/h`, `Registration.cpp/h`, `StoredCredentialManagement.cpp/h`, `StringConversion.cpp/h`, `TraceExport.cpp/h`, `Tracing.cpp/h`, `SmartCardModule.h`, `EIDCardLibrary.h`, `EIDAuthenticateVersion.h`
- Build output: `EIDCardLibrary.lib` (static library linked by other components)

**EIDConfigurationWizard:**
- Purpose: Multi-page Windows GUI application for smart card enrollment and configuration
- Contains: Wizard pages for certificate selection, credential enrollment, policy configuration, certificate testing, debug report generation
- Key files: `EIDConfigurationWizard.cpp/h`, `EIDConfigurationWizardPage02.cpp/h` through `EIDConfigurationWizardPage07.cpp/h` (individual wizard pages), `CContainerHolder.cpp/h`, `Common.cpp/h`, `DebugReport.cpp/h`, `EIDConfigurationWizard.rc` (dialog resources), `EIDConfigurationWizard.vcxproj`
- Build output: `EIDConfigurationWizard.exe`

**EIDConfigurationWizardElevated:**
- Purpose: UAC-elevated helper process for operations requiring administrator privileges
- Contains: Policy enforcement/removal dialogs and handlers
- Key files: `EIDConfigurationWizardElevated.cpp/h`, `forcepolicy.cpp`, `removepolicy.cpp`, `EIDConfigurationWizardElevated.rc` (dialog resources), `EIDConfigurationWizardElevated.vcxproj`
- Build output: `EIDConfigurationWizardElevated.exe`

**EIDCredentialProvider:**
- Purpose: Credential Provider v2 COM DLL that integrates with Windows login screen
- Contains: COM class factory, credential provider implementation, individual credential objects, smart card connection monitoring
- Key files: `Dll.cpp/h` (COM DLL exports), `CEIDProvider.cpp/h` (main provider), `CEIDCredential.cpp/h` (per-card credential), `CEIDFilter.cpp/h`, `CMessageCredential.cpp/h` (status messages), `helpers.cpp/h`, `guid.cpp` (COM CLSID), `EIDCredentialProvider.def` (exports), `EIDCredentialProvider.rc` (resources), `EIDCredentialProvider.vcxproj`
- Build output: `EIDCredentialProvider.dll` (registered as COM server, installed to System32)

**EIDLogManager:**
- Purpose: Diagnostic utility to enable/disable event tracing
- Contains: Tracing control UI, log management
- Key files: `EIDLogManager.cpp/h`, `stdafx.cpp/h`, `resource.h`, `EIDLogManager.rc`, `EIDLogManager.vcxproj`
- Build output: `EIDLogManager.exe`

**EIDPasswordChangeNotification:**
- Purpose: Password filter DLL that synchronizes password changes with stored smart card credentials
- Contains: Password notification handlers
- Key files: `EIDPasswordChangeNotification.cpp/h`, `resources.h`, `resources.rc` (resources), `EIDPasswordChangeNotification.def` (exports), `EIDPasswordChangeNotification.vcxproj`
- Build output: `EIDPasswordChangeNotification.dll` (installed to System32, registered in LSA notification packages)

**Installer:**
- Purpose: NSIS installer scripts and support files for deployment
- Contains: Installer script, PowerShell utilities for virtual smart card setup, service management, certificate cleanup
- Key files: `Installerx64.nsi` (NSIS script), `InstallVirtualSmartCard.ps1`, `EnableSmartCardServices.ps1`, `CleanupCertificates.ps1`, `CleanupPartialInstall.ps1`, `StartSmartCardService.bat`, `EnableLog.reg`, `DisableLog.reg`, `InstallEIDRef.reg`, `license.txt`

**include:**
- Purpose: Public header files from external SDKs (Cryptographic Provider Development Kit)
- Contains: Smart card module interface definitions
- Key files: `cardmod.h` (Microsoft smart card module interface)

**.github/workflows:**
- Purpose: CI/CD pipeline definitions
- Contains: GitHub Actions workflow files

**x64:**
- Purpose: Build output directory for Release/x64 configuration
- Contains: Compiled binaries, intermediate build artifacts

## Key File Locations

**Entry Points:**
- `EIDAuthenticationPackage/EIDAuthenticationPackage.cpp`: LSA package entry points (`LsaApInitializePackage`, `LsaApLogonUserEx2`, etc.)
- `EIDCredentialProvider/Dll.cpp`: COM DLL entry points (`DllGetClassObject`, `DllRegisterServer`, `DllUnregisterServer`)
- `EIDPasswordChangeNotification/EIDPasswordChangeNotification.cpp`: Password filter entry points (`InitializeChangeNotify`, `PasswordFilter`, `PasswordChangeNotify`)
- `EIDConfigurationWizard/EIDConfigurationWizard.cpp`: WinMain entry point for configuration wizard
- `EIDLogManager/EIDLogManager.cpp`: WinMain entry point for log manager utility

**Configuration:**
- `EIDCredentialProvider.sln`: Visual Studio solution containing all 7 projects
- `build.bat`: Command-line build script using MSBuild
- `build.ps1`: PowerShell build script
- `BUILD.md`: Build documentation and prerequisites
- `DEPLOYMENT.md`: Deployment guide and registry configuration

**Core Logic:**
- `EIDCardLibrary/StoredCredentialManagement.cpp/h`: Credential storage and retrieval logic
- `EIDCardLibrary/CertificateValidation.cpp/h`: Certificate trust validation, chain building, CSP whitelist
- `EIDCardLibrary/CContainer.cpp/h`: Smart card container abstraction
- `EIDAuthenticationPackage/EIDAuthenticationPackage.cpp`: LSA authentication protocol implementation
- `EIDCredentialProvider/CEIDProvider.cpp/h`: Credential provider COM object
- `EIDCredentialProvider/CEIDCredential.cpp/h`: Individual credential COM object

**Testing:**
- `EIDConfigurationWizard/EIDConfigurationWizardPage06.cpp/h`, `EIDConfigurationWizardPage07.cpp/h`: Test result pages in wizard

## Naming Conventions

**Files:**
- C++ source: `<ModuleName>.cpp` (e.g., `StoredCredentialManagement.cpp`)
- C++ headers: `<ModuleName>.h` (e.g., `StoredCredentialManagement.h`)
- Resource files: `<ModuleName>.rc` (e.g., `EIDConfigurationWizard.rc`)
- Resource headers: `resources.h` or `<ModuleName>.h` (e.g., `EIDConfigurationWizard.h` for resource IDs)
- Export definition files: `<ModuleName>.def` (e.g., `EIDAuthenticationPackage.def`)
- Project files: `<ModuleName>.vcxproj` (Visual Studio project)
- Wizard pages: `EIDConfigurationWizardPage<NN>.cpp/h` (01-07)

**Directories:**
- Project directories: `EID<ComponentName>` (e.g., `EIDCardLibrary`, `EIDCredentialProvider`)
- PascalCase for all project directories starting with `EID` prefix

**Functions:**
- LSA exports: `LsaAp<Operation>` (e.g., `LsaApLogonUserEx2`, `LsaApInitializePackage`)
- Password filter exports: `<Operation>ChangeNotify` (e.g., `InitializeChangeNotify`, `PasswordChangeNotify`)
- COM methods: PascalCase (e.g., `SetUsageScenario`, `GetSerialization`, `Advise`)
- Member functions: PascalCase (e.g., `GetUsernameFromCertContext`, `CreateCredential`)
- Windows API wrappers: PascalCase (e.g., `EIDCardLibraryTrace`, `EIDSecurityAudit`)

**Classes:**
- C prefix for classes (e.g., `CContainer`, `CStoredCredentialManager`, `CEIDProvider`, `CEIDCredential`)
- COM interfaces: `I<InterfaceName>` (e.g., `ICredentialProvider`, `ICredentialProviderCredential`)

**Types:**
- Structures: `ALL_CAPS` or `<MODULE>_<NAME>` (e.g., `EID_INTERACTIVE_LOGON`, `EID_CALLPACKAGE_BUFFER`, `EID_SMARTCARD_CSP_INFO`)
- Enums: `ALL_CAPS` or `<MODULE>_<NAME>` (e.g., `EID_CALLPACKAGE_MESSAGE`, `EID_MESSAGE_TYPE`)
- Type aliases (pointers): `P<STRUCT_NAME>` (e.g., `PEID_INTERACTIVE_LOGON`, `PLSA_STRING`)

**Constants:**
- `#define` macros: `ALL_CAPS` (e.g., `AUTHENTICATIONPACKAGENAME`, `CERT_HASH_LENGTH`)
- `constexpr` values: `camelCase` or `ALL_CAPS` (e.g., `AUTHENTICATIONPACKAGENAMEW`, `EID_MESSAGE_VERSION`)

## Where to Add New Code

**New Feature:**
- Primary code: Depends on feature type
  - Authentication logic: `EIDAuthenticationPackage/` (new LSA message handlers in `.cpp`, message types in `EIDCardLibrary/EIDCardLibrary.h`)
  - Credential provider UI: `EIDCredentialProvider/` (new credential classes, provider methods)
  - Smart card operations: `EIDCardLibrary/` (new utility modules)
  - Configuration UI: `EIDConfigurationWizard/` (new wizard pages)
- Tests: Not currently present - tests would go in new `test/` directory (needs to be created)

**New Component/Module:**
- Implementation: Add new subdirectory with `EID<ModuleName>/` prefix pattern
- Add `<ModuleName>.vcxproj` project file
- Reference from `EIDCredentialProvider.sln`
- Link against `EIDCardLibrary.lib` for shared utilities
- Include `EIDCardLibrary/EIDCardLibrary.h` for common definitions

**Utilities:**
- Shared helpers: `EIDCardLibrary/` (new `.cpp/.h` files)
- Export new utility declarations via `EIDCardLibrary.h` header
- Use `EIDAlloc/EIDFree` for memory allocation in LSA context

## Special Directories

**x64:**
- Purpose: Build output for Release/x64 configuration
- Generated: Yes
- Committed: No (in `.gitignore`)

**.vs:**
- Purpose: Visual Studio workspace configuration and intellisense database
- Generated: Yes
- Committed: Yes (though typically could be excluded)

**.planning:**
- Purpose: Project planning documentation, research notes, codebase analysis
- Generated: Yes (by GSD planning tools)
- Committed: Yes

**include:**
- Purpose: Third-party SDK headers (Microsoft CPDK)
- Generated: No
- Committed: Yes

**Installer:**
- Purpose: Deployment packaging and utilities
- Contains: `EIDInstallx64.exe` (compiled NSIS installer) - generated but committed for distribution
- Generated: Partially (NSIS output)
- Committed: Yes

---

*Structure analysis: 2026-02-15*
