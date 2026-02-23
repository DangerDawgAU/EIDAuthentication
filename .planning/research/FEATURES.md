# Feature Research

**Domain:** Windows Smart Card Authentication - UI/UX Enhancement (v1.7)
**Researched:** 2026-02-24
**Confidence:** HIGH

---

## Executive Summary

This research covers the feature landscape for the v1.7 UI/UX Enhancement milestone of EIDAuthentication, a Windows smart card authentication package. The milestone focuses on three specific improvements:

1. **UIUX-01:** Remove P12 import option from Configure Smart Card window
2. **UIUX-02:** Add modal progress popup during card flashing operation
3. **UIUX-03:** Expand Selected Authority info box with additional certificate fields

**Key Findings:**
1. **Progress indication is table stakes** - Windows smart card operations can take 2-30+ seconds; users expect visual feedback. Current implementation freezes the UI, which is perceived as an application hang.
2. **Certificate info display is standard** - Windows certificate dialogs (CryptUIDlgViewCertificate) show Subject, Issuer, Validity, Serial Number, and Thumbprint. Current implementation shows only 3 fields.
3. **P12 import is legacy/deprecated** - For local account smart card authentication on non-domain machines, P12 import is rarely needed. Removing it simplifies the UI.

---

## Feature Landscape

### Table Stakes (Users Expect These)

Features users assume exist. Missing these = product feels incomplete.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| **Progress indication during card operations** | Smart card operations take 2-30+ seconds; users expect visual feedback to know the system is working | MEDIUM | Currently causes UI freeze. Windows smart card unlock can take up to 30 seconds (Microsoft KB). Users accustomed to Windows "Please wait..." spinners and progress dialogs. |
| **Clear certificate information display** | Security software must show what certificate is being used for authentication | LOW | Standard Windows certificate dialog shows: Subject, Issuer, Validity dates, Serial Number, Thumbprint. Current implementation shows only Object, Delivered, Expires. |
| **Responsive UI during operations** | Modern Windows applications do not freeze the UI thread during I/O | HIGH | Current implementation blocks wizard UI during CreateCertificate, ClearCard, and ImportFileToSmartCard operations. |
| **Consistent UI options** | Users expect all visible options to be meaningful and functional | LOW | P12 import option exists but is rarely needed for target use case (local account smart card auth on non-domain machines). |

### Differentiators (Competitive Advantage)

Features that set the product apart. Not required, but valuable.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| **Enhanced certificate info panel** | Shows more certificate details (Issuer, Serial Number, Key Size, Fingerprint, Validity) directly in wizard without opening separate dialog | MEDIUM | Current: 3 fields (Object, Delivered, Expires). Enhanced: 6+ fields. Improves user confidence by showing exact certificate being used. |
| **Modal progress popup with cancel** | Non-blocking progress indicator with animated marquee and optional cancellation | MEDIUM | Better UX than frozen UI. Must use worker thread for card operations. Windows Credential Provider pattern: use ICredentialProviderCredentialEvents::SetFieldString for status updates in login screen. |
| **Real-time validity warning** | Warns when certificate validity would exceed CA expiry before user attempts creation | LOW | Already partially implemented (IDC_03VALIDITYWARNING). Could be enhanced with more visible styling. |

### Anti-Features (Commonly Requested, Often Problematic)

Features that seem good but create problems.

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| **Animated progress bar with percentage** | Users want to see exact progress | Smart card operations have unpredictable timing; percentage estimates are misleading and cause frustration when inaccurate | Use marquee/indeterminate progress animation with status text |
| **Auto-dismiss progress on completion** | Cleaner UI flow | Users may miss success/failure state; no confirmation that operation completed | Keep dialog visible with result message, require user acknowledgment |
| **Background card operations without modal** | User can interact with other controls | Creates confusing state where user can modify inputs while operation in progress; race conditions possible | Block wizard navigation (PropSheet_SetWizButtons) but show progress popup |

---

## Feature Dependencies

