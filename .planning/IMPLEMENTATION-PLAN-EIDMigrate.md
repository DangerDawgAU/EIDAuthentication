# EIDMigrate - Implementation Plan

**Document Version:** 1.0
**Date:** 2026-03-23
**Estimated Total Effort:** 80-100 hours

---

## Phase Breakdown

| Phase | Description | Effort | Dependencies |
|-------|-------------|--------|--------------|
| 1 | Project Setup | 4h | None |
| 2 | LSA IPC Implementation | 12h | Phase 1 |
| 3 | Crypto Library | 16h | Phase 1 |
| 4 | File Format Implementation | 12h | Phase 3 |
| 5 | Export Functionality | 16h | Phases 2, 4 |
| 6 | Import Functionality | 20h | Phases 2, 4 |
| 7 | User Management | 8h | Phase 1 |
| 8 | CLI & Logging | 8h | Phases 5, 6 |
| 9 | Testing | 24h | All phases |
| **Total** | | **120h** | |

---

## Phase 1: Project Setup (4 hours)

### Task 1.1: Create Visual Studio Project
- [ ] Create `EIDMigrate/EIDMigrate.vcxproj`
- [ ] Configure for x64 only
- [ ] Set Unicode character set
- [ ] Configure runtime library (static /MT for release)
- [ ] Set output directories

**Dependencies:** EIDCardLibrary.lib

### Task 1.2: Create Basic File Structure
```
EIDMigrate/
├── EIDMigrate.cpp              // Main entry point
├── EIDMigrate.h
├── resource.h
├── EIDMigrate.rc
├── Export.cpp
├── Export.h
├── Import.cpp
├── Import.h
├── List.cpp
├── List.h
├── LsaClient.cpp
├── LsaClient.h
├── FileCrypto.cpp
├── FileCrypto.h
├── UserManagement.cpp
├── UserManagement.h
├── CryptoHelpers.cpp
├── CryptoHelpers.h
├── SecureMemory.cpp
├── SecureMemory.h
├── AuditLogging.cpp
├── AuditLogging.h
├── PinPrompt.cpp
├── PinPrompt.h
├── Tracing.h
├── FileFormat.h
├── Utils.h
└── Utils.cpp
```

### Task 1.3: Add Dependencies
- [ ] Add reference to EIDCardLibrary project
- [ ] Link: advapi32.lib, crypt32.lib, bcrypt.lib, netapi32.lib
- [ ] Add jsoncpp (NuGet or static)
- [ ] Configure include paths

### Task 1.4: Resource Files
- [ ] Create IDD_PIN_PROMPT dialog
- [ ] Create string table resources
- [ ] Create version info resource

---

## Phase 2: LSA IPC Implementation (12 hours)

### Task 2.1: Add IPC Message Definitions
**File:** `EIDCardLibrary/EIDCardLibrary.h`

```cpp
// Add to enum EID_CALLPACKAGE_MESSAGE
EIDCMEnumerateCredentials,    // Value: 8
EIDCMExportCredential,        // Value: 9

// New structures (add to file)
struct EIDM_ENUM_REQUEST { ... };
struct EIDM_ENUM_RESPONSE { ... };
struct EIDM_CREDENTIAL_SUMMARY { ... };
struct EIDM_EXPORT_REQUEST { ... };
struct EIDM_EXPORT_RESPONSE { ... };
```

**Effort:** 1h

### Task 2.2: Implement Enumerate Credentials Handler
**File:** `EIDAuthenticationPackage/EIDAuthenticationPackage.cpp`

- [ ] Add case for EIDCMEnumerateCredentials in LsaApCallPackageUntrusted
- [ ] Implement HandleEnumerateCredentials()
- [ ] Call MatchUserOrIsAdmin(0) for security check
- [ ] Enumerate users via NetUserEnum
- [ ] For each user, check if L$_EID_<RID> exists
- [ ] Build array of EIDM_CREDENTIAL_SUMMARY
- [ ] Allocate response buffer via AllocateClientBuffer
- [ ] Copy data via CopyToClientBuffer
- [ ] Handle buffer size constraints

