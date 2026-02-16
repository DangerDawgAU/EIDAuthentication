# Codebase Concerns

**Analysis Date:** 2026-02-15

## Tech Debt

### Memory Management: Custom Allocators

**Issue:** The codebase uses custom `EIDAlloc`/`EIDFree` wrappers throughout instead of standard C++ memory management. This creates maintenance burden and prevents use of RAII patterns.

**Files:**
- `EIDCardLibrary/Package.cpp` - Custom allocator implementation with debug markup
- `EIDCardLibrary/EIDCardLibrary.h` - Allocator declarations
- Used throughout all components via `EIDAlloc`/`EIDFree` macros

**Impact:**
- No automatic memory cleanup on exceptions
- Manual tracking required for all allocations
- Debug markup bloats allocations in debug builds
- Incompatible with modern C++ smart pointers
- Heap corruption risk from manual pairing

**Fix approach:**
- Phase 1: Add RAII wrappers for common allocations
- Phase 2: Introduce `std::unique_ptr` with custom deleters
- Phase 3: Migrate to standard allocators where LSA allows

---

### Structured Exception Handling (SEH) Overuse

**Issue:** Extensive use of `__try`/`__except`/`__finally` blocks instead of modern C++ error handling. SEH doesn't play well with C++ exceptions or RAII.

**Files:**
- `EIDCardLibrary/StoredCredentialManagement.cpp` - 60+ SEH blocks
- `EIDCardLibrary/CContainer.cpp` - 6 SEH blocks
- `EIDCardLibrary/CContainerHolderFactory.cpp` - 2 SEH blocks
- `EIDAuthenticationPackage/EIDAuthenticationPackage.cpp` - 20+ SEH blocks
- `EIDCardLibrary/CompleteToken.cpp` - 6 SEH blocks
- `EIDCardLibrary/GPO.cpp` - 2 SEH blocks

**Impact:**
- SEH doesn't unwind C++ objects properly
- Prevents use of `std::expected` in SEH blocks
- Difficult to reason about control flow
- Manual cleanup required in `__finally` blocks

**Fix approach:**
- Replace SEH with `std::expected<T, E>` for normal error handling
- Use SEH only at module boundaries for external API calls
- Implement RAII resource management to eliminate `__finally` blocks

---

### Mixed Error Handling Patterns

**Issue:** Code uses HRESULT, NTSTATUS, BOOL, and `std::expected` inconsistently. Error handling strategy is unclear.

**Files:**
- `EIDAuthenticationPackage/EIDAuthenticationPackage.cpp` - Mixed HRESULT/NTSTATUS
- `EIDCardLibrary/CertificateValidation.cpp` - Mixed BOOL/HRESULT
- All credential provider files - HRESULT returns
- LSA package files - NTSTATUS returns

**Impact:**
- Inconsistent error propagation
- Type confusion between error codes
- Loss of error information across boundaries
- Difficult to debug error paths

**Fix approach:**
- Adopt `std::expected<T, ErrorCode>` for internal error handling
- Convert to appropriate type only at API boundaries
- Define standard error enum for all component errors

---

### Pre-C++14 String Handling

**Issue:** Extensive use of raw WCHAR pointers, manual string length calculations, and unsafe string operations. Modern C++ string utilities are unused.

**Files:**
- `EIDCardLibrary/StringConversion.cpp` - Manual string conversion
- `EIDCardLibrary/Package.cpp` - Manual UNICODE_STRING manipulation
- `EIDCredentialProvider/CEIDCredential.cpp` - Raw WCHAR handling
- All credential provider files - C-style string operations

**Impact:**
- Buffer overflow risk from manual length calculations
- Memory leaks from missed CoTaskMemFree calls
- Inefficient string copying
- Type safety issues

**Fix approach:**
- Use `std::wstring` for internal string storage
- Use `std::wstring_view` for read-only parameters
- Use `std::format` instead of `swprintf_s`
- Convert to `LPCWSTR` only at Windows API boundaries

---

### Windows 7 Platform Target

**Issue:** Code targets Windows Vista (WINVER=0x0600) but `.planning/PROJECT.md` indicates Windows 7+ requirement. Windows 7 is end-of-life (2020).

**Files:**
- `EIDAuthenticationPackage/EIDAuthenticationPackage.vcxproj` - WINVER=0x0600
- All `.vcxproj` files - WINVER=0x0600

**Impact:**
- Unable to use newer Windows security features
- Testing burden on obsolete OS
- Dead code paths for Vista compatibility
- Delayed modernization

**Fix approach:**
- Drop Vista support completely (already done per notes.md)
- Set WINVER=0x0601 for Windows 7 minimum
- Document Windows 7 EOL in security considerations
- Plan to drop Windows 7 entirely for Windows 10+ target

