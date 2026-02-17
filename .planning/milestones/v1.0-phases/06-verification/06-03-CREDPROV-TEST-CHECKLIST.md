# Credential Provider Testing Checklist

**Plan:** 06-03
**Component:** EIDCredentialProvider.dll (Credential Provider v2)
**CLSID:** {B4866A0A-DB08-4835-A26F-414B46F3244C}

---

## Test Prerequisites

- [ ] LSA package tested and working (from 06-02)
- [ ] Test user account exists on system
- [ ] Smart card reader connected or virtual smart card created
- [ ] Test smart card with enrolled certificate available
- [ ] Administrator access for DLL registration

---

## Installation Steps

1. Copy `EIDCredentialProvider.dll` from build output (`x64\Release\`) to `C:\Windows\System32\`
2. Register COM components:
   ```
   regsvr32 C:\Windows\System32\EIDCredentialProvider.dll
   ```
3. Confirm registration success message appears
4. Verify registry entries:
   - `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\Credential Providers\{B4866A0A-DB08-4835-A26F-414B46F3244C}`
   - `HKCR\CLSID\{B4866A0A-DB08-4835-A26F-414B46F3244C}`
5. Reboot or restart Winlogon process

---

## Verification Steps

### Scenario 1: Login Screen

1. Lock the computer (Win+L or Ctrl+Alt+Del)
2. At login screen, look for:
   - "Smart Card" or "EID Authentication" tile/icon
   - Alternative credentials selector
3. **Document:** Does the EID Credential Provider appear? (Yes/No)

### Scenario 2: Smart Card Detection

1. While at login screen, insert smart card
2. Wait 2-3 seconds for detection
3. **Document:** Does Credential Provider detect the card? (Yes/No)
4. If yes, verify certificate information displays correctly

### Scenario 3: PIN Entry

1. If card detected, click on the smart card credential
2. **Document:** Does PIN entry interface appear? (Yes/No)
3. Enter PIN (use test PIN if applicable)
4. **Document:** Does authentication proceed? (Yes/No)

### Scenario 4: Lock Screen

1. Log in to Windows with password
2. Lock computer (Win+L)
3. Verify Credential Provider appears on lock screen
4. Test smart card login from lock screen

### Scenario 5: Credential UI

1. Open Command Prompt
2. Run: `runas /smartcard`
3. Verify Credential Provider appears in Credential UI

---

## Event Viewer Verification

1. Open Event Viewer (`eventvwr.msc`)
2. Navigate to Windows Logs -> System
3. Filter for "Winlogon" or "Credential Provider" events
4. **Document:** Any errors or warnings?

---

## Results Template

For each Windows version tested, report:

| Check | Windows 7 | Windows 10 | Windows 11 |
|-------|-----------|------------|------------|
| DLL copied and registered | [ ] | [ ] | [ ] |
| Registry entries verified | [ ] | [ ] | [ ] |
| Credential Provider appears on login screen | [ ] Yes/No | [ ] Yes/No | [ ] Yes/No |
| Smart card detection working | [ ] Yes/No | [ ] Yes/No | [ ] Yes/No |
| PIN entry interface displays | [ ] Yes/No | [ ] Yes/No | [ ] Yes/No |
| Authentication proceeds | [ ] Yes/No | [ ] Yes/No | [ ] Yes/No |
| Event Viewer errors | [ ] None/List | [ ] None/List | [ ] None/List |
| **Overall Result** | [ ] PASS/FAIL/PARTIAL | [ ] PASS/FAIL/PARTIAL | [ ] PASS/FAIL/PARTIAL |

---

## Status Definitions

- **PASS:** All functionality works correctly
- **PARTIAL:** UI appears and card detection works, but full authentication not tested (e.g., no enrolled smart card available)
- **FAIL:** Critical functionality broken (COM registration failure, UI not appearing, etc.)

---

## Notes

**Note:** Full authentication flow (actual login) requires an enrolled smart card with matching certificate. If test card is not available, verify UI appearance and card detection only.

**C++23 Compatibility:** This test verifies that the Credential Provider works correctly after C++23 conformance fixes from Phases 2.1 and 2.2. Any UI rendering issues or COM registration failures may indicate C++23-related regressions.

---

*Checklist created: 2026-02-15*
*Plan: 06-03 Credential Provider Testing*
