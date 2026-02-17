# Phase 28: Diagnostics & Logging - Research

**Researched:** 2026-02-18
**Domain:** Enhanced error messaging, tracing coverage, and structured logging for Windows security-critical code
**Confidence:** HIGH (verified with Microsoft Learn, existing codebase patterns, Windows API documentation)

## Summary

This phase focuses on enhancing diagnostic capabilities in the EIDAuthentication codebase through three improvements: more contextual error messages, improved tracing coverage for key code paths, and structured logging where feasible. The existing infrastructure already provides a solid foundation with ETW-based tracing (`Tracing.cpp`), security audit logging, and `std::expected<T, E>` for internal error handling.

**Primary recommendation:** Incremental enhancements to existing infrastructure - no new frameworks needed. Focus on (1) adding context to existing error messages, (2) adding tracing to uninstrumented key paths, and (3) using existing ETW patterns with better fielded data.

<phase_requirements>

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| DIAG-01 | Enhance error messages with more context | Add operation context, HRESULT translation, and relevant state to error traces. Use existing `LookUpErrorMessage()` and `EIDCardLibraryTrace()` patterns. |
| DIAG-02 | Improve tracing coverage for key code paths | Add tracing to authentication flow, credential validation, and smart card operations that lack coverage. Use existing `EIDCardLibraryTrace()` macro pattern. |
| DIAG-03 | Add structured logging where feasible | Use existing ETW infrastructure (`EventWriteString`, `EIDSecurityAudit`). Consider `CaptureStackBackTrace` for error path diagnostics. LSASS-safe - no dynamic allocation in hot paths. |

</phase_requirements>

## Standard Stack

### Core (Existing Infrastructure)

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| ETW (Event Tracing for Windows) | Windows 7+ | Primary logging mechanism | Already implemented, Windows-native, LSASS-safe |
| `EventWriteString` | Win32 API | Simple ETW event writing | Used in `Tracing.cpp`, no manifest required |
| `OutputDebugString` | Win32 API | Debug output | Available, used in debug builds |
| `CaptureStackBackTrace` | Win32 API | Stack trace capture | Recommended by Phase 27 research for LSASS context |

### Supporting (Error Context)

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `FormatMessage` | Win32 API | Error code to string | Already used in `LookUpErrorMessage()` |
| `std::expected<T, HRESULT>` | C++23 | Internal error handling | Already adopted, provides error context |
| `GetLastError()` / `HRESULT_FROM_WIN32` | Win32 API | Error code retrieval | Existing patterns, extend with context |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `std::stacktrace` | `CaptureStackBackTrace` | std::stacktrace buggy in MSVC; Win32 API is reliable |
| TraceLogging | Existing `EventWriteString` | TraceLogging requires more changes; current ETW works |
| Custom logging framework | Extend existing `Tracing.cpp` | No need for new framework; incremental enhancement |
| Logging to file | ETW only | File I/O risky in LSASS; ETW is safer |

## Architecture Patterns

### Current Pattern: EIDCardLibraryTrace Macro (KEEP/EXTEND)

```cpp
// Source: EIDCardLibrary/Tracing.h
#define EIDCardLibraryTrace(dwLevel, ...) \
    EIDCardLibraryTraceEx(__FILE__,__LINE__,__FUNCTION__, dwLevel, __VA_ARGS__)
```

**Why this is the right pattern:**
- Captures file, line, function automatically
- ETW integration via `EventWriteString`
- Works in LSASS (no dynamic allocation in the macro itself)
- Debug builds get `OutputDebugString` for kernel debugger visibility

### Current Pattern: Security Audit Logging (EXTEND)

```cpp
// Source: EIDCardLibrary/Tracing.h
constexpr UCHAR SECURITY_AUDIT_SUCCESS  = 0;
constexpr UCHAR SECURITY_AUDIT_FAILURE  = 1;
constexpr UCHAR SECURITY_AUDIT_WARNING  = 2;

#define EIDSecurityAudit(dwAuditType, ...) \
    EIDSecurityAuditEx(__FILE__,__LINE__,__FUNCTION__, dwAuditType, __VA_ARGS__)
```

**Why extend this pattern:**
- Already provides structured event types
- Used for security-relevant events
- Logged at ERROR/CRITICAL level for visibility
- Easy to filter in SIEM tools

### Pattern: Enhanced Error Messages

```cpp
// CURRENT: Minimal context
EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING, L"not SUCCEEDED hr=0x%08x", hr);

// IMPROVED: With operation context
EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,
    L"GetCredentialState failed: hr=0x%08x, credential_index=%u, provider_state=%d",
    hr, credIndex, providerState);
```

**Why this approach:**
- No new infrastructure needed
- Same LSASS safety guarantees
- Better post-mortem debugging
- Easy to implement incrementally

### Pattern: Stack Trace on Error Paths (NEW)