**Effort:** 4h

### Task 2.3: Implement Export Credential Handler
**File:** `EIDAuthenticationPackage/EIDAuthenticationPackage.cpp`

- [ ] Add case for EIDCMExportCredential in LsaApCallPackageUntrusted
- [ ] Implement HandleExportCredential()
- [ ] Call MatchUserOrIsAdmin(0) for security check
- [ ] Call RetrievePrivateData(dwRid)
- [ ] Validate size (max 20KB)
- [ ] Get username via GetUsernameFromRid
- [ ] Build EIDM_EXPORT_RESPONSE
- [ ] Allocate and populate response buffer
- [ ] Include EID_PRIVATE_DATA + certificate + password

**Effort:** 5h

### Task 2.4: Update LSA Client
**File:** `EIDCardLibrary/Package.cpp`

- [ ] Add LsaEIDEnumerateCredentials() wrapper
- [ ] Add LsaEIDExportCredential() wrapper
- [ ] Handle Unicode conversion for IPC
- [ ] Add error handling and cleanup

**Effort:** 2h

---

## Phase 3: Crypto Library (16 hours)

### Task 3.1: PBKDF2 Key Derivation
**File:** `EIDMigrate/CryptoHelpers.cpp`

- [ ] Implement DeriveKeyFromPassphrase()
- [ ] Use BCryptDeriveKeyPBKDF2 with SHA-256
- [ ] 600,000 iterations
- [ ] Generate 16-byte random salt
- [ ] Derive separate encryption and auth keys
- [ ] Add error handling

**Effort:** 3h

### Task 3.2: AES-256-GCM Encryption
**File:** `EIDMigrate/CryptoHelpers.cpp`

- [ ] Implement EncryptWithGCM()
- [ ] Use BCryptOpenAlgorithmProvider with BCRYPT_AES_GCM_ALGORITHM
- [ ] Generate 12-byte nonce
- [ ] Setup BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO
- [ ] Return ciphertext + 16-byte tag

**Effort:** 3h

### Task 3.3: AES-256-GCM Decryption
**File:** `EIDMigrate/CryptoHelpers.cpp`

- [ ] Implement DecryptWithGCM()
- [ ] Verify authentication tag before decryption
- [ ] Return error if tag verification fails
- [ ] Return plaintext

**Effort:** 2h

### Task 3.4: Base64 Encoding/Decoding
**File:** `EIDMigrate/CryptoHelpers.cpp`

- [ ] Implement EncodeBase64() using CryptBinaryToStringA
- [ ] Implement DecodeBase64() using CryptStringToBinaryA
- [ ] Handle errors gracefully

**Effort:** 1h

### Task 3.5: HMAC File Integrity
**File:** `EIDMigrate/CryptoHelpers.cpp`

- [ ] Implement ComputeFileHMAC()
- [ ] Use HMAC(PBKDF2_key, "EIDMigrate integrity")
- [ ] Hash entire file except HMAC field
- [ ] Verify HMAC on import

**Effort:** 2h

### Task 3.6: Passphrase Validation
**File:** `EIDMigrate/CryptoHelpers.cpp`

- [ ] Implement ValidatePassphraseStrength()
- [ ] Check minimum 16 characters
- [ ] Warn on weak entropy (all same case, etc.)
- [ ] Return BOOL

**Effort:** 1h

### Task 3.7: Secure Memory Classes
**File:** `EIDMigrate/SecureMemory.cpp`

- [ ] Implement SecureBuffer class
- [ ] Implement SecureWString class
- [ ] Implement SecurePin class
- [ ] Implement CryptoOperationScope RAII wrapper
- [ ] All classes zero memory on destruction

**Effort:** 4h

---

## Phase 4: File Format Implementation (12 hours)

