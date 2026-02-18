# Cognitive Complexity Won't-Fix Documentation

**Date:** 2026-02-18
**Phase:** 36 - Complexity Reduction
**Requirement References:** STRUCT-01, STRUCT-02

## Overview

This document explains why certain cognitive complexity issues in the EIDAuthentication codebase are marked as **won't-fix** in SonarQube. These issues are primarily related to SEH (Structured Exception Handling) blocks that cannot be refactored due to LSASS (Local Security Authority Subsystem Service) safety requirements.

## SEH Constraint Rationale

### LSASS Context

The EIDAuthentication package runs within the LSASS process, which is a security-critical Windows subsystem. Code running in LSASS context has strict requirements:

1. **No C++ exceptions** - C++ exceptions require stack unwinding which is incompatible with SEH
2. **No heap-allocating STL containers** - `std::string`, `std::vector`, etc. can throw exceptions
3. **SEH for error handling** - `__try`/`__except`/`__finally` blocks must contain all related code

### Why SEH Blocks Cannot Be Refactored

SEH blocks use a different exception handling mechanism than C++ exceptions. The `__try` block must contain all code that could raise structured exceptions, and the `__except` or `__finally` handlers must be in the same lexical scope.

**Extracting code from inside a `__try` block would:**
- Break exception handling scope
- Potentially leak resources
- Crash LSASS if an exception occurs in extracted code

## Won't-Fix Files and Functions

### Files with SEH Blocks (Cannot Extract from `__try`)

| File | SEH Block Count | Notes |
|------|-----------------|-------|
| StoredCredentialManagement.cpp | 60+ | Credential encryption/decryption |
| EIDAuthenticationPackage.cpp | 20+ | LSA authentication handling |
| CContainer.cpp | 6 | Smart card container operations |
| CContainerHolderFactory.cpp | 2 | Container factory operations |
| CompleteToken.cpp | 6 | Token completion operations |
| GPO.cpp | 2 | Group Policy operations |

### Allowed Extractions

Code **CAN** be extracted in these cases:

1. **Code BEFORE `__try` block** - Validation logic, parameter checks, setup code
2. **Code AFTER `__finally` block** - Result processing, cleanup coordination
3. **New helper functions with own SEH** - Creating a new function that has its own `__try`/`__finally` structure

Code **CANNOT** be extracted:

1. **Code INSIDE `__try` block** - Must remain in the exception handling scope
2. **Code INSIDE `__except` handler** - Exception filter and handler code
3. **Code INSIDE `__finally` handler** - Cleanup code

## Complexity Reduction Achieved

In Phase 36, the following complexity reductions were made within SEH constraints:

### StoredCredentialManagement.cpp

- **CalculateSecretSize** - Extracted buffer size calculation
- **BuildSecretData** - Extracted data layout construction
- **EncryptPasswordWithDPAPI** - Extracted DPAPI encryption logic

These helpers are called from **outside** the `__try` block or in new helper functions with their own SEH.

### CertificateValidation.cpp

- **BuildCertificateChain** - Extracted chain building logic
- **VerifyChainPolicy** - Extracted policy verification
- **CheckChainTrustStatus** - Extracted trust status checking
- **CheckChainDepth** - Extracted depth validation
- **IsPolicySoftFailure** - Extracted failure classification

CertificateValidation.cpp does not use SEH (uses `Result<T>` pattern), so these extractions were unrestricted.

## SonarQube Documentation

### Won't-Fix Justification Template

When marking a cognitive complexity issue as won't-fix in SonarQube, use this justification:

```
Won't Fix: This function contains SEH (__try/__except/__finally) blocks that
cannot be refactored due to LSASS safety requirements. Code inside __try
blocks must remain in the same lexical scope for proper exception handling.
Extracting code would break exception handling and potentially crash LSASS.

See: docs/COMPLEXITY_WONTFIX.md
```

### Tags to Apply

- `lsass-context`
- `seh-protected`
- `wontfix-security-critical`

## Related Documentation

- **PROJECT.md** - Key Constraints section
- **Phase 36-01 SUMMARY.md** - Complexity reduction details
- **STATE.md** - Won't-Fix Categories table

## Conclusion

The cognitive complexity issues in SEH-protected code are intentional and necessary for LSASS safety. Alternative complexity reduction approaches (extracting code outside SEH blocks, creating new helpers with their own SEH) have been applied where possible. The remaining complexity is a required trade-off for system stability and security.
