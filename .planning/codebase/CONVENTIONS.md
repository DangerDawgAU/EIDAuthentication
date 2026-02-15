# Coding Conventions

**Analysis Date:** 2025-02-15

## Naming Patterns

**Files:**
- Header files: `.h` extension (e.g., `EIDCardLibrary.h`, `CContainer.h`)
- Source files: `.cpp` extension (e.g., `CContainer.cpp`, `StringConversion.cpp`)
- Resource headers: `resource.h`, `guid.h`
- Test files: Not detected - no test files found in codebase

**Classes:**
- PascalCase prefix with `C` letter: `CContainer`, `CStoredCredentialManager`, `CEIDCredential`, `CEIDProvider`
- Template classes use PascalCase with `C` prefix: `CContainerHolderFactory<T>`
- Interface classes use `I` prefix: `ISmartCardConnectionNotifierRef`

**Functions/Methods:**
- PascalCase for public methods: `GetUserName()`, `GetCertificate()`, `AllocateLogonStruct()`
- Private methods use PascalCase with underscore prefix: `CContainer::ValidateAndCopyString()`

**Variables:**
- Member variables: underscore prefix + PascalCase: `_szReaderName`, `_pCertContext`, `_dwRid`, `_cRef`
- Local variables: camelCase: `szUserName`, `dwSize`, `pBuffer`, `fReturn`
- Parameters: Hungarian notation + camelCase: `szSource`, `maxLength`, `szFieldName`
- Constants: all caps with underscores: `MAX_CONTAINER_NAME_LENGTH`, `REG_DWORD`

**Types:**
- Enums: PascalCase: `EID_INTERACTIVE_LOGON_SUBMIT_TYPE`, `GPOPolicy`
- Structs: PascalCase: `EID_INTERACTIVE_LOGON`, `CHAIN_VALIDATION_PARAMS`
- Type aliases: `P` prefix for pointers: `PEID_INTERACTIVE_LOGON`, `PCCERT_CONTEXT`
- Windows HANDLE types: `h` prefix lowercase: `hProv`, `hToken`, `hWnd`
- BOOL variables: `f` prefix: `fReturn`, `fSuccess`, `fImpersonating`

**Macros:**
- All caps with underscores: `EIDAlloc`, `EIDFree`, `UNREFERENCED_PARAMETER`
- Define macros use ALL_CAPS: `AUTHENTICATIONPACKAGENAME`, `CERT_HASH_LENGTH`

## Code Style

**Formatting:**
- Indentation: Not explicitly configured (no .clang-format or similar found)
- Brace style: K&R/Allman - opening brace on same line for functions, next line for classes
- Line length: No strict limit observed, generally under 120 characters
- Spacing: Spaces around operators, after commas

**Linting:**
- SAL (Source Annotation Language) annotations used: `__in`, `__out`, `__deref_out`, `__in_opt`
- CodeAnalysis warnings: `#include <CodeAnalysis/Warnings.h>` used
- Security annotations: `UNREFERENCED_PARAMETER` macro used extensively

**Pragma Warnings:**
- Deprecated function warnings suppressed for safe string functions (`4995`):
```cpp
#pragma warning(push)
#pragma warning(disable : 4995)
#include <Shlwapi.h>
#pragma warning(pop)
```

## Import Organization

**Order:**
1. Standard C library headers (`<Windows.h>`, `<tchar.h>`, `<stdio.h>`)
2. Project-specific headers (`"EIDCardLibrary.h"`, `"Tracing.h"`)
3. Third-party library headers

**Path Aliases:**
- No path aliases configured
- Relative imports use `../` for parent directories
- Project imports use quotes: `#include "../EIDCardLibrary/EIDCardLibrary.h"`

**Include Guards:**
- `#pragma once` used exclusively (no `#ifndef` guards)
- All header files use `#pragma once` as first line after license

## Error Handling

**Patterns:**
- Use structured exception handling (`__try`/`__except`) for Windows SEH
- SetLastError() calls before returning failure
- BOOL return pattern: `TRUE` on success, `FALSE` on failure
- HRESULT return pattern: `S_OK` on success, error code on failure
- NTSTATUS return pattern: `STATUS_SUCCESS` on success

**Error Propagation:**
```cpp
DWORD dwError = 0;
__try
{
    // ... code ...
}
__finally
{
    if (pCertContext)
        CertFreeCertificateContext(pCertContext);
}
SetLastError(dwError);
return fReturn;
}
```

**Error Logging:**
- `EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING, L"Message 0x%08x", error)` for warnings
- `EIDSecurityAudit(SECURITY_AUDIT_FAILURE, L"Message")` for security events
- `MessageBoxWin32(error)` for user-facing errors (wizard only)

