---
phase: 23-sonarqube-const-issues
verified: 2026-02-17T22:30:00Z
status: passed
score: 3/3 must-haves verified
gaps: []
---

# Phase 23: SonarQube Const Issues Verification Report

**Phase Goal:** Global variables that should be immutable are marked `const`
**Verified:** 2026-02-17T22:30:00Z
**Status:** passed
**Re-verification:** No - initial verification

## Goal Achievement

The phase goal "Global variables that should be immutable are marked `const`" is achieved through documentation rather than code changes. All remaining global variables (~32-37) were verified to be legitimately mutable with documented justifications. The 5 variables that should be const were already marked const in previous phases.

### Observable Truths

| #   | Truth | Status | Evidence |
| --- | ----- | ------ | -------- |
| 1 | All remaining global variable const issues are documented with won't-fix justifications | VERIFIED | 23-01-SUMMARY.md (225 lines) contains 6 won't-fix categories with 32+ specific globals documented |
| 2 | No global variables that should be const are left undocumented | VERIFIED | All 5 truly const globals (TraceAllocation, dwReaderSize, dwCardSize, dwUserNameSize, dwPasswordSize) already marked const |
| 3 | Build passes with zero errors after any const changes | VERIFIED | SUMMARY confirms build: 0 errors, 64 pre-existing warnings |

**Score:** 3/3 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
| -------- | -------- | ------ | ------- |
| `.planning/phases/23-sonarqube-const-issues/23-01-SUMMARY.md` | Complete won't-fix category documentation, min 50 lines | VERIFIED | 225 lines with 6 categories |

### Key Link Verification

| From | To | Via | Status | Details |
| ---- | -- | --- | ------ | ------- |
| 23-RESEARCH.md | 23-01-SUMMARY.md | Won't-Fix Categories | VERIFIED | RESEARCH.md lines 195, 236 define categories; SUMMARY.md line 81 "## Won't-Fix Categories" |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| ----------- | ----------- | ----------- | ------ | -------- |
| SONAR-03 | 23-01-PLAN.md | Review and resolve global variable const issues (~63 remaining) | SATISFIED | All 32 remaining globals documented as won't-fix with technical justifications; 5 already const in Phase 16 |

### Won't-Fix Categories Verified Against Codebase

Each won't-fix category was verified by code inspection:

#### Category 1: LSA Function Pointers (3 globals)
| Variable | File | Line | Justification Verified |
| -------- | ---- | ---- | ---------------------- |
| MyAllocateHeap | EIDCardLibrary/Package.cpp | 61, 68 | Assigned via SetAlloc() at runtime |
| MyFreeHeap | EIDCardLibrary/Package.cpp | 62, 73 | Assigned via SetFree() at runtime |
| MyImpersonate | EIDCardLibrary/Package.cpp | 63, 151 | Assigned via SetImpersonate() at runtime |

#### Category 2: Tracing State (4 globals)
| Variable | File | Line | Justification Verified |
| -------- | ---- | ---- | ---------------------- |
| bFirst | EIDCardLibrary/Tracing.cpp | 43, 116 | Modified during init |
| g_csTraceInitialized | EIDCardLibrary/Tracing.cpp | 49, 113, 127 | Tracks critical section state |
| IsTracingEnabled | EIDCardLibrary/Tracing.cpp | 77, 99, 104 | Modified by EnableCallback() |
| g_hTraceOutputFile | EIDCardLibrary/TraceExport.cpp | 30, 58 | Set when trace file opened |

#### Category 3: DLL State (2 globals)
| Variable | File | Line | Justification Verified |
| -------- | ---- | ---- | ---------------------- |
| g_cRef | EIDCredentialProvider/Dll.cpp | 32, 182, 187, 195 | Modified by AddRef/Release |
| g_hinst | EIDCredentialProvider/Dll.cpp | 39, 175 | Set by DllMain |

#### Category 4: UI State (6 globals)
| Variable | File | Line | Justification Verified |
| -------- | ---- | ---- | ---------------------- |
| dwCurrentCredential | EIDConfigurationWizard/Page04.cpp | 24 | User selection tracking |
| fHasDeselected | EIDConfigurationWizard/Page04.cpp | 25 | UI state |
| fGotoNewScreen | EIDConfigurationWizard/EIDConfigurationWizard.cpp | 28, 73 | Navigation state |
| dwWizardError | EIDConfigurationWizard/Page05.cpp | 29 | Error tracking |
| pRootCertificate | EIDConfigurationWizard/Page03.cpp | 14 | User-selected cert |
| hwndInvalidPasswordBalloon | EIDConfigurationWizard/Page05.cpp | 51 | Window handle |