### Task 4.1: File Header Structure
**File:** `EIDMigrate/FileFormat.h`

- [ ] Define EIDMIGRATE_FILE_HEADER
- [ ] Define constants (Magic, offsets, sizes)
- [ ] Add static_assert for size validation

**Effort:** 1h

### Task 4.2: JSON Serialization
**File:** `EIDMigrate/FileCrypto.cpp`

- [ ] Implement CredentialToJson()
- [ ] Convert EID_PRIVATE_DATA to JSON object
- [ ] Base64 encode binary fields
- [ ] Convert certificate hash to hex string
- [ ] Add metadata (enrollment date, cert subject, etc.)

**Effort:** 3h

### Task 4.3: JSON Deserialization
**File:** `EIDMigrate/FileCrypto.cpp`

- [ ] Implement JsonToCredential()
- [ ] Parse JSON with schema validation
- [ ] Base64 decode binary fields
- [ ] Validate required fields
- [ ] Handle version compatibility

**Effort:** 3h

### Task 4.4: File Write
**File:** `EIDMigrate/FileCrypto.cpp`

- [ ] Implement WriteEncryptedFile()
- [ ] Create header with magic, version, timestamps
- [ ] Serialize credentials to JSON
- [ ] Encrypt JSON with AES-256-GCM
- [ ] Write header + encrypted payload + HMAC
- [ ] Set file permissions (600)

**Effort:** 3h

### Task 4.5: File Read
**File:** `EIDMigrate/FileCrypto.cpp`

- [ ] Implement ReadEncryptedFile()
- [ ] Validate magic number
- [ ] Validate version compatibility
- [ ] Verify HMAC
- [ ] Decrypt JSON payload
- [ ] Return parsed credentials

**Effort:** 2h

---

## Phase 5: Export Functionality (16 hours)

### Task 5.1: LSA Client Wrapper
**File:** `EIDMigrate/LsaClient.cpp`

- [ ] Implement EnumerateLsaCredentials()
- [ ] Call LsaEIDEnumerateCredentials IPC
- [ ] Parse response into vector<CredentialInfo>
- [ ] Handle errors

**Effort:** 2h

### Task 5.2: Credential Export from LSA
**File:** `EIDMigrate/LsaClient.cpp`

- [ ] Implement ExportLsaCredential()
- [ ] Call LsaEIDExportCredential IPC
- [ ] Parse response
- [ ] Return EID_PRIVATE_DATA structure

**Effort:** 2h

### Task 5.3: Main Export Logic
**File:** `EIDMigrate/Export.cpp`

- [ ] Implement ExportCredentials()
- [ ] Open LSA policy handle
- [ ] Enumerate all local users
- [ ] For each user:
  - [ ] Check if L$_EID_<RID> exists
  - [ ] Get credential type
  - [ ] Skip DPAPI with warning
  - [ ] Export certificate-based credentials
- [ ] Build JSON payload
- [ ] Collect statistics
- [ ] Encrypt and write file

**Effort:** 6h

### Task 5.4: Certificate Validation (Optional)
**File:** `EIDMigrate/Export.cpp`

- [ ] Implement ValidateCertificateForExport()
- [ ] Check certificate validity period
- [ ] Check certificate trust chain
- [ ] Warn on expired or untrusted certs
- [ ] Only if -validate flag specified

**Effort:** 3h

### Task 5.5: Export Error Handling
**File:** `EIDMigrate/Export.cpp`

- [ ] Handle LSA access denied
- [ ] Handle no credentials found
- [ ] Handle file write errors
- [ ] Log DPAPI skipped credentials
- [ ] Generate export report

**Effort:** 3h

---

## Phase 6: Import Functionality (20 hours)

### Task 6.1: Read and Parse Import File
**File:** `EIDMigrate/Import.cpp`

