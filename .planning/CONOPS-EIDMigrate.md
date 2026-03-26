# EIDMigrate - Concept of Operations (CONOPS)

**Document Version:** 1.0
**Date:** 2026-03-23
**Component:** EIDMigrate.exe - Bulk Credential Migration Tool

---

## 1. Purpose

EIDMigrate enables administrators to transfer EIDAuthentication credentials between machines during hardware replacement, OS reinstallation, or bulk deployment scenarios. It exports smart card-based credentials from LSA storage and imports them to target systems.

---

## 2. Intended Users and Use Cases

### Primary Users
| Role | Responsibilities | Privileges Required |
|------|------------------|---------------------|
| System Administrator | Execute export/import on domain-joined or workstations | Local Administrator |
| Deployment Engineer | Migrate users during PC refresh campaigns | Local Administrator |
| Security Auditor | Verify credential portability | Read-only access |

### Use Cases

#### UC1: Hardware Replacement
**Scenario:** User receives new workstation, old workstation is decommissioned.
```
Old PC → EIDMigrate export → Encrypted file → New PC → EIDMigrate import → User logs in
```
**Requirements:**
- User must have same username on target machine
- User's smart card with same certificate must be available
- Certificate authority must be trusted on both machines

#### UC2: OS Reinstallation
**Scenario:** Windows is reinstalled on same hardware.
```
Pre-format → EIDMigrate export → Backup → Reinstall Windows → EIDMigrate import
```
**Requirements:**
- Export file stored on external media/network share
- Same smart card required for import

#### UC3: Bulk Deployment
**Scenario:** Organization deploying 100 new workstations.
```
Golden master → EIDMigrate export → Distribution → 100 target PCs → EIDMigrate import
```
**Requirements:**
- Scriptable CLI interface
- Batch import capability
- Audit logging for compliance

#### UC4: Credential Backup
**Scenario:** Disaster recovery preparation.
```
Scheduled → EIDMigrate export → Encrypted backup → Secure storage
```
**Requirements:**
- Export file encrypted with strong passphrase
- Secure storage recommendations documented

---

## 3. Operational Scenarios

### Scenario 3.1: Standard Migration Flow

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         MIGRATION OPERATIONAL FLOW                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  SOURCE MACHINE (Old PC)                   TARGET MACHINE (New PC)          │
│  =======================                   =======================          │
│                                                                             │
│  1. Administrator logs on as local admin    5. Administrator logs on       │
│                                             as local admin                  │
│                                                                             │
│  2. Insert smart card (optional)             6. Install EIDAuthentication  │
│     (for validation only)                                                │
│                                                                             │
│  3. Run: EIDMigrate.exe export               7. Run: EIDMigrate.exe import │
│       -output credentials.eidm               -input credentials.eidm        │
│       -password <passphrase>                 -password <passphrase>         │
│                                                                             │
│  4. Transfer encrypted file via:             8. For each credential:       │
│     - USB drive (encrypted)                  - Verify username exists       │
│     - Network share (SMB)                    - Create account if requested  │
│     - Secure file transfer                   - Verify certificate trusted   │
│                                             - Store to LSA                 │
│                                                                             │
│                                             9. Generate import report      │
│                                                                             │
│  10. User inserts smart card                 10. User inserts smart card    │
│  11. User enters PIN                         11. User enters PIN            │
│  12. User logs in successfully               12. User logs in successfully  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Scenario 3.2: Error Handling Flow

```
Export Errors:
├── LSA access denied → "Run as Administrator" required
├── No credentials found → Warning message, exit code 0 (no-op is OK)
├── Certificate expired → Warning, include in export (may fail on import)
├── DPAPI credential found → Warning: "Will not be portable", skip

Import Errors:
├── Invalid passphrase → Abort, no credentials modified
├── File corrupted → Abort, report corruption details
├── Username not found → Warning, skip credential (unless -create-users)
├── Certificate not trusted → Warning, skip credential
├── Smart card not available → Warning, skip credential
├── RID conflict → Abort, no credentials modified (safety check)
```

---

## 4. Security Considerations

### 4.1 Export File Protection

| Threat | Mitigation |
|--------|------------|
| File interception in transit | AES-256 encryption with user passphrase |
| Brute force passphrase attack | PBKDF2 key derivation with 100,000 iterations |
| Temporary file exposure | In-memory encryption before disk write |
| Unauthorized access | File permissions set to 600 (owner-only) |

### 4.2 Operational Security