```
[Modal Progress Popup]
    └──requires──> [Worker thread for card operations]
                       └──requires──> [Thread-safe callback to UI]

[Enhanced Certificate Panel]
    └──requires──> [Certificate property extraction]
                       └──uses──> [Windows CryptoAPI: CertGetNameString, CertGetCertificateContextProperty]

[Remove P12 Import]
    └──requires──> [UI layout adjustment]
    └──conflicts──> [Existing P12 import code paths]
```

### Dependency Notes

- **Modal Progress Popup requires Worker Thread:** Card operations (ClearCard, CreateCertificate, ImportFileToSmartCard) are blocking Win32 CryptoAPI calls. Must move to worker thread to keep UI responsive.
- **Worker Thread requires Thread-safe Callback:** Windows UI requires UI updates on main thread. Use PostMessage or ICredentialProviderCredentialEvents from worker thread.
- **Enhanced Panel uses CryptoAPI:** Already have CertGetNameString for Subject. Need to add: Issuer (CertGetNameString with CERT_NAME_SIMPLE_DISPLAY_TYPE on issuer chain), Serial Number (pCertInfo->SerialNumber), Thumbprint (CertGetCertificateContextProperty with CERT_HASH_PROP_ID), Key Size (CryptAcquireCertificatePrivateKey + CryptGetKeyParam).

---

## MVP Definition

### Launch With (v1.7)

Minimum viable product for this milestone.

- [ ] **UIUX-01: Remove P12 import option** -- Simplifies UI, removes unused feature for target use case (local accounts). LOW complexity. Simply hide/disable controls in IDD_03NEW dialog and update radio button logic.

- [ ] **UIUX-02: Add modal progress popup** -- Prevents perceived application freeze during card flashing (CreateCertificate, ClearCard). MEDIUM complexity. Requires:
  - New dialog resource with progress control (marquee style)
  - Worker thread for blocking card operations
  - Window message callback to update/close progress

- [ ] **UIUX-03: Expand Selected Authority info box** -- Improves user confidence by showing more certificate details. MEDIUM complexity. Requires:
  - Expand IDC_03CERTIFICATEPANEL ListBox to accommodate more lines
  - Add certificate property extraction: Issuer, Serial Number, Key Size, Fingerprint (SHA-256)
  - Update UpdateCertificatePanel() function in EIDConfigurationWizardPage03.cpp

### Add After Validation (v1.x)

Features to add once core is working.

- [ ] **Cancel button on progress popup** -- Allow user to abort long-running operations. Requires thread cancellation signaling.

### Future Consideration (v2+)

Features to defer until product-market fit is established.

- [ ] **Progress stages text** -- Show current operation stage ("Reading card...", "Generating key...", "Writing certificate..."). Requires refactoring card operations into discrete steps with callbacks.

---

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| Remove P12 import | LOW (cleanup) | LOW | P1 |
| Modal progress popup | HIGH (fixes freeze) | MEDIUM | P1 |
| Expanded certificate info | MEDIUM (improves confidence) | MEDIUM | P1 |
| Cancel button | MEDIUM (convenience) | MEDIUM | P2 |
| Progress stages text | LOW (nice to have) | HIGH | P3 |

**Priority key:**
- P1: Must have for this milestone
- P2: Should have, add when possible
- P3: Nice to have, future consideration

---

## Competitor Feature Analysis

| Feature | Windows Built-in Smart Card CP | Third-party Smart Card Software | Our Approach |
|---------|-------------------------------|--------------------------------|--------------|
| Progress indication | Uses LogonUI's built-in progress animation | Modal dialogs with marquee progress | Modal popup during wizard operations |
| Certificate display | Opens full Windows certificate dialog (CryptUIDlgViewCertificate) | Varies; some inline, some dialogs | Inline panel summary + "Show" button for full details (already have this pattern) |
| Operation blocking | Non-blocking in LogonUI context | Varies; best practice is worker thread | Currently blocking; need to fix |

---

## Windows Smart Card UI Conventions

Based on Microsoft documentation and Windows built-in behavior:

### Certificate Property Display