---

## Known Bugs

### Typo in Error Message

**Symptoms:** "Unknow Error" instead of "Unknown Error"

**Files:** `EIDCredentialProvider/CEIDCredential.cpp:649`

**Trigger:** User authentication failure displays in logon UI

**Workaround:** None (minor cosmetic issue)

---

### Credential Provider Typo

**Symptoms:** "définit le point d'entrée" (French comment) in English codebase

**Files:** Multiple files contain French comments

**Trigger:** N/A (documentation only)

**Workaround:** None (documentation inconsistency only)

---

### Deprecated `GetVersionExW` Usage

**Symptoms:** Compiler warning C4996, deprecated API

**Files:** Identified in notes.md (search required for exact location)

**Trigger:** Any code path determining Windows version

**Workaround:** Use `VersionHelpers.h` APIs instead

**Fix approach:**
- Replace `GetVersionExW` with `IsWindowsXOrGreater()` functions
- Use `VerifyVersionInfo()` for complex version checks

---

### Deprecated String Function Warnings

**Symptoms:** C4996 warnings for deprecated string functions

**Files:**
- `EIDConfigurationWizard/PragmaWarnings.h` - Disables 4995 for Shlwapi.h/strsafe.h

**Trigger:** Build with /W4 warnings enabled

**Workaround:** Pragma warning disable in place

**Fix approach:**
- Replace deprecated functions with modern equivalents
- Use `StringCch*` functions from `strsafe.h` consistently
- Remove pragma disables once functions are replaced

---

## Security Considerations

### LSA Package Security Context

**Risk:** The EIDAuthenticationPackage runs in lsass.exe process. Any vulnerability can lead to credential dumping, privilege escalation, or system compromise (BSOD).

**Files:**
- `EIDAuthenticationPackage/EIDAuthenticationPackage.cpp` - LSA package entry points
- `EIDAuthenticationPackage/EIDSecuritySupportProvider.cpp` - SSP implementation
- All code loaded into lsass.exe

**Current mitigation:**
- LSA protection enabled (requires signed DLL)
- Debug credential file storage disabled (see StoredCredentialManagement.cpp:2048-2060)
- SEH to catch access violations

**Recommendations:**
- Enable Control Flow Guard (/guard:cf)
- Enable ASLR (/DYNAMICBASE)
- Sign all DLLs with proper certificate
- Audit all code running in lsass.exe context
- Consider Credential Guard compatibility

---

### Credential Storage Encryption

**Risk:** Stored credentials in LSA secrets must be unbreakable. Any weakness allows offline password attacks.

**Files:**
- `EIDCardLibrary/StoredCredentialManagement.cpp` - LSA secret storage
- `EIDCardLibrary/Package.cpp` - Helper functions

**Current mitigation:**
- Uses SystemFunction007 (RtlEncryptDecrypt) for encryption
- Encryption based on certificate public key
- Debug credential storage to TEMP files disabled (security fix)

**Recommendations:**
- Audit encryption implementation for proper key derivation
- Ensure salt is used and unique per credential
- Verify credential zeroing after use (SecureZeroMemory)
- Test encryption strength against known attacks

---

### Buffer Overflow Risks

**Risk:** Manual buffer manipulation in credential serialization creates overflow vulnerabilities.

**Files:**
- `EIDCardLibrary/Package.cpp` - EIDUnlockLogonPack function
- `EIDAuthenticationPackage/EIDAuthenticationPackage.cpp` - RemapPointer function

**Current mitigation:**
- Buffer overflow checks added (SafeCheckBufferOverflow in Package.cpp:526-535)
- Size validation before copying

**Recommendations:**
- Use `std::span<T>` for all buffer operations
- Apply SAL annotations (_In_, _Out_, _Inout_) consistently
- Enable /GS buffer security check
- Use Address Sanitizer during testing

---

### Smart Card PIN Timing Attacks

**Risk:** Constant-time PIN verification not implemented. Attackers may measure timing to determine valid PIN digits.

**Files:**
- `EIDCardLibrary/CContainer.cpp` - PIN verification
- `EIDCardLibrary/smartcardmodule.cpp` - Smart card module

**Current mitigation:**
- PIN attempt counting enforced by smart card hardware

**Recommendations:**
- Implement constant-time comparison for PIN validation
- Add random delays on authentication failure
- Log repeated PIN failures for security monitoring

---

### CSP Provider Validation

**Risk:** Malicious CSP provider could be loaded for credential theft.

**Files:**
- `EIDCardLibrary/CertificateValidation.cpp:89-94` - IsAllowedCSPProvider check

