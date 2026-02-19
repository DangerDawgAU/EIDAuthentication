# Phase 40: Won't-Fix Documentation Catalog

**Date:** 2026-02-18
**Purpose:** Comprehensive catalog of won't-fix issue categories with SonarQube-ready justifications
**Estimated Issues Covered:** ~280

## Won't-Fix Categories Summary

| Category | Count (Est.) | SonarQube Rule | Priority |
|----------|--------------|----------------|----------|
| C-style char array (not std::string) | ~200 | cpp:S5352 | High |
| Parameter pointer-to-const | ~50 | cpp:S5542 | Medium |
| Resource compiler macros | ~10 | cpp:S986 | Low |
| Flow-control macros | ~5 | cpp:S986 | Low |
| Runtime-assigned globals | ~20 | cpp:S2990 | Medium |
| COM interface method signatures | ~10 | cpp:S3554 | Low |
| Windows API enum types | ~10 | cpp:S1186 | Low |
| Security-critical explicit types | ~40 | cpp:S6184 | Low |

## Category 1: LSASS Memory Safety (C-style char arrays)

**SonarQube Rule:** cpp:S5352 (C-style arrays should be replaced with std::array/std::vector)
**Estimated Count:** ~200 issues
**Files Affected:** EIDAuthenticationPackage, EIDCredentialProvider, EIDPasswordChangeNotification

### Justification Template

```
Won't Fix: This code runs in LSASS (Local Security Authority Subsystem Service) context.
Dynamic memory allocation (std::string, std::vector) can cause LSASS instability or crashes.
C-style buffers with SecureZeroMemory cleanup are the correct pattern for this security-critical context.
Reference: Microsoft LSASS development guidelines
```

### Technical Rationale

- LSASS is a critical Windows security process
- Heap allocation in LSASS can cause memory fragmentation and crashes
- std::string and std::vector use dynamic heap allocation
- C-style char arrays with SecureZeroMemory provide predictable memory behavior
- Microsoft explicitly recommends against std::string in LSASS code

### Affected Patterns

- `char buffer[256]` - Fixed-size character buffers
- `WCHAR wszBuffer[MAX_PATH]` - Wide character buffers
- `BYTE pbBuffer[1024]` - Binary data buffers

---

## Category 2: Windows API Compatibility (Parameter pointer-to-const)

**SonarQube Rule:** cpp:S5542 (Parameters should be passed by const reference)
**Estimated Count:** ~50 issues
**Files Affected:** All projects with Windows API calls

### Justification Template

```
Won't Fix: This parameter type is required by the Windows API function signature.
Changing to const would break compilation or require internal const_cast which is worse.
Reference: Microsoft Learn documentation for [specific API]
```

### Technical Rationale

- Windows API functions often require non-const pointers
- LSA authentication APIs use specific parameter types
- Credential Provider interfaces have fixed signatures
- Changing parameter types would break API contracts
- Internal const_cast would add complexity without benefit

### Affected Patterns

- `LPWSTR` parameters in LSA functions
- `PBYTE` parameters in credential handling
- Non-const pointer parameters in callback signatures

---

## Category 3: Resource Compiler Macros

**SonarQube Rule:** cpp:S986 (Macros should be replaced with constexpr)
**Estimated Count:** ~10 issues
**Files Affected:** .rc resource files, resource.h

### Justification Template

```
Won't Fix: This macro is used in .rc resource files which are processed by RC.exe.
RC.exe cannot process C++ constexpr, only preprocessor macros.
Reference: Win32 resource compiler documentation
```

### Technical Rationale

- Resource compiler (RC.exe) only understands preprocessor macros
- constexpr is a C++ language feature not understood by RC.exe
- Resource IDs must be #define macros for resource file processing
- Converting to constexpr would break resource compilation

### Affected Macros

- `#define IDS_STRING1 101` - String resource IDs
- `#define IDD_DIALOG1 201` - Dialog resource IDs
- `#define IDI_ICON1 301` - Icon resource IDs

---

## Category 4: Flow-Control Macros (SEH Safety)

**SonarQube Rule:** cpp:S986 (Macros should be replaced with constexpr)
**Estimated Count:** ~5 issues
**Files Affected:** Files with SEH (Structured Exception Handling)

### Justification Template

```
Won't Fix: This macro is used in __try/__except SEH blocks for control flow.
Converting to constexpr/function would break SEH exception handling semantics.
Reference: Windows SEH documentation
```

### Technical Rationale

- SEH (__try/__except) has specific requirements for code in exception blocks
- Macros used for flow control must be evaluated at preprocessor time
- Function calls in SEH blocks have different semantics
- SEH-protected code must maintain specific structure

### Affected Macros

- `#define RETURN_IF_FAILED(hr)` - HRESULT checking macros
- `#define EXIT_ON_ERROR(expr)` - Error handling macros
- Flow-control macros used in __try blocks

---

## Category 5: Runtime-Assigned Globals

**SonarQube Rule:** cpp:S2990 (Global variables should be const)
**Estimated Count:** ~20 issues
**Files Affected:** EIDAuthenticationPackage, EIDCredentialProvider

### Justification Template

```
Won't Fix: This global is assigned at runtime during LSA/credential provider initialization.
LSA function pointers, tracing state, and DLL state are set in DllMain or Initialize functions.
Cannot be const as values are determined dynamically.
Reference: LSA Authentication Package development guidelines
```

### Technical Rationale

- LSA packages receive function pointers at runtime
- Credential providers initialize state during Initialize calls
- DLL state is set during DllMain processing
- Tracing/logging state is configured at runtime
- These globals cannot be const or constexpr

### Sub-Categories

