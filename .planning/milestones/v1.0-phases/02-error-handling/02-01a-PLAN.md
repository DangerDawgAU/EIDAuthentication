---
phase: 02-error-handling
plan: 01a
type: execute
wave: 1
depends_on: []
files_modified:
  - EIDCardLibrary/CertificateUtilities.cpp
  - EIDCardLibrary/CertificateValidation.cpp
autonomous: true
user_setup: []

must_haves:
  truths:
    - "EIDCardLibrary compiles with zero errors from CertificateUtilities.cpp and CertificateValidation.cpp"
    - "All OID string literal to LPSTR conversions use static char arrays"
    - "No string literals are directly assigned to non-const LPSTR pointers"
    - "Certificate-related const-correctness errors resolved (12 of 23 total)"
  artifacts:
    - path: "EIDCardLibrary/CertificateUtilities.cpp"
      provides: "Fixed const-correctness for szOID_* string assignments"
      contains: "static char s_szOid...[]"
    - path: "EIDCardLibrary/CertificateValidation.cpp"
      provides: "Fixed const-correctness for szOID_KP_SMARTCARD_LOGON usage"
      contains: "static char s_szOidSmartCardLogon[]"
  key_links:
    - from: "EIDCardLibrary/CertificateUtilities.cpp"
      to: "Windows SDK header wincrypt.h"
      via: "static char arrays for OID string constants"
      pattern: "static char s_szOid.*\\[\\] = szOID_"
    - from: "EIDCardLibrary/CertificateValidation.cpp"
      to: "Windows SDK header wincrypt.h"
      via: "static char arrays for OID string constants"
      pattern: "static char s_szOid.*\\[\\] = szOID_"
---

<objective>
Fix const-correctness compile errors in CertificateUtilities.cpp and CertificateValidation.cpp to enable C++23 compilation with /Zc:strictStrings flag.

Purpose: The /Zc:strictStrings flag (enabled by default in C++23) enforces strict const-correctness for string literals. String literals are now const char*/const wchar_t* and cannot be implicitly converted to non-const LPSTR/LPWSTR parameters used by Windows APIs. This plan fixes 12 of 23 compile errors in certificate-related files by creating writable static buffers for OID string literals.

Output: CertificateUtilities.cpp and CertificateValidation.cpp compile successfully with C++23, enabling dependent projects to link and allowing std::expected adoption in subsequent plans.
</objective>

<execution_context>
@C:/Users/user/.claude/get-shit-done/workflows/execute-plan.md
@C:/Users/user/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/PROJECT.md
@.planning/ROADMAP.md
@.planning/STATE.md
@.planning/phases/02-error-handling/02-RESEARCH.md
@.planning/phases/01-build-system/01-03-SUMMARY.md

# Locked decisions from STATE.md
- "Incremental modernization - fix compile errors first, then refactor"
- Phase 1 documented 23 compile errors from /Zc:strictStrings requiring fix

# Research patterns to apply
From 02-RESEARCH.md Pitfall 1: String literal to LPSTR/LPWSTR fix pattern:
```cpp
// BEFORE (compile error):
params.rgpszUsageIdentifier[0] = szOID_KP_SMARTCARD_LOGON;  // const char* to LPSTR

// AFTER (fixed):
static char s_szOidSmartCardLogon[] = szOID_KP_SMARTCARD_LOGON;  // Non-const copy
params.rgpszUsageIdentifier[0] = s_szOidSmartCardLogon;
```
</context>

<tasks>

<task type="auto">
  <name>Task 1: Fix CertificateUtilities.cpp const-correctness (10 errors)</name>
  <files>EIDCardLibrary/CertificateUtilities.cpp</files>
  <action>
Fix 10 const-correctness errors in CertificateUtilities.cpp by creating static char arrays for OID string literals.

Lines 620-626: szOID_PKIX_KP_CLIENT_AUTH, szOID_PKIX_KP_SERVER_AUTH, szOID_KP_SMARTCARD_LOGON, szOID_KP_EFS assigned to LPSTR* array element

