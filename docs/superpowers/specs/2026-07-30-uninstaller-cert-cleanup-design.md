# Uninstaller certificate cleanup — design

Date: 2026-07-30
Status: approved by user
Branch: binskim-hardening (lands in PR #53 at user's request)

## Problem

The uninstaller's "Remove EID Root Certificate Authority and user certificates" option has
never worked, and even if it ran it would miss most of the certificates it claims to remove:

1. **The PowerShell never executes.** `un.RemoveEIDCertificates` (Installer/Installerx64.nsi)
   passes the script to `nsExec::ExecToLog` as ~16 separate quoted strings joined by NSIS `\`
   line continuations. NSIS pushes each quoted string as a separate plugin argument and
   `ExecToLog` pops only the first, so the executed command is
   `powershell.exe -NoProfile -ExecutionPolicy Bypass -Command ` with an empty command.
   PowerShell exits immediately with an error; nothing is removed; the remaining strings are
   left on the uninstaller stack.
2. **The filter cannot match user certificates.** It checks `$c.Subject -like "*EID:*"`, but
   user certificates are created with subject `CN=<username>`
   (EIDConfigurationWizard/EIDConfigurationWizardPage03.cpp:211). Only their **issuer** is
   `CN=EID:<name>`. Only the root CA itself carries `EID:` in its subject.
3. **Wrong store coverage.** It scans CurrentUser My/TrustedPeople/Root + LocalMachine Root.
   The product actually writes to: LocalMachine **Root** (wizard root CA,
   EIDCardLibrary/CertificateUtilities.cpp `UI_CERTIFICATE_INFO_SAVEON_SYSTEMSTORE`),
   LocalMachine **CA** and **TrustedPeople** (`MakeTrustedCertifcate`,
   EIDCardLibrary/CertificateValidation.cpp), LocalMachine **My**
   (`UI_CERTIFICATE_INFO_SAVEON_SYSTEMSTORE_MY`), and CurrentUser **My** (user enrolment) —
   where "CurrentUser" is each enrolled user, not the admin running the uninstall.
4. **The CA's private key survives.** Removing the CA cert from LocalMachine Root leaves its
   machine CAPI key container on disk, so the signing key outlives the uninstall.

## Decisions (user-confirmed)

- Scope: machine stores **and all user profiles** (loaded and unloaded hives).
- Delete the CA's private key container as part of the cleanup.
- Land in **PR #53** (branch binskim-hardening).
- Both uninstall cleanup checkboxes default to **unchecked** (destructive cleanup is opt-in).

## Design

### 1. New export `CleanupEIDCertificates`

- Added to `EIDAuthenticationPackage/EIDAuthenticationPackage.def`.
- Thin `HRESULT WINAPI CleanupEIDCertificates()` in `EIDAuthenticationPackage.cpp`, mirroring
  the existing `CleanupLsaCredentials` export (rundll32 invocation, `__try/__except` +
  `EIDCardLibraryTrace` pattern).
- Logic in a new function `RemoveAllEIDCertificates()` in
  `EIDCardLibrary/CertificateUtilities.cpp` (beside the code that creates these certs),
  declared in CertificateUtilities.h.

### 2. Match rule

A certificate is EID-owned iff its **subject CN starts with `EID:`** (root CA) **or its
issuer CN starts with `EID:`** (certs issued by that CA). Names read via
`CertGetNameString(CERT_NAME_SIMPLE_DISPLAY_TYPE)`, issuer with `CERT_NAME_ISSUER_FLAG`.
Prefix match, not substring, so unrelated certs containing "EID:" mid-string are untouched.

### 3. Store sweep

- **LocalMachine:** `Root`, `CA`, `TrustedPeople`, `My`.
- **Loaded user profiles:** `CertEnumSystemStore(CERT_SYSTEM_STORE_USERS)`; clean the same
  four store names per SID (skip `.DEFAULT` duplicates gracefully; enumeration yields
  `<SID>\<store>` names).
- **Unloaded profiles** (common case — only the admin is logged on): enumerate
  `HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\ProfileList` for `ProfileImagePath`,
  skip SIDs already loaded under HKEY_USERS, enable `SeBackupPrivilege` +
  `SeRestorePrivilege`, `RegLoadKey` each `NTUSER.DAT` under a temp subkey, open each store
  with `CERT_STORE_PROV_SYSTEM_REGISTRY` on
  `<hive>\Software\Microsoft\SystemCertificates\<store>`, clean, close, `RegUnLoadKey`.
  A hive that fails to load (in use, corrupt) is traced and skipped.
- Deletion uses enumerate → `CertDuplicateCertificateContext` → `CertDeleteCertificateFromStore`
  so the enumerator stays valid (delete frees the context).

### 4. CA private key deletion — safety rail

Only when the deleted cert matched on **subject** (it IS the EID CA) and its
`CERT_KEY_PROV_INFO_PROP_ID` has `CRYPT_MACHINE_KEYSET`: delete via
`CryptAcquireContext(..., CRYPT_DELETEKEYSET | CRYPT_MACHINE_KEYSET)`. Never for issuer-only
matches — user certs' key-prov-info points at smart-card CSPs and `CRYPT_DELETEKEYSET`
would destroy the key **on the card**. The machine-keyset requirement makes card damage
impossible by construction.

### 5. Installer changes (Installer/Installerx64.nsi)

- Delete `un.RemoveEIDCertificates` (the broken PowerShell) entirely.
- Call site becomes
  `ExecWait 'rundll32.exe "$SYSDIR\EIDAuthenticationPackage.dll",CleanupEIDCertificates' $0`
  inside the existing `${DisableX64FSRedirection}` guards, **before** the DLL is deleted
  (same ordering as the LSA cleanup). Non-zero exit → `DetailPrint` warning, continue.
- Both cleanup checkboxes default **unchecked** (remove the two `${NSD_Check}` calls).
- Certificate checkbox label notes that removal includes the CA private key and is
  irreversible.

### 6. Error handling & observability

Per-store/per-profile failures are traced (`EIDCardLibraryTrace`) and skipped; uninstall is
never blocked. The export returns S_OK if the sweep ran (even partially), an error HRESULT
only if it could not start. Trace counts: certs removed, profiles swept, key containers
deleted.

### 7. Testing

`.\build.ps1 Release x64`, then on the VM:
1. Install, enrol a user (root CA + user cert created).
2. Confirm presence: `certutil -store root`, per-user stores.
3. Uninstall with the certificate box **ticked** → zero `EID:`-subject/issuer certs in
   machine stores and every profile; CA key container gone (`certutil -key`).
4. Reinstall, uninstall with the box **unticked** → certs and key survive.
Add to the VM checklist beside the smart-card logon test (both gate the PR #53 release).