```
BEFORE Export:
- Verify smart card is present (optional but recommended)
- Document source machine name, timestamp, administrator
- Choose strong passphrase (minimum 16 characters)

DURING Export:
- Display progress for each credential
- Show warnings for non-portable credentials (DPAPI)
- Generate SHA-256 hash of export file for integrity verification

AFTER Export:
- Store export file securely (encrypted USB, secure share)
- Document transfer method and chain of custody
- Delete export file after successful import

BEFORE Import:
- Verify target machine trusts certificate authority
- Verify target has compatible EIDAuthentication version
- Confirm username mapping matches organizational policy

DURING Import:
- Require confirmation before writing to LSA
- Display each credential before import
- Log all import operations to Windows Event Log

AFTER Import:
- Verify each user can authenticate with smart card
- Delete export file from target machine
- Document successful migration
```

### 4.3 Privilege Requirements

| Operation | Required Privilege | Justification |
|-----------|-------------------|---------------|
| Export | Local Administrator | Access to `HKLM\SECURITY\Policy\Secrets` |
| Import | Local Administrator | Write to LSA private data |
| List (read-only) | Local Administrator | Read LSA private data |

---

## 5. Prerequisites

### Source Machine (Export)

| Prerequisite | Verification Method |
|--------------|---------------------|
| EIDAuthentication installed | Check `EIDAuthenticationPackage.dll` registered |
| Administrator access | Current token has `SeDebugPrivilege` |
| LSA accessible | `LsaOpenPolicy` succeeds |
| At least one enrolled user | `L$_EID_*` secrets exist |

### Target Machine (Import)

| Prerequisite | Verification Method |
|--------------|---------------------|
| EIDAuthentication installed | Check `EIDAuthenticationPackage.dll` registered |
| Administrator access | Current token has `SeDebugPrivilege` |
| Smart card reader installed | `SCardEstablishContext` succeeds |
| Minidriver installed | Aventura MyEID minidriver registered |
| CA certificate trusted | Certificate in `Trusted Root` |

### Environment

| Requirement | Minimum |
|-------------|---------|
| Windows Version | Windows 10/11 (same as EIDAuthentication) |
| Architecture | x64 (matches source) |
| Free disk space | 10 MB for export file (per 100 users) |

---

## 6. Design Decisions Required

### DD1: Export File Format
**Question:** What file format should be used for the export?

| Option | Advantages | Disadvantages |
|--------|------------|---------------|
| JSON (Base64 encoded) | Human-readable, easy to debug | Larger file size, parsing overhead |
| Binary (custom format) | Compact, fast parsing | Proprietary, harder to debug |
| XML | Standard, tools available | Verbose, complex parsing |
| Cryptographic Archive (7z/zip) | Built-in compression, encryption | External dependency |

**DECISION (2026-03-23):** JSON + Base64 for readability, with file-level AES-256 encryption

### DD2: User Account Creation
**DECISION (2026-03-23):** Create enabled accounts with blank password (smart card required for login)

### DD3: DPAPI Credential Handling
**Question:** How should DPAPI-encrypted credentials (Type 3) be handled?

| Option | Advantages | Disadvantages |
|--------|------------|---------------|
| Skip with warning | Safe, no false promises | Incomplete export |
| Export anyway | Complete export | Cannot import, confusing |
| Attempt conversion | Maximizes portability | May fail, complex |
| Prompt user | Flexible | Interactive, not scriptable |

**DECISION (2026-03-23):** Skip with warning, log to file for follow-up

### DD4: Certificate Validation
**Question:** Should certificates be validated during export or import?

| Option | Advantages | Disadvantages |
|--------|------------|---------------|
| Validate on export only | Catch issues early | False positives for offline scenarios |
| Validate on import only | Faster export | Late failure detection |
| Validate on both | Maximum safety | Slower overall |
| No validation | Fastest | May import invalid credentials |

**DECISION (2026-03-23):** Validate on export with warning, validate on import with fail

### DD5: Passphrase Protection
**Question:** How should the export file passphrase be protected?

| Option | Advantages | Disadvantages |
|--------|------------|---------------|
| Prompt interactively | Secure, no command-line logging | Not scriptable |
| Command-line argument | Scriptable | Visible in process list, logs |
| Environment variable | Scriptable, better hidden | Can be dumped |
| Config file | Persistent | Must be secured separately |

**DECISION (2026-03-23):** Prompt interactively by default, `-password` flag for scripting

### DD6: Smart Card Requirement
**Question:** Is the smart card physically required during export/import?

| Option | Advantages | Disadvantages |
|--------|------------|---------------|
| Required during export | Validates certificate is present | Requires all cards during migration |
| Required during import only | Flexible export | May export invalid credentials |
| Not required at all | Fully offline | May export invalid credentials |