Fix: Add at file scope before first use (around line 600):
```cpp
// Non-const copies for Windows API compatibility (CERT_ENHKEY_USAGE.rgpszUsageIdentifier is LPSTR*)
static char s_szOidClientAuth[] = szOID_PKIX_KP_CLIENT_AUTH;
static char s_szOidServerAuth[] = szOID_PKIX_KP_SERVER_AUTH;
static char s_szOidSmartCardLogon[] = szOID_KP_SMARTCARD_LOGON;
static char s_szOidEfs[] = szOID_KP_EFS;
```

Then replace assignments at lines 620, 622, 624, 626:
- Line 620: `CertEnhKeyUsage.rgpszUsageIdentifier[...] = s_szOidClientAuth;`
- Line 622: `CertEnhKeyUsage.rgpszUsageIdentifier[...] = s_szOidServerAuth;`
- Line 624: `CertEnhKeyUsage.rgpszUsageIdentifier[...] = s_szOidSmartCardLogon;`
- Line 626: `CertEnhKeyUsage.rgpszUsageIdentifier[...] = s_szOidEfs;`

Lines 40-41: pwszProviderName, pwszContainerName (LPCWSTR) cast to LPWSTR
- These are function parameters being passed to Windows APIs that don't modify
- Already use const_cast (LPWSTR) - no change needed, these are safe casts

AVOID: Do NOT modify API signatures or change Windows SDK structures
  </action>
  <verify>Build EIDCardLibrary and verify CertificateUtilities.cpp produces zero compile errors</verify>
  <done>Line 620-626 assignments compile without const-correctness errors</done>
</task>

<task type="auto">
  <name>Task 2: Fix CertificateValidation.cpp const-correctness (2 errors)</name>
  <files>EIDCardLibrary/CertificateValidation.cpp</files>
  <action>
Fix 2 const-correctness errors in CertificateValidation.cpp for szOID_KP_SMARTCARD_LOGON assignments.

Lines 307-308: szOID_KP_SMARTCARD_LOGON assigned to LPSTR* pointer
Line 448-449: szOID_KP_SMARTCARD_LOGON assigned to LPSTR* pointer

Fix: Add at file scope (after includes, before function definitions):
```cpp
// Non-const copy for Windows API compatibility (params.EnhkeyUsage.rgpszUsageIdentifier is LPSTR*)
static char s_szOidSmartCardLogon[] = szOID_KP_SMARTCARD_LOGON;
```

Replace line 307: `szOid = s_szOidSmartCardLogon;` (note: szOid is local LPSTR variable)
Replace line 448: `szOid = s_szOidSmartCardLogon;` (note: szOid is local LPSTR variable)

Note: Line 235 compares against pCertUsage->rgpszUsageIdentifier[dwI] (const char* comparison with strcmp)
- No fix needed, strcmp accepts const char* parameters
  </action>
  <verify>Build EIDCardLibrary and verify CertificateValidation.cpp produces zero compile errors</verify>
  <done>Lines 307-308, 448-449 compile without const-correctness errors</done>
</task>

</tasks>

<verification>
1. Build EIDCardLibrary project with C++23 (Debug|x64 configuration)
2. Verify zero compile errors from CertificateUtilities.cpp and CertificateValidation.cpp
3. Run `grep -E "static char s_szOid" EIDCardLibrary/CertificateUtilities.cpp EIDCardLibrary/CertificateValidation.cpp` to confirm static array pattern used
4. Verify no string literals directly assigned to non-const LPSTR pointers
</verification>

<success_criteria>
1. CertificateUtilities.cpp compiles with zero errors (10 const-correctness errors resolved)
2. CertificateValidation.cpp compiles with zero errors (2 const-correctness errors resolved)
3. All OID string literal to non-const LPSTR conversions use static array pattern
4. Ready for 02-01b (fix remaining 11 const-correctness errors)
</success_criteria>

<output>
After completion, create `.planning/phases/02-error-handling/02-01a-SUMMARY.md`
</output>