**Current mitigation:**
- CSP provider allowlist check implemented
- Provider name validated before loading

**Recommendations:**
- Audit allowlist for completeness
- Consider requiring Microsoft Base CSP only
- Validate CSP provider DLL signature before loading

---

### Registry/DLL Hijacking Risks

**Risk:** DLL loading without full path enables DLL hijacking attacks.

**Files:**
- `EIDCardLibrary/Package.cpp` - EIDLoadSystemLibrary function

**Current mitigation:**
- Secure DLL loading implemented (Package.cpp:179-209)
- Full path construction to System32
- Path separator validation

**Recommendations:**
- Mitigation appears adequate
- Add audit logging for all DLL loads
- Consider using SetDefaultDllDirectories

---

## Performance Bottlenecks

### Manual Enumeration Without Caching

**Problem:** NetUserEnum called repeatedly without caching user information.

**Files:**
- `EIDCardLibrary/StoredCredentialManagement.cpp` - GetUsernameFromCertContext, GetCertContextFromHash
- `EIDCardLibrary/Package.cpp` - GetUsernameFromRID

**Cause:** No user credential cache implemented

**Improvement path:**
- Implement LRU cache for user lookups
- Cache invalidated on user change events
- Consider using SAM/LSA lookup APIs directly

---

### Certificate Chain Validation on Every Auth

**Problem:** Full certificate chain validation performed on each authentication attempt.

**Files:**
- `EIDCardLibrary/CertificateValidation.cpp` - ValidateCertificateChain

**Cause:** No caching of validation results

**Improvement path:**
- Cache validation results with certificate hash as key
- Invalidate on certificate store changes
- Consider shorter certificate validity periods

---

### String Copy Inefficiency

**Problem:** Multiple unnecessary string copies in credential serialization.

**Files:**
- `EIDCardLibrary/Package.cpp` - EIDUnlockLogonPack function

**Cause:** Manual buffer management for serialized credentials

**Improvement path:**
- Use move semantics to avoid copies
- Pre-calculate required buffer size once
- Consider std::span for zero-copy views

---

## Fragile Areas

### LSA Package Registration

**Files:**
- `EIDAuthenticationPackage/EIDRundll32Commands.cpp` - DllRegister/DllUnregister

**Why fragile:**
- Registry modification must match exact Windows structure
- Changes to LSA structure in Windows updates break registration
- No rollback mechanism for failed registration

**Safe modification:**
- Always test registration on clean VM
- Implement registration rollback on failure
- Add verification checks after registration
- Document required registry permissions

**Test coverage:**
- No automated tests for registration process
- Manual testing only

---

### Credential Provider COM Registration

**Files:**
- `EIDCredentialProvider/Dll.cpp` - DllGetClassObject

**Why fragile:**
- COM registration must match Windows expectations exactly
- Inproc server registration sensitive to registry structure
- No error recovery for registration failures

**Safe modification:**
- Test COM registration on multiple Windows versions
- Use regsvr32 for registration instead of manual registry writes
- Verify COM object instantiation after registration

**Test coverage:**
- No automated tests for COM registration

---

### Smart Card Reader Detection

**Files:**
- `EIDCardLibrary/CSmartCardNotifier.cpp` - Reader monitoring
- `EIDConfigurationWizard/CContainerHolder.cpp` - Container management

**Why fragile:**
- Hardware-dependent behavior
- Multiple smart card subsystems (WinSCard, CSP, WBio)
- Device insertion/removal race conditions

**Safe modification:**
- Test with multiple reader types
- Handle virtual smart cards (TPM-based)
- Add robust error handling for reader failures
- Implement device change debouncing

**Test coverage:**
- Limited to physical hardware testing
- No virtual reader test coverage

---

### Mixed 32-bit/64-bit Assumptions

**Files:**
- Throughout codebase - pointer truncation warnings (C4311/C4302)

**Why fragile:**
- Assumes 64-bit build in some places
- Mixed size_t/DWORD usage
- Pointer arithmetic for offsets

**Safe modification:**
- Use size_t consistently for buffer sizes
- Use `uintptr_t` for pointer arithmetic
- Enable /W4 as error to catch truncations
- Test both x86 and x64 builds

**Test coverage:**
- x64 only in current configuration
- x86 configuration untested

---

## Scaling Limits

### Concurrent Smart Card Operations

**Current capacity:** Limited to single smart card operation at a time per session

**Limit:** No support for multiple simultaneous smart card operations

**Scaling path:**
- Add per-operation locking granularity
- Implement async smart card operations
- Queue multiple pending operations

---

### Credential Storage Size

**Current capacity:** Limited by LSA secret storage size limits

**Limit:** LSA secrets have size limits (approximately 64KB total)

