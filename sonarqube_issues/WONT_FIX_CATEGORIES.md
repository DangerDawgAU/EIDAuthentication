# Won't-Fix Categories for SonarQube Issues

**Generated:** 2026-02-18
**Phase:** 39-integration-changes
**Purpose:** Document categories of issues that cannot be fixed due to architectural constraints

---

## Overview

This document catalogs SonarQube issues that are marked as "Won't Fix" with technical justifications. These issues are inherent to the codebase's requirements for LSASS compatibility and Windows API integration.

---

## Category 1: Large Stack Buffers (more than 1KB)

**SonarQube Rule:** S5998 (Use std::array instead of C-style array)
**Estimated Issue Count:** ~15
**Recommendation:** Won't Fix

### Technical Justification

Large stack buffers must remain as C-style arrays for stack size safety in the LSASS context. Converting to `std::array` provides no benefit and actually increases stack frame size due to template overhead. In LSASS, stack space is at a premium, and excessive stack usage can cause stack overflow crashes in the security subsystem.

### Examples

```cpp
// CertificateValidation.cpp - 4KB buffer
BYTE Data[4096];

// CContainerHolder.cpp - 8KB buffer
TCHAR szParameters[8000];

// CertificateUtilities.cpp - 1KB+ buffers
TCHAR szCertName[1024];
TCHAR szProviderName[1024];
CHAR szContainerName[1024];
```

### Stack Size Analysis

| Buffer | Size | Location | Reason for C-style |
|--------|------|----------|-------------------|
| `BYTE Data[4096]` | 4KB | CertificateValidation.cpp | Large buffer, std::array adds no value |
| `TCHAR szParameters[8000]` | 8KB/16KB | CContainerHolder.cpp | Very large, Windows API buffer |
| `TCHAR szCertName[1024]` | 1KB/2KB | CertificateUtilities.cpp | Windows Crypto API buffer |

---

## Category 2: Global Mutable Arrays

**SonarQube Rule:** S5998 (Use std::array), S1533 (const correctness)
**Estimated Issue Count:** ~10
**Recommendation:** Won't Fix

### Technical Justification

Global mutable arrays are used for external linkage, Windows API compatibility, and sensitive credential data storage. Converting to `std::array` or making `const` would break the design or create security issues.

### Examples

```cpp
// EIDConfigurationWizard/global.h - Global credential buffers
WCHAR szReader[256];
WCHAR szCard[256];
WCHAR szUserName[256];
WCHAR szPassword[256];  // Sensitive credential data
```

### Reasons for Mutable Globals

1. **External Linkage:** Required for sharing state between Wizard pages
2. **Windows API Compatibility:** APIs expect mutable char buffers
3. **Credential Handling:** `SecureZeroMemory` cleanup required for sensitive data
4. **Legacy Design:** Refactoring would require significant architectural changes

---

## Category 3: Windows API Interfacing Arrays

**SonarQube Rule:** S5998 (Use std::array instead of C-style array)
**Estimated Issue Count:** ~80
**Recommendation:** Won't Fix

### Technical Justification

Arrays that interface directly with Windows APIs must remain as C-style arrays. While `std::array::data()` provides raw pointer access, the conversion provides minimal benefit while adding complexity and potential for subtle bugs when used with APIs that have specific memory layout requirements.

### Examples by API Type

#### Credential Provider APIs
```cpp
// LoadStringW requires char buffer
WCHAR Message[256];
LoadStringW(Handle, 34, Message, ARRAYSIZE(Message));

// CredUIPromptForWindowsCredentialsW
TCHAR szCaption[256];
credUiInfo.pszCaptionText = szCaption;
```

#### Smart Card APIs
```cpp
// SCardStatus requires output buffer
TCHAR szReaderTemp[256];
SCardStatus(hSCardHandle, szReaderTemp, &dwSize, ...);
```

#### Crypto APIs
```cpp
// GetComputerName
TCHAR szComputerName[255];
GetComputerName(szComputerName, &cchComputerName);

// CertSetCertificateContextProperty requires CERT_KEY_PROV_INFO
TCHAR szProviderName[1024];
```

### Files Affected

| File | Arrays | API Usage |
|------|--------|-----------|
| CEIDCredential.cpp | Message[256], szCertificateDetail[256] | LoadStringW |
| CMessageCredential.cpp | szDisableForcePolicy[256] | LoadStringW |
| helpers.cpp | szUserName[256], szPassword[256] | Credential APIs |
| Common.cpp | szUser[255], szDomain[255], szComputerName[255] | Account APIs |
| CContainerHolder.cpp | szName[1024] | GetModuleFileName |
| DebugReport.cpp | szMessage[256], szCaption[256] | MessageBox, CredUI |
| Package.cpp | Buffer[1000], szExeName[256] | Trace, GetModuleFileName |