- [ ] Implement ReadImportFile()
- [ ] Prompt for passphrase
- [ ] Decrypt file
- [ ] Parse JSON
- [ ] Validate schema
- [ ] Return vector of credentials

**Effort:** 4h

### Task 6.2: User Account Lookup/Creation
**File:** `EIDMigrate/UserManagement.cpp`

- [ ] Implement LookupUserByName()
- [ ] Use NetUserEnum to find user
- [ ] Return SID and RID

- [ ] Implement CreateLocalUserAccount()
- [ ] Use NetUserAdd()
- [ ] Set password to empty
- [ ] Add to Users group
- [ ] Enable account

**Effort:** 4h

### Task 6.3: Certificate Validation on Import
**File:** `EIDMigrate/Import.cpp`

- [ ] Implement ValidateCertificateForImport()
- [ ] Parse certificate from import
- [ ] Verify in Trusted Root store
- [ ] Check validity period
- [ ] Warn on issues

**Effort:** 2h

### Task 6.4: Smart Card PIN Prompt
**File:** `EIDMigrate/PinPrompt.cpp`

- [ ] Create IDD_PIN_PROMPT dialog resource
- [ ] Implement SecurePinPrompt class
- [ ] DialogProc with password edit control
- [ ] Store PIN in SecurePin object
- [ ] Disable clipboard operations

**Effort:** 4h

### Task 6.5: Import Single Credential
**File:** `EIDMigrate/Import.cpp`

- [ ] Implement ImportSingleCredential()
- [ ] Lookup/create user account
- [ ] Get new RID
- [ ] Validate certificate
- [ ] Prompt for smart card PIN
- [ ] Store to LSA via CreateCredential
- [ ] Verify success

**Effort:** 3h

### Task 6.6: Main Import Logic
**File:** `EIDMigrate/Import.cpp`

- [ ] Implement ImportCredentials()
- [ ] Read and parse import file
- [ ] Dry-run mode: just validate and report
- [ ] Force mode: actual import
- [ ] For each credential:
  - [ ] Call ImportSingleCredential()
  - [ ] Continue on error if flag set
- [ ] Generate import report
- [ ] Log to Event Log

**Effort:** 3h

---

## Phase 7: User Management (8 hours)

### Task 7.1: User Lookup Functions
**File:** `EIDMigrate/UserManagement.cpp`

- [ ] Implement GetRidFromUsername()
- [ ] Implement GetUsernameFromRid()
- [ ] Implement GetSidFromUsername()
- [ ] Implement UserExists()

**Effort:** 2h

### Task 7.2: User Creation
**File:** `EIDMigrate/UserManagement.cpp`

- [ ] Implement CreateLocalUserAccount()
- [ ] Use NetUserAdd() with USER_INFO_1
- [ ] Set password to empty (blank)
- [ ] Add to local Users group
- [ ] Enable account (UF_ACCOUNTENABLE)
- [ ] Handle errors (user exists, etc.)

**Effort:** 3h

### Task 7.3: User Information Display
**File:** `EIDMigrate/UserManagement.cpp`

- [ ] Implement DisplayUserInfo()
- [ ] Show username, SID, RID
- [ ] Show account status (enabled/disabled)
- [ ] Show group memberships

**Effort:** 1h

### Task 7.4: SID Conversion Utilities
**File:** `EIDMigrate/UserManagement.cpp`

- [ ] Implement SidToString() (SDDL format)
- [ ] Implement StringToSid()
- [ ] Implement GetRidFromSid()

**Effort:** 2h

---

## Phase 8: CLI & Logging (8 hours)

### Task 8.1: Command-Line Parser
**File:** `EIDMigrate/EIDMigrate.cpp`

- [ ] Implement ParseCommandLine()
- [ ] Handle: export, import, list, validate commands
- [ ] Parse: -output, -input, -password, -dry-run, -force, -create-users, -verbose, -log
- [ ] Validate required arguments
- [ ] Show usage on error

**Effort:** 3h

