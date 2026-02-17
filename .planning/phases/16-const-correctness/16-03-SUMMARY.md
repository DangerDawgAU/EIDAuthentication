# Phase 16-03 Summary: Mark Member Functions Const

## Execution Status: Complete

**Date**: 2026-02-17

## Changes Made

### CContainer Class (EIDCardLibrary)

| Method | Change | Justification |
|--------|--------|---------------|
| GetProviderName() | Added const | Returns member pointer, no modification |
| GetContainerName() | Added const | Returns member pointer, no modification |
| GetKeySpec() | Added const | Returns member value, no modification |
| GetCertificate() | Added const | Returns duplicated cert context, no modification |
| IsOnReader() | Added const | String comparison only, no modification |
| GetCSPInfo() | Added const | Reads members, allocates new object |
| FreeCSPInfo() | Added const | Static-like method, no instance access |
| Erase() | Added const | Reads members only for crypto call |
| ViewCertificate() | Added const | Reads _pCertContext only |
| TriggerRemovePolicy() | Added const | Reads members, no state modification |

### CEIDCredential Class (EIDCredentialProvider)

| Method | Change | Justification |
|--------|--------|---------------|
| GetContainer() | Added const | Returns member pointer, no modification |

## Justified Exceptions (Not Marked Const)

### CContainer Methods Not Marked Const
- `GetUserName()` - **Lazy initialization** - modifies `_szUserName` on first call
- `GetRid()` - **Lazy initialization** - modifies `_dwRid` on first call
- `AllocateLogonStruct()` - Calls `GetRid()` which is non-const

### CEIDCredential Methods Not Marked Const
- All ICredentialProviderCredential interface methods - **COM interface contract**
  - Must match vtable layout, cannot add const
  - Methods: Advise, UnAdvise, SetSelected, SetDeselected, GetFieldState, GetStringValue, etc.

### CEIDProvider Methods Not Marked Const
- All ICredentialProvider interface methods - **COM interface contract**
  - Must match vtable layout, cannot add const
  - Methods: SetUsageScenario, SetSerialization, Advise, UnAdvise, GetFieldDescriptorCount, etc.

### CContainerHolderFactory Methods Not Marked Const
- `HasContainerHolder()`, `ContainerHolderCount()`, `GetContainerHolderAt()` - Use `Lock()`/`Unlock()` which modify CRITICAL_SECTION
  - Would require `mutable CRITICAL_SECTION` to mark const (design decision deferred)
- `SetUsageScenario()`, `ConnectNotification()`, `DisconnectNotification()`, etc. - Modify `_CredentialList`

## Verification

- [x] Solution builds successfully with all changes
- [x] No new compiler warnings introduced
- [x] Const member functions don't break existing callers
- [x] Const qualifiers consistent between declarations and definitions

## Files Modified

- EIDCardLibrary/CContainer.h
- EIDCardLibrary/CContainer.cpp
- EIDCredentialProvider/CEIDCredential.h
- EIDCredentialProvider/CEIDCredential.cpp

## Commit

```
c9e096e feat(const): add const-correctness to member functions and global variables
```