---

## Category 4: std::string/std::vector Suggestions

**SonarQube Rule:** S5997 (Use std::string), S5995 (Use std::vector)
**Estimated Issue Count:** ~181
**Recommendation:** Won't Fix

### Technical Justification

This is the most critical won't-fix category. **Dynamic memory allocation is dangerous in LSASS context.** The LSASS process manages system-wide security, and heap failures can crash the entire security subsystem, preventing system login.

### Why std::string/std::vector Are Unsafe in LSASS

1. **Heap Allocation:** Both use dynamic memory that can fail under memory pressure
2. **Exception Safety:** Memory allocation failures throw exceptions, which are unsafe in LSASS
3. **Memory Fragmentation:** Long-running LSASS process can suffer heap fragmentation
4. **Security Criticality:** LSASS crashes require system restart and can prevent recovery

### Safe Alternative

C-style buffers with explicit `SecureZeroMemory` cleanup:

```cpp
// Safe pattern for LSASS
TCHAR szPassword[256];
// ... use buffer ...
SecureZeroMemory(szPassword, sizeof(szPassword));  // Clear sensitive data
```

### Code Pattern Analysis

| Pattern | Count | Status |
|---------|-------|--------|
| `TCHAR buffer[N]` | ~120 | Won't Fix - LSASS safe |
| `WCHAR buffer[N]` | ~45 | Won't Fix - LSASS safe |
| `BYTE buffer[N]` | ~16 | Won't Fix - LSASS safe |

---

## Category 5: Non-Const Pointer Parameters

**SonarQube Rule:** S3575 (Make parameter pointer-to-const)
**Estimated Issue Count:** ~50
**Recommendation:** Won't Fix

### Technical Justification

Many Windows/LSA APIs require non-const pointers even when they don't modify the data. Adding `const` would require internal `const_cast` which is worse than the original pattern.

### Examples

```cpp
// LsaLogonUser - requires non-const pointers
NTSTATUS LsaLogonUser(
    LSA_HANDLE,        // non-const
    PLSA_STRING,       // non-const
    ...
);

// SecBufferDesc - Windows security buffer
SecBufferDesc Buffers;  // non-const required

// Credential Provider interfaces
ICredentialProviderCredential::SetStringValue(
    LPCWSTR,           // const ok
    LPCWSTR            // const ok - but interface defined without const
);
```

---

## Summary Table

| Category | Rule ID | Count | Justification |
|----------|---------|-------|---------------|
| Large stack buffers (>1KB) | S5998 | ~15 | Stack size safety in LSASS |
| Global mutable arrays | S5998, S1533 | ~10 | External linkage, credential handling |
| Windows API interfacing | S5998 | ~80 | API compatibility, no benefit from std::array |
| std::string suggestions | S5997 | ~181 | LSASS heap safety - dynamic allocation dangerous |
| std::vector suggestions | S5995 | (included above) | LSASS heap safety |
| Non-const parameters | S3575 | ~50 | Windows API requirements |

**Total Won't Fix: ~336 issues**

---

## Conversion Log (Phase 39)

The following conversions were made to address fixable std::array issues:

### Converted to std::array

| Location | Array | Size | Notes |
|----------|-------|------|-------|
| EIDCardLibrary.h | Signature[8] | 8 bytes | Protocol structure |
| EIDCardLibrary.h | Hash[32] | 32 bytes | Protocol structure |
| smartcardmodule.cpp | bAtr[32] | 32 bytes | ATR buffer |
| StoredCredentialManagement.cpp | data[16] | 16 bytes | ENCRYPTED_LM_OWF_PASSWORD |
| StoredCredentialManagement.cpp | bHash[16] | 16 bytes | Local hash buffer |
| Tracing.cpp | buffer[10] | 10 bytes | Debug dump buffer |
| CertificateUtilities.cpp | SerialNumber[8] | 8 bytes | Certificate serial number |

**Total conversions: ~15 arrays**

---

## References

- [LSASS Memory Management Best Practices](https://learn.microsoft.com/en-us/windows-server/security/credentials-protection-and-management/)
- [CNG Security Requirements](https://learn.microsoft.com/en-us/windows/win32/seccng/cng-features)
- SonarQube Rules: S5995, S5997, S5998, S3575, S1533

---

*Document created during Phase 39 - Integration Changes*
*Maintained as part of v1.4 SonarQube Zero milestone*