```cpp
// Source: Windows SDK documentation, Phase 27 research
#include <windows.h>
#include <dbghelp.h>

#pragma comment(lib, "dbghelp.lib")

// LSASS-safe stack trace capture - stack allocated buffer
constexpr USHORT MAX_STACK_FRAMES = 32;

void LogErrorWithContext(const char* operation, HRESULT hr) noexcept {
    PVOID stack[MAX_STACK_FRAMES];

    EIDCardLibraryTrace(WINEVENT_LEVEL_ERROR,
        L"Operation '%S' failed: hr=0x%08X", operation, hr);

    // Optional: Capture stack for post-mortem analysis
    USHORT frameCount = CaptureStackBackTrace(
        2,  // Skip this function and caller
        MAX_STACK_FRAMES,
        stack,
        nullptr
    );

    // Log raw addresses - symbol resolution done offline
    for (USHORT i = 0; i < frameCount && i < 8; ++i) {
        EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,
            L"  Stack[%u]: 0x%p", i, stack[i]);
    }
}
```

**Why this approach:**
- `CaptureStackBackTrace` is reliable in LSASS (Phase 27 recommendation)
- Stack-allocated buffer (no dynamic memory)
- Raw addresses logged for offline symbol resolution
- Only enabled in error paths (not hot paths)

### Anti-Patterns to Avoid

- **Logging sensitive data:** Never log PINs, passwords, encryption keys, or card data
- **Dynamic allocation in logging:** Avoid `std::string`, `new`, `malloc` in LSASS context
- **Excessive tracing in hot paths:** Auth validation runs frequently; limit trace volume
- **Exceptions for error flow:** LSASS prohibits exceptions; use `std::expected` or HRESULT
- **Console I/O:** LSASS has no console; avoid `std::print`, `printf`, `std::cout`

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Error message formatting | Custom string builder | `swprintf_s` with stack buffer | Stack-safe, existing pattern |
| Stack trace capture | Custom stack walker | `CaptureStackBackTrace` Win32 API | Windows-native, reliable, LSASS-safe |
| ETW event writing | Custom logging framework | Existing `EIDCardLibraryTrace` | Already integrated, working |
| Error code translation | Custom HRESULT lookup | `FormatMessage`, `LookUpErrorMessage` | Windows standard, existing code |
| Audit logging | Separate audit system | `EIDSecurityAudit` macro | Already implemented, ETW-integrated |

**Key insight:** The existing `Tracing.cpp` infrastructure is well-designed for LSASS context. Enhance it incrementally rather than replacing it.

## Common Pitfalls

### Pitfall 1: Logging Sensitive Data in LSASS
**What goes wrong:** Accidentally logging PINs, passwords, or crypto keys exposes credentials
**Why it happens:** Convenience during debugging, forgotten cleanup
**How to avoid:** Code review checklist: never pass sensitive buffers to trace functions; use `SecureZeroMemory` after use
**Warning signs:** Format strings containing "pin", "password", "key", "secret"

### Pitfall 2: Dynamic Memory in Logging Path
**What goes wrong:** Heap allocation in LSASS can cause crashes or instability
**Why it happens:** Using `std::string`, `std::format` (which allocates), or `new`
**How to avoid:** Use stack-allocated `WCHAR` buffers with `swprintf_s`; limit message length
**Warning signs:** `std::string` in trace function signatures, calls to `new`/`malloc`

### Pitfall 3: Excessive Tracing in Authentication Hot Path
**What goes wrong:** Too much ETW overhead impacts login performance
**Why it happens:** Tracing every step of every operation
**How to avoid:** Use appropriate log levels (VERBOSE for detailed, INFO for milestones, WARNING/ERROR for problems)
**Warning signs:** `WINEVENT_LEVEL_VERBOSE` in high-frequency code paths

### Pitfall 4: Unstructured Error Messages
**What goes wrong:** "Operation failed" with no context makes debugging impossible
**Why it happens:** Quick debugging statements that become permanent
**How to avoid:** Always include: operation name, error code, relevant state values
**Warning signs:** Messages like "not SUCCEEDED", "error", "failed" without details

### Pitfall 5: Missing Error Path Tracing
**What goes wrong:** Success paths are traced, but error paths are silent
**Why it happens:** Focus on happy path, error handling added later
**How to avoid:** Every error return path should have a trace with context
**Warning signs:** `return E_FAIL;` or `return nullptr;` without prior trace

## Code Examples

### Enhanced Error Message Pattern