**DECISION (2026-03-23):** Not required for export (offline mode), required for import validation

### DD7: Batch Import Strategy
**Question:** How should batch imports be handled?

| Option | Advantages | Disadvantages |
|--------|------------|---------------|
| All-or-nothing (atomic) | Clean rollback on error | Partial progress lost |
| Best-effort (continue on error) | Maximizes successful imports | Inconsistent state |
| Interactive confirmation per user | Safe, explicit | Not scriptable |
| Dry-run mode available | Test without changes | Extra step required |

**DECISION (2026-03-23):** Dry-run mode by default, `-force` flag for actual import with continue-on-error

### DD8: Audit Logging
**Question:** What audit trail should be maintained?

| Option | Advantages | Disadvantages |
|--------|------------|---------------|
| Console output only | Simple | No permanent record |
| Text log file | Persistent, searchable | Must be secured |
| Windows Event Log | Centralized, tamper-evident | Requires Event Log access |
| All of the above | Maximum flexibility | Complex configuration |

**DECISION (2026-03-23):** Console + text log file (stored in same directory as export)

---

## 7. Command-Line Interface

### 7.1 Export Command

```cmd
EIDMigrate.exe export -output <path> [options]

Options:
  -output, -o <path>      Output file path (required)
  -password, -p <pass>    Encryption passphrase (prompt if omitted)
  -format <json|bin>      Output format (default: json)
  -validate               Validate certificates during export
  -skip-dpapi             Skip DPAPI credentials (default: warn)
  -verbose, -v            Detailed output
  -log <path>             Log file path

Exit Codes:
  0    Success
  1    General error
  2    LSA access denied
  3    No credentials found
  4    File write error
  5    Invalid passphrase
```

### 7.2 Import Command

```cmd
EIDMigrate.exe import -input <path> [options]

Options:
  -input, -i <path>      Input file path (required)
  -password, -p <pass>   Decryption passphrase (prompt if omitted)
  -dry-run               Simulate import without changes
  -force                 Perform actual import
  -create-users          Create missing user accounts (disabled)
  -continue-on-error     Continue after individual credential errors
  -verbose, -v           Detailed output
  -log <path>            Log file path

Exit Codes:
  0    Success (all credentials imported)
  1    General error
  2    LSA access denied
  3    File read error
  4    Invalid passphrase
  5    File corrupted
  6    No valid credentials
  7    Some credentials failed (with -continue-on-error)
```

### 7.3 List Command

```cmd
EIDMigrate.exe list [options]

Options:
  -input, -i <path>      List contents of export file
  -local                 List local LSA credentials (no file)
  -verbose, -v           Show detailed credential info
  -json                  Output in JSON format

Exit Codes:
  0    Success
  1    Error
```

### 7.4 Validate Command

```cmd
EIDMigrate.exe validate -input <path> [options]

Options:
  -input, -i <path>      Export file to validate
  -require-smartcard     Verify smart card is available
  -verbose, -v           Detailed validation report
```

---

## 8. Operational Constraints

### 8.1 Limitations

| Limitation | Impact | Mitigation |
|------------|--------|------------|
| DPAPI credentials not portable | Some credentials cannot migrate | Re-enroll affected users |
| RID may change | Import must remap by username | Username lookup on import |
| Certificate trust required | Must install CA on target | Document in deployment guide |
| Smart card required for import | Cannot import without card | Plan card availability |

### 8.2 Performance

| Operation | Expected Time (100 users) |
|-----------|--------------------------|
| Export | ~5-10 seconds |
| Import (dry-run) | ~5 seconds |
| Import (actual) | ~30-60 seconds (includes LSA writes) |
| Validate | ~5 seconds |

---

## 9. Success Criteria

Migration is considered successful when:
1. Export file created without errors
2. All certificate-based credentials included
3. Import completed without LSA errors
4. Each user can authenticate with smart card + PIN
5. Event log confirms successful migration
6. DPAPI-only credentials documented for re-enrollment

---

## 10. Rollback Procedures

### Export Failure
- No rollback required (no changes to source system)

### Import Failure
- If partially imported: Use `RemoveAllStoredCredential` and retry
- If RID conflict: Delete conflicting LSA entry manually
- If certificate trust issue: Install CA certificate and re-import

### Post-Migration Issues
- User cannot authenticate: Verify card reader, minidriver, certificate trust
- DPAPI issues: Re-enroll affected user
- RID mismatch: Use `GetStoredCredentialRid` to verify

---

*End of CONOPS*