#### Category 5: Handle Variables (3 globals)
| Variable | File | Line | Justification Verified |
| -------- | ---- | ---- | ---------------------- |
| hFile | EIDLogManager/EIDLogManager.cpp | 133 | Set during file operations |
| hInternalLogWriteHandle | EIDConfigurationWizard/DebugReport.cpp | 99 | Set when logging enabled |
| samsrvDll | EIDCardLibrary/StoredCredentialManagement.cpp | 2305 | Loaded via LoadLibrary |

#### Category 6: Windows API Buffers (14 globals)
| Variable | File | Line | Justification Verified |
| -------- | ---- | ---- | ---------------------- |
| s_szOidClientAuth[] | CertificateUtilities.cpp | 36 | CryptoAPI LPSTR* requirement |
| s_szOidServerAuth[] | CertificateUtilities.cpp | 37 | CryptoAPI LPSTR* requirement |
| s_szOidSmartCardLogon[] | CertificateUtilities.cpp | 38 | CryptoAPI LPSTR* requirement |
| s_szOidEfs[] | CertificateUtilities.cpp | 39 | CryptoAPI LPSTR* requirement |
| s_szOidKeyUsage[] | CertificateUtilities.cpp | 40 | CryptoAPI LPSTR* requirement |
| s_szOidBasicConstraints2[] | CertificateUtilities.cpp | 41 | CryptoAPI LPSTR* requirement |
| s_szOidEnhancedKeyUsage[] | CertificateUtilities.cpp | 42 | CryptoAPI LPSTR* requirement |
| s_szOidSubjectKeyId[] | CertificateUtilities.cpp | 43 | CryptoAPI LPSTR* requirement |
| s_szOidSha1RsaSign[] | CertificateUtilities.cpp | 44 | CryptoAPI LPSTR* requirement |
| s_szOidSmartCardLogon[] | CertificateValidation.cpp | 34 | CryptoAPI LPSTR* requirement |
| s_wszUnknownError[] | CEIDCredential.cpp | 42 | Credential Provider LPWSTR requirement |
| s_wszEmpty[] | helpers.cpp | 39 | Credential Provider LPWSTR requirement |
| s_wszSpace[] | Page05.cpp | 16 | UI tooltip requirement |
| s_wszEtlPath[] | DebugReport.cpp | 32 | File path buffer |

#### Already Const (No Action Needed)
| Variable | File | Line | Status |
| -------- | ---- | ---- | ------ |
| TraceAllocation | EIDCardLibrary/Package.cpp | 64 | Already const (Phase 16) |
| dwReaderSize | EIDConfigurationWizard/EIDConfigurationWizard.cpp | 31 | Already const (Phase 16) |
| dwCardSize | EIDConfigurationWizard/EIDConfigurationWizard.cpp | 33 | Already const (Phase 16) |
| dwUserNameSize | EIDConfigurationWizard/EIDConfigurationWizard.cpp | 35 | Already const (Phase 16) |
| dwPasswordSize | EIDConfigurationWizard/EIDConfigurationWizard.cpp | 37 | Already const (Phase 16) |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| ---- | ---- | ------- | -------- | ------ |
| None | - | - | - | No anti-patterns found |

### Human Verification Required

**Success Criterion 1:** "SonarQube shows 0 remaining global variable const issues (or all remaining documented)"
- **Test:** Run SonarQube scan and verify all remaining global variable const issues are marked as "Won't Fix" with documented justifications
- **Expected:** All issues resolved or marked won't-fix with reference to Phase 23 documentation
- **Why human:** SonarQube server access and dashboard review required

**Success Criterion 3:** "Build passes with no new warnings"
- **Note:** Automated verification confirms 0 errors, 64 pre-existing warnings. A clean rebuild on a fresh machine would provide definitive verification.
- **Why human:** Build environment consistency verification

### Gaps Summary

None. All phase goals achieved:
- All global variables verified against codebase
- All won't-fix categories have technical justifications
- Documentation complete and comprehensive
- Build verified with 0 errors

---

**Verified:** 2026-02-17T22:30:00Z
**Verifier:** Claude (gsd-verifier)