```cpp
// Source: Pattern from EIDCardLibrary/CertificateValidation.cpp
// CURRENT: Good context example
EIDCardLibraryTrace(WINEVENT_LEVEL_ERROR,
    L"GetCertificateFromCspInfoInternal: Invalid CSP info offset - "
    L"dwCspInfoLen=%u, nContainerNameOffset=%u, nCSPNameOffset=%u",
    pCspInfo->dwCspInfoLen,
    pCspInfo->nContainerNameOffset,
    pCspInfo->nCSPNameOffset);

// IMPROVED: Even better with operation context and HRESULT mapping
void LogCspError(const char* operation, HRESULT hr, LPCWSTR providerName) noexcept {
    WCHAR errorMsg[256] = {0};
    LookUpErrorMessage(errorMsg, ARRAYSIZE(errorMsg), HRESULT_CODE(hr));

    EIDCardLibraryTrace(WINEVENT_LEVEL_ERROR,
        L"CSP '%s' operation '%S' failed: hr=0x%08X (%s)",
        providerName ? providerName : L"(null)",
        operation,
        hr,
        errorMsg);
}
```

### Stack Trace Capture Pattern (LSASS-Safe)

```cpp
// Source: Windows SDK, Phase 27 research recommendation
// Stack-allocated, noexcept, suitable for LSASS error paths

#include <windows.h>
#include <dbghelp.h>

#pragma comment(lib, "dbghelp.lib")

class DiagnosticHelper {
public:
    // Capture and log stack trace for error diagnostics
    // Uses stack allocation only - LSASS safe
    static void LogStackTraceWithContext(
        const char* operation,
        HRESULT error,
        UCHAR level = WINEVENT_LEVEL_ERROR) noexcept
    {
        // Log the error with context
        EIDCardLibraryTrace(level,
            L"Diagnostic: Operation '%S' error 0x%08X", operation, error);

        // Stack-allocated buffer for addresses
        constexpr USHORT MAX_FRAMES = 16;
        PVOID stack[MAX_FRAMES];

        USHORT frames = CaptureStackBackTrace(
            2,  // Skip this function and immediate caller
            MAX_FRAMES,
            stack,
            nullptr
        );

        // Log frames at VERBOSE level (only visible when tracing enabled)
        for (USHORT i = 0; i < frames; ++i) {
            EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,
                L"  frame[%u] = 0x%p", i, stack[i]);
        }
    }
};
```

### Improved Tracing Coverage Pattern

```cpp
// Source: Pattern for key code paths
// Add entry/exit tracing to important functions

// BEFORE: No tracing
HRESULT ValidateCertificate(PCCERT_CONTEXT pCert) {
    if (!pCert) return E_POINTER;
    // ... validation logic
    return hr;
}

// AFTER: Entry/exit with context
HRESULT ValidateCertificate(PCCERT_CONTEXT pCert) {
    EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,
        L"ValidateCertificate: entry, cert=%p", pCert);

    if (!pCert) {
        EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,
            L"ValidateCertificate: null certificate");
        return E_POINTER;
    }

    // ... validation logic

    if (FAILED(hr)) {
        EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,
            L"ValidateCertificate: failed hr=0x%08X, subject='%s'",
            hr, GetCertSubject(pCert));
    } else {
        EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,
            L"ValidateCertificate: success");
    }
    return hr;
}
```

### Structured Security Audit Pattern