**Validation:**
- Always validate pointer parameters before use
- Use `ARRAYSIZE()` macro for buffer bounds checking
- String length validation before copy operations

## Logging

**Framework:** ETW (Event Tracing for Windows) via custom wrapper

**Levels:**
- `WINEVENT_LEVEL_CRITICAL` (1): Abnormal exits
- `WINEVENT_LEVEL_ERROR` (2): Severe errors
- `WINEVENT_LEVEL_WARNING` (3): Warnings like allocation failures
- `WINEVENT_LEVEL_INFO` (4): Non-error events
- `WINEVENT_LEVEL_VERBOSE` (5): Detailed trace events

**Patterns:**
```cpp
EIDCardLibraryTrace(WINEVENT_LEVEL_INFO, L"Message %s", param);
EIDSecurityAudit(SECURITY_AUDIT_SUCCESS, L"Operation succeeded");
```

**When to Log:**
- Entry/exit of key functions at INFO level
- All failures at WARNING level
- Security-relevant events via `EIDSecurityAudit()`
- Verbose debugging only when tracing enabled

## Comments

**When to Comment:**
- File headers: GPL license block in all source files
- Security notes: Mark security fixes with "CWE-XXX: description"
- Complex algorithms: Explain non-obvious logic
- Registry keys: Document purpose
- Workarounds: Document why non-standard approach used

**JSDoc/TSDoc:**
- Not used - C++ codebase

**Security Fix Annotations:**
```cpp
// SECURITY FIX #37: Add synchronization for trace globals (CWE-362)
// ETW EnableCallback can be called from any thread; protect shared state
static CRITICAL_SECTION g_csTrace;
```

## Function Design

**Size:** No strict limit, but functions generally under 100 lines

**Parameters:**
- Use `__in` for input parameters (SAL annotation)
- Use `__out` for output parameters
- Use `__in_opt` for optional input parameters
- Use `__deref_out` for dereferenced output parameters
- Pointer parameters: Hungarian notation indicates type (`sz` = string, `dw` = dword, `pb` = byte pointer)

**Return Values:**
- BOOL functions: `TRUE`/`FALSE`
- HRESULT functions: `S_OK` for success
- DWORD functions: `ERROR_SUCCESS` (0) for success
- NTSTATUS functions: `STATUS_SUCCESS` for success
- Out parameters: Set via pointer dereference

## Module Design

**Exports:**
- DLL exports use `extern "C"` linkage
- Class factory pattern: `Instance()` static method for singletons
- API functions declared in headers, implemented in .cpp files

**Barrel Files:**
- `EIDCardLibrary.h` - Main library header with shared definitions
- `StringConversion.h`/`cpp` - C++ string utilities in `EID` namespace
- Other modules expose their own headers

**Namespaces:**
- `EID` namespace used for C++ string utilities (`StringConversion.h`)
- Most code uses global namespace (C-style)

## Memory Management

**Allocation:**
- `EIDAlloc()` - Custom allocator with file/line tracking
- `EIDFree()` - Custom deallocator with file/line tracking
- `CoTaskMemAlloc()` - COM allocations
- `HeapAlloc()` - Direct heap allocations (rare)

**Security Patterns:**
- `SecureZeroMemory()` for sensitive data before freeing
- PIN/passwords always zeroed after use
- Use `CredProtectW()` for password protection (Vista+)

**Smart Pointers:**
- Limited use - mostly raw pointers with manual management
- PCCERT_CONTEXT uses `CertDuplicateCertificateContext()`/`CertFreeCertificateContext()`

## Thread Safety

**Synchronization:**
- `CRITICAL_SECTION` for protecting shared state
- `EnterCriticalSection()`/`LeaveCriticalSection()` pattern
- `InterlockedIncrement()`/`InterlockedDecrement()` for ref counts

**Patterns:**
```cpp
if (g_csTraceInitialized)
{
    EnterCriticalSection(&g_csTrace);
    IsTracingEnabled = (IsEnabled == EVENT_CONTROL_CODE_ENABLE_PROVIDER);
    LeaveCriticalSection(&g_csTrace);
}
```

## Security Patterns

**Input Validation:**
- Always validate buffer lengths before copy operations
- Use `ARRAYSIZE()` instead of sizeof for arrays
- Check for NULL pointers before dereferencing

**CSP Whitelist:**
- `IsAllowedCSPProvider()` validates against whitelist
- Prevents arbitrary code loading via CSP injection

**Authentication:**
- `MatchUserOrIsAdmin()` for authorization checks
- Impersonation used for security context locking
- Audit all security-relevant events

---
*Convention analysis: 2025-02-15*
