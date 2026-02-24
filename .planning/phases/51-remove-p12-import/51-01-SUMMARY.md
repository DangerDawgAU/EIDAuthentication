---
phase: 51-remove-p12-import
plan: 01
subsystem: EIDConfigurationWizard
tags: [ui, cleanup, removal, p12-import]
requires: []
provides:
  - Cleaner Configuration Wizard UI without P12 import option
  - Simplified PSN_WIZNEXT handler with 3 options instead of 4
affects:
  - EIDConfigurationWizard/EIDConfigurationWizard.rc
  - EIDConfigurationWizard/EIDConfigurationWizardPage03.cpp
tech-stack:
  added: []
  patterns:
    - Win32 Dialog Resource Definition
    - Win32 Dialog Procedure Pattern
key-files:
  created: []
  modified:
    - EIDConfigurationWizard/EIDConfigurationWizard.rc
    - EIDConfigurationWizard/EIDConfigurationWizardPage03.cpp
decisions:
  - Remove entire SelectFile function since it was only used by IDC_03SELECTFILE handler
  - Keep resource ID definitions in EIDConfigurationWizard.h for stability
  - Keep ImportFileToSmartCard library function (not part of UI scope)
metrics:
  duration: 5 minutes
  completed-date: 2026-02-24
  tasks: 3
  files: 2
---

# Phase 51 Plan 01: Remove P12 Import Option Summary

**One-liner:** Removed legacy P12 import UI controls and handling code from the Configuration Wizard, simplifying the user experience to only show Create and UseThis options.

## Objective

Remove the legacy P12 import option from the Configuration Wizard to provide a cleaner, more focused user experience. The P12 import feature was rarely used for the target use case (local account smart card authentication on non-domain machines).

## Tasks Completed

| Task | Description | Status | Commit |
|------|-------------|--------|--------|
| 1 | Remove P12 import controls from dialog resource | Complete | 0657835 |
| 2 | Remove P12 import handling code from Page03 | Complete | ea7b314 |
| 3 | Build and verify compilation | Complete | ffc9d1c |

## Changes Made

### Task 1: Dialog Resource (EIDConfigurationWizard.rc)

Removed from IDD_03NEW dialog:
- `IDC_03IMPORT` radio button - "Import a p12 file into the smart card"
- `IDC_03IMPORTPASSWORD` edit text (password field)
- `IDC_STC2` static text - "Password :"
- `IDC_STC3` static text - "File :"
- `IDC_03FILENAME` edit text (file path field)
- `IDC_03SELECTFILE` push button "..."

### Task 2: Source Code (EIDConfigurationWizardPage03.cpp)

Removed:
- `HandleImportOption` forward declaration (line 16)
- `SelectFile` function (lines 62-92) - entire function removed
- `CheckDlgButton(hWnd, IDC_03IMPORT, BST_UNCHECKED)` from UpdateCertificatePanel (line 191)
- `HandleImportOption` function implementation (lines 271-283)
- `IsDlgButtonChecked(hWnd, IDC_03IMPORT) && HandleImportOption(hWnd)` from PSN_WIZNEXT (line 338)
- `case IDC_03SELECTFILE: SelectFile(hWnd); break;` from WM_COMMAND handler (lines 396-398)

### Task 3: Build Verification

- All 7 projects compiled successfully with 0 errors
- EIDConfigurationWizard.exe: 496640 bytes
- NSIS installer: 956.1 KB

## Verification Results

1. **Source code verification:**
   - `HandleImportOption` function removed - PASSED
   - `SelectFile` function removed - PASSED
   - No references to `IDC_03IMPORT`, `IDC_03FILENAME`, `IDC_03SELECTFILE`, `IDC_03IMPORTPASSWORD` - PASSED
   - PSN_WIZNEXT handler has exactly 3 option checks (Delete, Create, UseThis) - PASSED

2. **Resource file verification:**
   - IDD_03NEW dialog contains no P12 import controls - PASSED
   - Dialog preserves Create, UseThis, Certificate Panel, Validity, and Delete controls - PASSED

3. **Build verification:**
   - All 7 projects compile successfully - PASSED
   - No linker errors - PASSED
   - EIDConfigurationWizard.exe generated - PASSED

## Success Criteria Met

1. User cannot see P12 import controls in the Configure Smart Card wizard - **MET**
2. User cannot trigger P12 import functionality through any UI element - **MET**
3. Build compiles without errors after P12 removal - **MET**

## Deviations from Plan

None - plan executed exactly as written.

## Files Modified

| File | Changes |
|------|---------|
| EIDConfigurationWizard/EIDConfigurationWizard.rc | Removed 7 P12 import-related control definitions from IDD_03NEW dialog |
| EIDConfigurationWizard/EIDConfigurationWizardPage03.cpp | Removed SelectFile function, HandleImportOption function, and all P12-related handler code |

## Key Decisions

1. **Remove entire SelectFile function** - Since it was only called by IDC_03SELECTFILE handler (being removed), the entire function was removed rather than kept as dead code.

2. **Keep resource ID definitions** - The resource ID macros in EIDConfigurationWizard.h were intentionally left unchanged to maintain resource ID stability and avoid potential issues with resource compilation.

3. **Keep ImportFileToSmartCard library function** - The `ImportFileToSmartCard()` function in CertificateUtilities.cpp was not removed as it may be used elsewhere and is outside the UI removal scope.

## Commits

- `0657835`: feat(51-01): remove P12 import controls from dialog resource
- `ea7b314`: feat(51-01): remove P12 import handling code from Page03
- `ffc9d1c`: build(51-01): verify compilation after P12 import removal

## Requirements Satisfied

- **UIUX-01**: Remove P12 import option from Configuration Wizard - **COMPLETE**

## Self-Check: PASSED

- SUMMARY.md exists: FOUND
- Commit 0657835 exists: FOUND
- Commit ea7b314 exists: FOUND
- Commit ffc9d1c exists: FOUND