```cpp
// Source: EIDCardLibrary/Tracing.cpp - existing pattern to extend
// Security audits provide structured, filterable events

// CURRENT: Good structured audit
EIDSecurityAudit(SECURITY_AUDIT_FAILURE,
    L"Smart card logon failed for user '%wZ': Wrong PIN", *AccountName);

// EXTENDED: With additional structured fields
// Note: Keep messages filterable by type prefix
EIDSecurityAudit(SECURITY_AUDIT_FAILURE,
    L"[AUTH_PIN_ERROR] User='%wZ' Error='WRONG_PIN' AttemptsRemaining=%d",
    *AccountName, attemptsRemaining);

// Structured field examples:
// - [AUTH_CERT_ERROR] for certificate validation failures
// - [AUTH_CARD_ERROR] for smart card communication errors
// - [AUTH_POLICY_ERROR] for policy violations
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `printf` debugging | ETW tracing with levels | Original design | Production-safe logging |
| Generic error codes | `std::expected<T, HRESULT>` | v1.0 (2026-02-16) | Type-safe error handling |
| Ad-hoc error messages | Security audit logging | v1.0 security fixes | SIEM-compatible events |
| `std::stacktrace` | `CaptureStackBackTrace` | Phase 27 (2026-02-18) | Reliable stack capture |

**Deprecated/outdated:**
- Do not use `std::stacktrace` - buggy in MSVC (Phase 27 finding)
- Do not use `std::print`/`std::println` - no console in LSASS
- Do not use file-based logging in LSASS - I/O risk

## Key Code Paths Needing Tracing Coverage

Based on codebase analysis, these areas would benefit from enhanced tracing:

| Component | File | Priority | Recommended Enhancement |
|-----------|------|----------|------------------------|
| Authentication | `EIDAuthenticationPackage.cpp` | HIGH | Add context to PIN errors, certificate errors |
| Credential Provider | `CEIDCredential.cpp` | HIGH | Add context to "not SUCCEEDED" traces |
| Certificate Validation | `CertificateValidation.cpp` | MEDIUM | Add CSP provider context to failures |
| Token Creation | `CompleteToken.cpp` | MEDIUM | Add user/SID context to failures |
| Credential Management | `CredentialManagement.cpp` | MEDIUM | Add operation context to error paths |
| Smart Card Module | `smartcardmodule.cpp` | LOW | Add reader/card context where safe |

## LSASS Safety Checklist

For all diagnostics changes, verify:

- [ ] No dynamic memory allocation (`new`, `malloc`, `std::string`)
- [ ] Stack-allocated buffers only
- [ ] `noexcept` on all logging functions
- [ ] No sensitive data (PIN, password, key) in trace output
- [ ] No console I/O (`printf`, `std::cout`, `std::print`)
- [ ] No exceptions thrown
- [ ] Bounded string operations (`swprintf_s`, `_vsnwprintf_s`)

## Open Questions

1. **Should we add TraceLogging for more structured events?**
   - What we know: TraceLogging provides typed fields without manifests
   - What's unclear: Migration effort from `EventWriteString`
   - Recommendation: Defer to future phase; current ETW adequate for DIAG-01/02/03

2. **What tracing level should stack traces use?**
   - What we know: VERBOSE is only captured when tracing explicitly enabled
   - What's unclear: Whether ERROR level is more appropriate for error path stacks
   - Recommendation: Use VERBOSE for stack frames, ERROR for error context message

3. **Should we add timing/telemetry to auth operations?**
   - What we know: Performance data useful for debugging slow logins
   - What's unclear: ETW already provides timestamps; additional overhead
   - Recommendation: Defer to future phase; ETW timestamps sufficient for now

## Sources

### Primary (HIGH confidence)
- `.planning/phases/27-cpp23-advanced-features/27-RESEARCH.md` - CaptureStackBackTrace recommendation
- `.planning/research/FEATURES.md` - C++23 feature status and LSASS constraints
- `EIDCardLibrary/Tracing.cpp` - Existing implementation patterns
- `EIDCardLibrary/Tracing.h` - API documentation and level constants

### Secondary (MEDIUM confidence)
- `EIDCardLibrary/ErrorHandling.h` - Result<T> and error handling patterns
- `EIDAuthenticationPackage/EIDAuthenticationPackage.cpp` - Security audit examples
- Microsoft Learn - CaptureStackBackTrace API documentation

### Tertiary (LOW confidence - validation needed)
- None - all critical findings verified with codebase and existing research

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - Based on existing codebase infrastructure
- Architecture: HIGH - Patterns already established in codebase
- Pitfalls: HIGH - Derived from LSASS constraints and existing research

**Research date:** 2026-02-18
**Valid until:** 6 months - LSASS constraints are stable; no major Windows changes expected

---

## RESEARCH COMPLETE

**Phase:** 28 - Diagnostics & Logging
**Confidence:** HIGH

### Key Findings

1. **Existing infrastructure is solid** - ETW tracing (`Tracing.cpp`), security audit logging, and `std::expected<T, HRESULT>` provide good foundation; enhance incrementally

2. **CaptureStackBackTrace for error paths** - Phase 27 recommended this over buggy `std::stacktrace`; use stack-allocated buffers for LSASS safety

3. **Focus on context, not new frameworks** - DIAG-01/02/03 can be achieved by improving existing error messages with operation context, state values, and structured prefixes

4. **Key areas for enhancement** - "not SUCCEEDED hr=0x%08x" messages in CEIDCredential.cpp, authentication error paths in EIDAuthenticationPackage.cpp

5. **LSASS safety is paramount** - No dynamic allocation, no sensitive data logging, stack-allocated buffers only

### File Created
`.planning/phases/28-diagnostics-logging/28-RESEARCH.md`

### Confidence Assessment
| Area | Level | Reason |
|------|-------|--------|
| Standard Stack | HIGH | Based on existing working infrastructure in codebase |
| Architecture | HIGH | Patterns already established and proven in LSASS context |
| Pitfalls | HIGH | Derived from documented LSASS constraints and Phase 27 research |

### Open Questions
- TraceLogging migration (defer - current ETW adequate)
- Stack trace log level (recommend VERBOSE for frames)
- Performance telemetry (defer - ETW timestamps sufficient)

### Ready for Planning
Research complete. Planner can create PLAN.md files addressing:
1. DIAG-01: Enhanced error messages with context (add operation, state, error translation)
2. DIAG-02: Improved tracing coverage (add to uninstrumented key paths)
3. DIAG-03: Structured logging (extend existing EIDSecurityAudit with structured prefixes)