| Property | Windows API | How to Extract | Display Format |
|----------|-------------|----------------|----------------|
| Subject (Object) | CertGetNameString(CERT_NAME_SIMPLE_DISPLAY_TYPE) | Already implemented | "Object: CN=ComputerName" |
| Issuer | CertGetNameString on pCertInfo->Issuer | Add to UpdateCertificatePanel | "Issuer: CN=CA-Name" |
| Delivered (NotBefore) | FileTimeToSystemTime on pCertInfo->NotBefore | Already implemented | Locale-formatted date/time |
| Expires (NotAfter) | FileTimeToSystemTime on pCertInfo->NotAfter | Already implemented | Locale-formatted date/time |
| Serial Number | pCertInfo->SerialNumber (CRYPT_INTEGER_BLOB) | Add hex formatting | "Serial: 00 11 22 33..." |
| Thumbprint/Fingerprint | CertGetCertificateContextProperty(CERT_HASH_PROP_ID) | Add SHA-256 extraction | "Fingerprint: AABBCCDD..." |
| Key Size | CryptGetKeyParam(KP_KEYLEN) after CryptAcquireCertificatePrivateKey | Add extraction | "Key Size: 2048 bits" |

### Progress Dialog Behavior

Based on Windows Credential Provider patterns:

1. **Use ICredentialProviderCredentialEvents::SetFieldString** for status updates in login context
2. **Use DialogBox with progress control** for wizard/configuration context
3. **Marquee style (PBS_MARQUEE)** preferred over determinate progress - smart card timing is unpredictable
4. **Must not block UI thread** - operations can take 2-30+ seconds
5. **Show completion state** before closing - user needs confirmation

### Modal Dialog in Credential Provider Context

Special considerations for LogonUI/Winlogon context:

- Use `ICredentialProviderCredentialEvents::OnCreatingWindow` to get parent HWND
- Cannot create arbitrary windows; must work within LogonUI's window hierarchy
- Progress updates via `SetFieldString` on a text field
- Our project: Configuration Wizard runs as standalone app, so standard Win32 dialogs are fine

---

## Existing Code References

| Component | Location | Purpose |
|-----------|----------|---------|
| UpdateCertificatePanel() | EIDConfigurationWizardPage03.cpp:160-194 | Updates IDC_03CERTIFICATEPANEL ListBox with certificate info |
| IDC_03CERTIFICATEPANEL | EIDConfigurationWizard.rc:142 | ListBox showing current cert info (Object, Delivered, Expires) |
| CreateSmartCardCertificate() | EIDConfigurationWizardPage03.cpp:128-158 | Blocking operation that creates cert on smart card |
| ClearCard() | EIDCardLibrary/CertificateUtilities.cpp:1149 | Blocking operation that clears smart card |
| ViewCertificate() | EIDCardLibrary/CContainer.cpp:236-268 | Opens CryptUIDlgViewCertificate dialog (already implemented) |
| IDD_03NEW dialog | EIDConfigurationWizard.rc:131-155 | Dialog template with P12 import controls |
| IDC_03IMPORT radio | EIDConfigurationWizard.rc:139-140 | Radio button for P12 import option |
| IDC_03FILENAME/IDC_03SELECTFILE/IDC_03IMPORTPASSWORD | EIDConfigurationWizard.rc:148-152 | P12 import controls |

---

## Sources

- [Microsoft Learn - Credential Providers in Windows](https://learn.microsoft.com/en-us/windows/win32/secauthn/credential-providers-in-windows)
- [Microsoft Support - 30 Second Smart Card Wait Issue](https://support.microsoft.com/zh-cn/topic/you-may-wait-for-up-to-30-seconds-when-you-use-a-smart-card-to-unlock-a-computer-that-is-running-windows-7-or-windows-server-2008-r2-dcba2f35-8458-e39c-88dd-5e0e152999a1)
- [Microsoft Learn - CryptUIDlgViewCertificate](https://learn.microsoft.com/zh-cn/windows/win32/api/cryptuiapi/ns-cryptuiapi-cryptui_viewcertificate_structa)
- [Microsoft Learn - Certificate Services](https://learn.microsoft.com/en-us/windows/win32/seccrypto/certificate-services)
- Code analysis of EIDAuthentication codebase (existing implementation patterns)

---
*Feature research for: EIDAuthentication v1.7 UI/UX Enhancement*
*Researched: 2026-02-24*
