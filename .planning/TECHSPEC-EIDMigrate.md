# EIDMigrate - Technical Specification

**Document Version:** 1.0
**Date:** 2026-03-23
**Component:** EIDMigrate.exe - Bulk Credential Migration Tool

---

## Table of Contents
1. [Architecture Overview](#1-architecture-overview)
2. [Data Structures](#2-data-structures)
3. [IPC Message Definitions](#3-ipc-message-definitions)
4. [File Format Specification](#4-file-format-specification)
5. [Security Implementation](#5-security-implementation)
6. [Function Signatures](#6-function-signatures)
7. [Error Handling](#7-error-handling)
8. [Memory Management](#8-memory-management)
9. [Thread Safety](#9-thread-safety)

---

## 1. Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           EIDMigrate Architecture                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────┐         ┌─────────────────┐         ┌──────────────┐  │
│  │  EIDMigrate.exe │  IPC    │   EIDAuthPkg    │  LSA    │     LSA      │  │
│  │                 │────────▶│   (lsass.exe)   │────────▶│   Database   │  │
│  │  - CLI Parser   │  Unicode│                 │  RPC    │ L$_EID_<RID> │  │
│  │  - Export/Import│         │ - Enumerate     │         │              │  │
│  │  - File I/O     │         │ - Export        │         │              │  │
│  │  - Crypto       │         │ - Validate      │         │              │  │
│  └─────────────────┘         └─────────────────┘         └──────────────┘  │
│           │                                                       │         │
│           │                                                       │         │
│           ▼                                                       ▼         │
│  ┌─────────────────┐         ┌─────────────────┐                           │
│  │  Encrypted File │         │  Smart Card     │                           │
│  │  .eidm format   │         │  + PIN          │                           │
│  │  AES-256-GCM    │         │  (for decrypt)  │                           │
│  └─────────────────┘         └─────────────────┘                           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Component Summary

| Component | Purpose | Technology |
|-----------|---------|------------|
| EIDMigrate.exe | CLI tool for export/import | C++, Windows CNG |
| EIDAuthenticationPackage.dll | LSA authentication package | Existing IPC |
| EIDCardLibrary.dll | Credential management library | Existing code |
| .eidm file | Encrypted credential export | AES-256-GCM + PBKDF2 |

---

## 2. Data Structures

### 2.1 EID_PRIVATE_DATA (Existing)

```cpp
// Location: EIDCardLibrary/StoredCredentialManagement.h
#pragma pack(push, 1)
struct EID_PRIVATE_DATA
{
    EID_PRIVATE_DATA_TYPE dwType;           // 1=Clear, 2=Crypted, 3=DPAPI
    USHORT dwCertificatOffset;              // Offset to certificate
    USHORT dwCertificatSize;                // Certificate size
    USHORT dwSymetricKeyOffset;             // Offset to symmetric key
    USHORT dwSymetricKeySize;               // Symmetric key size
    USHORT dwPasswordOffset;                // Offset to encrypted password
    USHORT usPasswordLen;                   // Encrypted password length
    UCHAR Hash[CERT_HASH_LENGTH];           // SHA-256 hash (32 bytes)
    BYTE Data[sizeof(DWORD)];               // Certificate + key + password
};
#pragma pack(pop)
```

### 2.2 Export File Header (New)

```cpp
// Location: EIDMigrate/FileFormat.h
#pragma pack(push, 1)
struct EIDMIGRATE_FILE_HEADER
{
    UCHAR Magic[16];                        // "EIDMIGRATE\x00\x00\x00\x00\x00\x00"
    uint32_t FormatVersion;                 // Little-endian = 1
    UCHAR PBKDF2Salt[16];                   // PBKDF2 salt
    UCHAR GCMNonce[12];                     // GCM nonce/IV
    UCHAR Reserved[16];                     // Future use (zero)
    uint32_t PBKDF2Iterations;              // 600000 (0x000927C0)
    uint32_t GCMTagLength;                  // 16 (0x00000010)
    uint64_t PayloadLength;                 // Encrypted JSON size
    UCHAR GCMTag[16];                       // GCM authentication tag
};
static_assert(sizeof(EIDMIGRATE_FILE_HEADER) == 0x60, "Header must be 96 bytes");
#pragma pack(pop)
```

### 2.3 Credential Summary (New)

```cpp
// Location: EIDCardLibrary/EIDCardLibrary.h
#pragma pack(push, 1)
struct EIDM_CREDENTIAL_SUMMARY
{
    DWORD dwRid;                              // Relative ID
    WCHAR wszUsername[EIDM_MAX_USERNAME_LENGTH];  // Username (256 max)
    UCHAR CertificateHash[CERT_HASH_LENGTH];  // SHA-256 hash
    EID_PRIVATE_DATA_TYPE EncryptionType;     // 1, 2, or 3
    DWORD dwFlags;                            // Reserved
};
#pragma pack(pop)
```

---

## 3. IPC Message Definitions

### 3.1 Message Type Enumeration

```cpp
// Add to EIDCardLibrary.h enum EID_CALLPACKAGE_MESSAGE
enum EID_CALLPACKAGE_MESSAGE
{
    EIDCMCreateStoredCredential,
    EIDCMUpdateStoredCredential,
    EIDCMRemoveStoredCredential,
    EIDCMHasStoredCredential,
    EIDCMRemoveAllStoredCredential,
    EIDCMGetStoredCredentialRid,
    EIDCMEIDGinaAuthenticationChallenge,
    EIDCMEIDGinaAuthenticationResponse,

    // NEW: EIDMigrate messages
    EIDCMEnumerateCredentials,         // Enumerate all stored credentials
    EIDCMExportCredential,             // Export full credential data
};
```

### 3.2 Enumerate Credentials Request/Response

```cpp
// Request structure
#pragma pack(push, 1)
struct EIDM_ENUM_REQUEST
{
    EID_CALLPACKAGE_MESSAGE MessageType;   // EIDCMEnumerateCredentials
    DWORD dwFlags;                         // Reserved (must be 0)
    DWORD dwMaxCredentials;                // Max to return (0 = all)
};
#pragma pack(pop)

// Response structure
#pragma pack(push, 1)
struct EIDM_ENUM_RESPONSE
{
    DWORD dwError;                         // Win32 error code
    DWORD dwCredentialCount;               // Number returned
    DWORD dwTotalCredentials;              // Total available
    DWORD dwBufferSizeUsed;                // Buffer size used
    // Followed by: EIDM_CREDENTIAL_SUMMARY[dwCredentialCount]
};
#pragma pack(pop)
```

### 3.3 Export Credential Request/Response

```cpp
// Request structure
#pragma pack(push, 1)
struct EIDM_EXPORT_REQUEST
{
    EID_CALLPACKAGE_MESSAGE MessageType;   // EIDCMExportCredential
    DWORD dwRid;                           // RID of credential
    DWORD dwFlags;                         // Reserved (must be 0)
};
#pragma pack(pop)

// Response structure
#pragma pack(push, 1)
struct EIDM_EXPORT_RESPONSE
{
    DWORD dwError;                         // Win32 error code
    DWORD dwRid;                           // RID of exported credential
    DWORD dwPrivateDataSize;               // Size of EID_PRIVATE_DATA
    DWORD dwCertificateSize;               // Size of certificate
    DWORD dwPasswordSize;                  // Size of encrypted password
    EID_PRIVATE_DATA_TYPE EncryptionType;  // Password encryption type
    UCHAR CertificateHash[CERT_HASH_LENGTH];
    WCHAR wszUsername[EIDM_MAX_USERNAME_LENGTH];
    // Followed by: EID_PRIVATE_DATA + Certificate + Password
};
#pragma pack(pop)
```

---

## 4. File Format Specification

### 4.1 Binary Layout

```
┌─────────────────────────────────────────────────────────────────┐
│                    ENCRYPTED FILE FORMAT                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Offset 0x00 (16 bytes):  Magic Number                           │
│      "EIDMIGRATE\x00\x00\x00\x00\x00\x00"                       │
│                                                                  │
│  Offset 0x10 (4 bytes):   Format Version                         │
│      0x00000001                                                     │
│                                                                  │
│  Offset 0x14 (16 bytes):  PBKDF2 Salt                            │
│      Random per file                                              │
│                                                                  │
│  Offset 0x24 (12 bytes):  GCM Nonce                              │
│      Random per encryption                                        │
│                                                                  │
│  Offset 0x30 (16 bytes):  Reserved                               │
│      All zeros                                                     │
│                                                                  │
│  Offset 0x40 (4 bytes):   PBKDF2 Iterations                      │
│      600000 (0x000927C0)                                            │
│                                                                  │
│  Offset 0x44 (4 bytes):   GCM Tag Length                         │
│      16 (0x00000010)                                                │
│                                                                  │
│  Offset 0x48 (8 bytes):   Payload Length                         │
│      Size of encrypted JSON                                        │
│                                                                  │
│  Offset 0x50 (16 bytes):  GCM Authentication Tag                 │
│                                                                  │
│  Offset 0x60 (variable):  Encrypted JSON Payload                 │
│      AES-256-GCM encrypted                                         │
│                                                                  │
│  EOF-32 (32 bytes):       HMAC-SHA256                            │
│      HMAC of entire file (excluding this field)                   │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 4.2 JSON Schema

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "EIDAuthentication Credential Export",
  "type": "object",
  "required": ["version", "formatVersion", "exportDate", "sourceMachine", "credentials"],
  "properties": {
    "version": {"type": "integer", "minimum": 1, "maximum": 1},
    "formatVersion": {"type": "string", "pattern": "^EIDMigrate-v[0-9]+\\.[0-9]+$"},
    "exportDate": {"type": "string", "format": "date-time"},
    "sourceMachine": {"type": "string", "minLength": 1, "maxLength": 255},
    "exportedBy": {"type": "string", "minLength": 1, "maxLength": 255},
    "credentials": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["username", "sid", "rid", "certificateHash", "certificate",
                     "encryptionType", "encryptedPassword"],
        "properties": {
          "username": {"type": "string", "minLength": 1, "maxLength": 256},
          "sid": {"type": "string", "pattern": "^S-1-5-21-[0-9-]+$"},
          "rid": {"type": "integer", "minimum": 1000, "maximum": 4294967295},
          "certificateHash": {"type": "string", "pattern": "^[0-9a-f]{64}$"},
          "certificate": {"type": "string", "contentEncoding": "base64"},
          "encryptionType": {"type": "string", "enum": ["certificate", "dpapi", "cleartext"]},
          "encryptedPassword": {"type": "string", "contentEncoding": "base64"},
          "symmetricKey": {"type": "string", "contentEncoding": "base64"},
          "algorithm": {"type": "string", "enum": ["AES-256-CBC", "AES-256-GCM"], "default": "AES-256-CBC"}
        }
      }
    },
    "statistics": {
      "type": "object",
      "properties": {
        "totalCredentials": {"type": "integer"},
        "certificateEncrypted": {"type": "integer"},
        "dpapiEncrypted": {"type": "integer"},
        "skipped": {"type": "integer"}
      }
    }
  }
}
```

### 4.3 Example Export (Before Encryption)

```json
{
  "version": 1,
  "formatVersion": "EIDMigrate-v1.0",
  "exportDate": "2026-03-23T10:30:00Z",
  "sourceMachine": "OLDPC-01",
  "exportedBy": "ADMINISTRATOR",
  "credentials": [
    {
      "username": "jsmith",
      "sid": "S-1-5-21-123456789-1234567890-1234567890-1001",
      "rid": 1001,
      "certificateHash": "a1b2c3d4e5f67890abcdef1234567890abcdef1234567890abcdef1234567890",
      "certificate": "MIIDpDCCAoygAwIBAgIQQvHJK...",
      "encryptionType": "certificate",
      "encryptedPassword": "YWJjZGVmZ2hpams...",
      "symmetricKey": "U2FsdGVkX1+vx7...",
      "algorithm": "AES-256-CBC"
    }
  ],
  "statistics": {
    "totalCredentials": 1,
    "certificateEncrypted": 1,
    "dpapiEncrypted": 0,
    "skipped": 0
  }
}
```

---

## 5. Security Implementation

### 5.1 Cryptographic Parameters

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| Key Derivation | PBKDF2-HMAC-SHA256 | Standard, proven KDF |
| Iterations | 600,000 | OWASP 2024 recommendation |
| Salt Size | 16 bytes | 128-bit, prevents rainbow tables |
| Encryption | AES-256-GCM | Authenticated encryption |
| Key Size | 256 bits | Maximum security |
| Nonce Size | 96 bits | GCM recommendation |
| Tag Size | 128 bits | Maximum authentication |
| HMAC | HMAC-SHA256 | File integrity verification |

### 5.2 Passphrase Requirements

```cpp
constexpr DWORD MIN_PASSPHRASE_LENGTH = 16;

BOOL ValidatePassphraseStrength(PCWSTR pwszPassphrase)
{
    if (!pwszPassphrase || wcslen(pwszPassphrase) < MIN_PASSPHRASE_LENGTH)
        return FALSE;

    BOOL fHasUpper = FALSE, fHasLower = FALSE, fHasDigit = FALSE;
    for (size_t i = 0; i < wcslen(pwszPassphrase); i++)
    {
        if (iswupper(pwszPassphrase[i])) fHasUpper = TRUE;
        if (iswlower(pwszPassphrase[i])) fHasLower = TRUE;
        if (iswdigit(pwszPassphrase[i])) fHasDigit = TRUE;
    }

    return fHasUpper && fHasLower && fHasDigit;
}
```

### 5.3 Key Derivation

```cpp
CRYPTO_STATUS DeriveKeyFromPassphrase(
    __in PCWSTR pwszPassphrase,
    __in SIZE_T cchPassphrase,
    __out DERIVED_KEY* pDerivedKey)
{
    NTSTATUS status;
    BCRYPT_ALG_HANDLE hAlgorithm = NULL;

    // Generate random salt
    status = BCryptOpenAlgorithmProvider(&hAlgorithm, BCRYPT_RNG_ALGORITHM, nullptr, 0);
    status = BCryptGenRandom(hAlgorithm, pDerivedKey->rgbSalt, 16, 0);
    BCryptCloseAlgorithmProvider(hAlgorithm, 0);

    // Derive encryption key
    status = BCryptOpenAlgorithmProvider(&hAlgorithm, BCRYPT_PBKDF2_ALGORITHM, nullptr, 0);
    status = BCryptDeriveKeyPBKDF2(
        hAlgorithm, BCRYPT_SHA256_ALGORITHM,
        (PUCHAR)pwszPassphrase, (ULONG)(cchPassphrase * sizeof(WCHAR)),
        pDerivedKey->rgbSalt, 16, 600000,
        pDerivedKey->rgbKey, 32, 0);
    BCryptCloseAlgorithmProvider(hAlgorithm, 0);

    return BCRYPT_SUCCESS(status) ? CRYPTO_STATUS::SUCCESS : CRYPTO_STATUS::ERROR_KDF_FAILED;
}
```

### 5.4 Secure Memory Handling

```cpp
// Secure buffer that zeros on destruction
class SecureBuffer
{
private:
    PBYTE m_pbData;
    DWORD m_cbSize;

public:
    ~SecureBuffer()
    {
        if (m_pbData)
        {
            SecureZeroMemory(m_pbData, m_cbSize);
            free(m_pbData);
        }
    }

    void Lock()   { VirtualLock(m_pbData, m_cbSize); }
    void Unlock() { VirtualUnlock(m_pbData, m_cbSize); }
};
```

---

## 6. Function Signatures

### 6.1 New LSA IPC Functions

```cpp
// Location: EIDAuthenticationPackage/EIDAuthenticationPackage.cpp

// Message handler: EIDCMEnumerateCredentials
NTSTATUS HandleEnumerateCredentials(
    __in PLSA_CLIENT_REQUEST ClientRequest,
    __in PVOID ProtocolSubmitBuffer,
    __in PVOID ClientBufferBase,
    __out PVOID* ProtocolReturnBuffer,
    __out PULONG ReturnBufferLength);

// Message handler: EIDCMExportCredential
NTSTATUS HandleExportCredential(
    __in PLSA_CLIENT_REQUEST ClientRequest,
    __in PVOID ProtocolSubmitBuffer,
    __in PVOID ClientBufferBase,
    __out PVOID* ProtocolReturnBuffer,
    __out PULONG ReturnBufferLength);
```

### 6.2 EIDMigrate.exe Functions

```cpp
// Location: EIDMigrate/Export.cpp
HRESULT ExportCredentials(
    __in PCWSTR pwszOutputPath,
    __in_opt PCWSTR pwszPassword,
    __in BOOL fValidateCerts);

// Location: EIDMigrate/Import.cpp
HRESULT ImportCredentials(
    __in PCWSTR pwszInputPath,
    __in_opt PCWSTR pwszPassword,
    __in BOOL fDryRun,
    __in BOOL fCreateUsers,
    __in BOOL fContinueOnError);

// Location: EIDMigrate/List.cpp
HRESULT ListCredentials(__in BOOL fLocal, __in_opt PCWSTR pwszInputPath);

// Location: EIDMigrate/Validate.cpp
HRESULT ValidateExportFile(__in PCWSTR pwszInputPath);
```

### 6.3 Helper Functions

```cpp
// Location: EIDMigrate/UserManagement.cpp
HRESULT CreateLocalUserAccount(
    __in PCWSTR pwszUsername,
    __in PCWSTR pwszPassword);  // Use empty password

// Location: EIDMigrate/LsaClient.cpp
HRESULT EnumerateLsaCredentials(
    __out std::vector<EIDM_CREDENTIAL_SUMMARY>& credentials);

HRESULT ExportLsaCredential(
    __in DWORD dwRid,
    __out EID_PRIVATE_DATA** ppPrivateData,
    __out PDWORD pdwDataSize);

// Location: EIDMigrate/FileCrypto.cpp
HRESULT EncryptExportFile(
    __in PCWSTR pwszPassword,
    __in const std::string& jsonPayload,
    __out PCWSTR pwszOutputPath);

HRESULT DecryptExportFile(
    __in PCWSTR pwszInputPath,
    __in PCWSTR pwszPassword,
    __out std::string& jsonPayload);
```

---

## 7. Error Handling

### 7.1 Error Codes

```cpp
enum class EIDMIGRATE_ERROR : DWORD
{
    SUCCESS = 0,
    ERROR_INVALID_ARGUMENT = 1,
    ERROR_LSA_ACCESS_DENIED = 2,
    ERROR_NO_CREDENTIALS = 3,
    ERROR_FILE_WRITE = 4,
    ERROR_FILE_READ = 5,
    ERROR_INVALID_PASSPHRASE = 6,
    ERROR_FILE_CORRUPTED = 7,
    ERROR_NO_VALID_CREDENTIALS = 8,
    ERROR_SOME_FAILED = 9,
    ERROR_CERTIFICATE_NOT_TRUSTED = 10,
    ERROR_USER_NOT_FOUND = 11,
    ERROR_SMARTCARD_NOT_AVAILABLE = 12,
};
```

### 7.2 Error Handling Pattern

```cpp
HRESULT ExportCredentialsWithErrorHandling(__in PCWSTR pwszOutputPath)
{
    HRESULT hr = S_OK;
    LSA_HANDLE hLsaPolicy = nullptr;

    __try
    {
        // Open LSA policy
        hLsaPolicy = OpenLsaPolicy();
        if (!hLsaPolicy)
        {
            EIDMigrateTrace(WINEVENT_LEVEL_ERROR, L"Failed to open LSA policy");
            return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        }

        // Perform export
        hr = ExportCredentialsInternal(hLsaPolicy, pwszOutputPath);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DWORD dwExceptionCode = GetExceptionCode();
        EIDMigrateTrace(WINEVENT_LEVEL_ERROR, L"Exception: 0x%08x", dwExceptionCode);
        hr = HRESULT_FROM_WIN32(dwExceptionCode);
    }
    __finally
    {
        if (hLsaPolicy)
        {
            LsaClose(hLsaPolicy);
        }
    }

    return hr;
}
```

---

## 8. Memory Management

### 8.1 Memory Allocation Strategy

| Scenario | Allocation Type | Cleanup |
|----------|-----------------|---------|
| Temporary buffers | malloc/free | Explicit free + SecureZeroMemory |
| LSA buffers | LSA allocation functions | LSA free functions |
| Crypto operations | SecureBuffer class | RAII with zeroing |
| JSON payload | std::string | Automatic |

### 8.2 LSA Memory Management

```cpp
// Pattern for LSA buffer allocation
PVOID pLsaBuffer = nullptr;
ULONG cbLsaBuffer = 0;

__try
{
    // Allocate using LSA functions
    NTSTATUS status = MyLsaDispatchTable->AllocateClientBuffer(
        ClientRequest,
        cbRequired,
        &pLsaBuffer);

    if (!NT_SUCCESS(status))
        __leave;

    // Copy to client
    status = MyLsaDispatchTable->CopyToClientBuffer(
        ClientRequest,
        cbRequired,
        pLsaBuffer,
        pSourceData);
}
__finally
{
    if (pLsaBuffer)
    {
        MyLsaDispatchTable->FreeClientBuffer(ClientRequest, pLsaBuffer);
    }
}
```

---

## 9. Thread Safety

### 9.1 Single-Threaded Design

EIDMigrate.exe is designed as a single-threaded CLI tool:
- No concurrent access to shared resources
- No locks required
- Simplified error handling

### 9.2 LSA Thread Safety

The LSA authentication package runs in lsass.exe context:
- Each IPC message is handled independently
- Existing code uses proper synchronization
- No changes needed to existing thread safety

---

## 10. Constants Reference

```cpp
// File sizes
constexpr DWORD MAX_CREDENTIAL_COUNT = 1000;
constexpr DWORD MAX_USERNAME_LENGTH = 256;
constexpr DWORD MAX_CERTIFICATE_SIZE = 8192;
constexpr DWORD MAX_PRIVATE_DATA_SIZE = 20480;  // 20KB sanity check

// Crypto
constexpr DWORD PBKDF2_ITERATIONS = 600000;
constexpr DWORD PBKDF2_SALT_SIZE = 16;
constexpr DWORD PBKDF2_KEY_SIZE = 32;
constexpr DWORD GCM_NONCE_SIZE = 12;
constexpr DWORD GCM_TAG_SIZE = 16;

// LSA
constexpr LPCWSTR CREDENTIAL_LSAPREFIX = L"L$_EID_";
constexpr DWORD CERT_HASH_LENGTH = 32;

// File format
constexpr char EIDMIGRATE_MAGIC[16] = {
    'E', 'I', 'D', 'M', 'I', 'G', 'R', 'A', 'T', 'E',
    '\0', '\0', '\0', '\0', '\0', '\0'
};
constexpr uint32_t EIDMIGRATE_VERSION = 1;
```

---

## 11. File Structure

```
EIDMigrate/
├── EIDMigrate.cpp              // Main entry point, CLI parser
├── EIDMigrate.rc                // Resources (dialogs, strings)
├── Export.cpp                   // Export functionality
├── Import.cpp                   // Import functionality
├── List.cpp                     // List functionality
├── Validate.cpp                 // Validation functionality
├── LsaClient.cpp                // LSA IPC client
├── FileCrypto.cpp               // File encryption/decryption
├── UserManagement.cpp           // User account creation
├── CryptoHelpers.cpp            // PBKDF2, AES-GCM helpers
├── SecureMemory.cpp             // Secure memory utilities
├── AuditLogging.cpp             // Event logging
├── PinPrompt.cpp                // PIN prompt dialog
├── Tracing.h                    // Trace logging
├── FileFormat.h                 // File structures
├── CryptoHelpers.h              // Crypto utilities
├── SecureMemory.h               // Secure memory classes
├── resource.h
└── EIDMigrate.vcxproj           // Project file
```

---

## 12. Dependencies

| Library | Purpose | Linkage |
|---------|---------|---------|
| advapi32.lib | LSA, crypto, registry | System |
| crypt32.lib | Certificate functions | System |
| bcrypt.lib | Modern crypto (CNG) | System |
| netapi32.lib | User enumeration | System |
| EIDCardLibrary.lib | Credential structures | Project |
| jsoncpp.lib | JSON parsing | Third-party |

---

*End of Technical Specification*