### Task 8.2: Progress Display
**File:** `EIDMigrate/EIDMigrate.cpp`

- [ ] Implement ShowProgress()
- [ ] Display current operation
- [ ] Show credential count
- [ ] Show warnings and errors
- [ ] Verbose mode details

**Effort:** 2h

### Task 8.3: File Logging
**File:** `EIDMigrate/AuditLogging.cpp`

- [ ] Implement InitializeLogging()
- [ ] Create log file in same directory as export
- [ ] Log all operations with timestamps
- [ ] Log all errors and warnings
- [ ] Close log on exit

**Effort:** 2h

### Task 8.4: Windows Event Logging
**File:** `EIDMigrate/AuditLogging.cpp`

- [ ] Implement LogAuditEvent()
- [ ] Register EIDMigrate event source
- [ ] Define event messages
- [ ] Log: export success/failure, import success/failure, access denied
- [ ] Deregister on exit

**Effort:** 1h

---

## Phase 9: Testing (24 hours)

### Task 9.1: Unit Tests
- [ ] Test PBKDF2 key derivation
- [ ] Test AES-256-GCM encryption/decryption
- [ ] Test Base64 encoding/decoding
- [ ] Test JSON serialization/deserialization
- [ ] Test file format read/write
- [ ] Test SecureBuffer zeroing

**Effort:** 6h

### Task 9.2: Integration Tests
- [ ] Test LSA enumeration
- [ ] Test LSA export
- [ ] Test LSA import
- [ ] Test user creation
- [ ] Test smart card PIN validation

**Effort:** 6h

### Task 9.3: End-to-End Tests
- [ ] Export from test machine
- [ ] Verify file format
- [ ] Import to test machine
- [ ] Verify authentication works
- [ ] Test with multiple users
- [ ] Test with DPAPI credentials (should skip)
- [ ] Test with missing users (with -create-users)
- [ ] Test dry-run mode
- [ ] Test error conditions (wrong passphrase, corrupted file)

**Effort:** 8h

### Task 9.4: Security Testing
- [ ] Test weak passphrase rejection
- [ ] Test file tampering detection
- [ ] Test wrong passphrase handling
- [ ] Test memory zeroing (debugger verification)
- [ ] Test privilege checking

**Effort:** 4h

---

## Implementation Order

### Week 1: Foundation
- Day 1-2: Phase 1 (Project Setup)
- Day 3-4: Phase 2 (LSA IPC)
- Day 5: Phase 3 start (Crypto)

### Week 2: Core Functionality
- Day 1-3: Phase 3 complete (Crypto)
- Day 4-5: Phase 4 (File Format)

### Week 3: Export/Import
- Day 1-3: Phase 5 (Export)
- Day 4-5: Phase 6 start (Import)

### Week 4: Completion
- Day 1-2: Phase 6 complete, Phase 7 (User Management)
- Day 3: Phase 8 (CLI & Logging)
- Day 4-5: Phase 9 (Testing)

---

## Critical Success Factors

1. **LSA IPC reliability** - Must not crash LSASS (use SEH, validate all sizes)
2. **File encryption** - Must prevent tampering and brute force
3. **User mapping** - Must handle RID changes correctly
4. **DPAPI handling** - Must skip with clear warning
5. **Error handling** - Must roll back on partial import failures
6. **Memory security** - Must zero all sensitive data
7. **Privilege checking** - Must require administrator

---

## Risk Mitigation

| Risk | Mitigation |
|------|------------|
| LSASS crash | SEH blocks, size validation, extensive testing |
| Passphrase brute force | PBKDF2 with 600K iterations |
| File corruption | HMAC + GCM authentication tag |
| RID mismatch | Remap by username on import |
| Partial import failure | Dry-run default, atomic option |
| Memory exposure | SecureBuffer, VirtualLock, SecureZeroMemory |
| Smart card unavailable | Clear error messages, skip option |

---

*End of Implementation Plan*