**Scaling path:**
- Implement credential storage consolidation
- Compress stored credentials
- Move to alternate storage for large credentials

---

### Smart Card Reader Count

**Current capacity:** Enumerates all readers on system

**Limit:** No limit, but performance degrades with many readers

**Scaling path:**
- Implement reader filtering
- Add reader priority/policy
- Cache reader enumeration results

---

## Dependencies at Risk

### Windows SDK Cryptography API

**Risk:** Deprecated CryptoAPI functions still in use. CNG (Cryptography Next Generation) is preferred.

**Files:**
- All files using CryptAcquireContext (80+ occurrences)

**Impact:** CryptoAPI may be removed in future Windows versions

**Migration plan:**
- Phase 1: Introduce CNG alongside CryptoAPI
- Phase 2: Migrate certificate operations to CNG
- Phase 3: Deprecate and remove CryptoAPI usage

---

### CPDK (Cryptographic Provider Development Kit)

**Risk:** External dependency with specific path requirement. Not available via standard package managers.

**Files:**
- `include/cardmod.h` - CPDK header

**Impact:** Build requires manual CPDK installation. Cannot be automated easily.

**Migration plan:**
- Bundle required CPDK headers in repository
- Or use alternative smart card APIs
- Document CPDK requirement clearly in build instructions

---

### Belgian eID SDK

**Risk:** Dependency on specific national ID card SDK limits applicability.

**Files:** Referenced in notes.md but not found in includes

**Impact:** Code may be tied to Belgian eID specifics

**Migration plan:**
- Abstract card-specific functionality
- Support generic smart card standards
- Make Belgian eID support optional/conditional

---

## Missing Critical Features

### Automated Testing Infrastructure

**Problem:** No unit tests, integration tests, or automated tests exist.

**What's missing:**
- Unit tests for all core components
- Integration tests for authentication flow
- Mock implementations for hardware dependencies
- Automated regression tests

**Blocks:**
- Safe refactoring of critical code
- Detection of regressions during modernization
- CI/CD quality gates

**Priority:** HIGH

---

### Code Signing Integration

**Problem:** DLLs are unsigned. LSA protection blocks unsigned code loading.

**What's missing:**
- Code signing certificate integration in build
- Automated signing in build script
- Signature verification in tests

**Blocks:**
- Production deployment on secure systems
- LSA protection enabling
- Windows Hello integration

**Priority:** HIGH (deployment blocker)

---

### Configuration Migration Path

**Problem:** No migration path for configuration settings between versions.

**What's missing:**
- Configuration versioning
- Migration logic for settings changes
- User data preservation during upgrade

**Blocks:**
- Smooth upgrades between versions
- Settings compatibility across releases

**Priority:** MEDIUM

---

### Diagnostic Tools

**Problem:** Limited diagnostic capabilities for troubleshooting authentication failures.

**What's missing:**
- Structured logging with levels
- Event log integration for admin visibility
- Troubleshooting wizard for common issues
- Credential enrollment verification tool

**Blocks:**
- Support for complex authentication issues
- User self-service capabilities

**Priority:** MEDIUM

---

## Test Coverage Gaps

### LSA Authentication Package

**What's not tested:**
- All authentication code paths
- Error handling paths
- LSA package registration/unregistration
- Stored credential CRUD operations

**Files:** `EIDAuthenticationPackage/*.cpp`

**Risk:** LSA bugs can cause system crashes (BSOD)

**Priority:** HIGH

---

### Credential Provider

**What's not tested:**
- Credential enumeration scenarios
- Tile display logic
- User interaction flows
- Error message display

**Files:** `EIDCredentialProvider/*.cpp`

**Risk:** UI bugs can lock users out of system

**Priority:** MEDIUM

---

### Certificate Validation

**What's not tested:**
- Certificate chain validation
- EKU verification
- CRL/OCSP checking
- Trust anchor validation

**Files:** `EIDCardLibrary/CertificateValidation.cpp`

**Risk:** Invalid certificates may be accepted

**Priority:** HIGH (security)

---

### Smart Card Integration

**What's not tested:**
- Multiple reader types
- Virtual smart cards (TPM)
- Card removal scenarios
- PIN retry lockout

**Files:** `EIDCardLibrary/CSmartCardNotifier.cpp`, `EIDCardLibrary/CContainer.cpp`

**Risk:** Hardware-specific failures not detected

**Priority:** MEDIUM

---

### Error Handling Paths

**What's not tested:**
- All error code paths
- Memory allocation failures
- Registry operation failures
- LSA operation failures

**Files:** All components

**Risk:** Unhandled errors cause crashes or security issues

**Priority:** HIGH

---

*Concerns audit: 2026-02-15*