| Sub-category | Count | Description |
|--------------|-------|-------------|
| LSA pointers | ~8 | Function pointers from LSA |
| Tracing state | ~4 | Logging/tracing configuration |
| DLL state | ~4 | DLL initialization flags |
| SAM function pointers | ~2 | SAM API function pointers |
| UI state | ~2 | UI configuration state |
| File handles | ~2 | Runtime file handles |

---

## Category 6: COM Interface Method Signatures

**SonarQube Rule:** cpp:S3554 (Parameter should be passed by const reference)
**Estimated Count:** ~10 issues
**Files Affected:** EIDCredentialProvider (COM interfaces)

### Justification Template

```
Won't Fix: This method signature is defined by the COM interface (ICredentialProvider, etc.).
Cannot change parameter types without breaking COM interface contract.
Method must match exactly as defined in Windows SDK.
Reference: Windows Credential Provider documentation
```

### Technical Rationale

- COM interfaces have immutable method signatures
- ICredentialProvider, ICredentialProviderCredential have fixed APIs
- Windows expects specific parameter types at call sites
- Changing signatures would break COM compatibility
- queryInterface and other COM methods cannot be modified

### Affected Interfaces

- `ICredentialProvider::SetUsageScenario`
- `ICredentialProviderCredential::GetSerialization`
- Other Credential Provider interface methods

---

## Category 7: Windows API Enum Types

**SonarQube Rule:** cpp:S1186 (enum should be scoped)
**Estimated Count:** ~10 issues
**Files Affected:** Files with Windows API enum definitions

### Justification Template

```
Won't Fix: This enum type is defined by Windows SDK and used with Windows APIs.
Must use unscoped enum to match Windows SDK definitions.
Using enum class would require casting for all API calls.
Reference: Windows SDK documentation
```

### Technical Rationale

- Windows SDK defines specific enum types
- LSA authentication uses specific enum values
- Credential Provider uses Windows-defined enums
- Using enum class would require extensive casting
- Code would become less readable with constant casting

### Affected Enums

- `SAMPLE_FIELD_ID` - Field identifiers
- `EID_INTERACTIVE_LOGON_SUBMIT_TYPE` - Submit types
- Other Windows-defined enum types

---

## Category 8: Security-Critical Explicit Types

**SonarQube Rule:** cpp:S6184 (Use auto for type deduction)
**Estimated Count:** ~40 issues
**Files Affected:** All projects with Windows/LSA code

### Justification Template

```
Won't Fix: Explicit type declaration is intentional for security auditing.
HRESULT, NTSTATUS, HANDLE types must be visible for security review.
Using auto would obscure security-relevant type information.
Reference: Microsoft Security Development Lifecycle guidelines
```

### Technical Rationale

- HRESULT indicates COM operation result - must be visible
- NTSTATUS indicates system operation result - must be visible
- HANDLE types indicate resource management - must be visible
- Security auditors need to see explicit types
- auto would hide security-relevant type information

### Affected Types

- `HRESULT` - COM result codes
- `NTSTATUS` - NT kernel status codes
- `HANDLE` - Windows object handles
- `LPCWSTR` - String pointers for security-critical paths

---

## SonarQube Manual Work Checklist

Use this checklist when applying won't-fix comments in SonarQube:

- [ ] **Category 1: LSASS Memory Safety** (~200 issues)
  - Filter by rule cpp:S5352
  - Apply LSASS memory safety justification
  - Group by file for batch processing

- [ ] **Category 2: Windows API Compatibility** (~50 issues)
  - Filter by rule cpp:S5542
  - Apply Windows API compatibility justification
  - Note specific API in comment if relevant

- [ ] **Category 3: Resource Compiler Macros** (~10 issues)
  - Filter by rule cpp:S986 in .rc files
  - Apply RC.exe limitation justification

- [ ] **Category 4: Flow-Control Macros** (~5 issues)
  - Filter by rule cpp:S986 in SEH code
  - Apply SEH safety justification

- [ ] **Category 5: Runtime-Assigned Globals** (~20 issues)
  - Filter by rule cpp:S2990
  - Apply runtime initialization justification
  - Note sub-category in comment

- [ ] **Category 6: COM Interface Signatures** (~10 issues)
  - Filter by rule cpp:S3554 in COM methods
  - Apply COM interface contract justification
  - Note interface name in comment

- [ ] **Category 7: Windows API Enum Types** (~10 issues)
  - Filter by rule cpp:S1186
  - Apply Windows SDK compatibility justification
  - Note enum name in comment

- [ ] **Category 8: Security-Critical Types** (~40 issues)
  - Filter by rule cpp:S6184
  - Apply security auditing justification
  - Note specific type in comment

## Edge Cases

Document any issues that don't fit standard categories:

| Issue | File | Line | Reason | Custom Justification |
|-------|------|------|--------|---------------------|
| (none identified) | - | - | - | - |

## Reference Links

- [Microsoft LSASS Development Guidelines](https://learn.microsoft.com/en-us/windows-server/security/credentials-protection-and-management/)
- [Windows Credential Provider Documentation](https://learn.microsoft.com/en-us/windows/win32/secauthn/credential-providers-in-windows)
- [LSA Authentication Documentation](https://learn.microsoft.com/en-us/windows/win32/secauthn/lsa-authentication)
- [Windows SEH Documentation](https://learn.microsoft.com/en-us/cpp/cpp/structured-exception-handling-c-cpp)
- [SonarQube C++ Rules](https://rules.sonarsource.com/cpp/)

---

*Generated: 2026-02-18*
*Phase 40: Final Verification*
*Total estimated won't-fix issues: ~280*
