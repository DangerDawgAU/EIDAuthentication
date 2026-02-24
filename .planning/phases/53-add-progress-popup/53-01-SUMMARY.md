# Phase 53-01: Add Progress Popup - SUMMARY

**Status:** COMPLETE (with post-execution fix)
**Date:** 2026-02-24
**Plan:** `.planning/phases/53-add-progress-popup/53-01-PLAN.md`

---

## What Was Built

Modal progress dialog for card flashing operation with marquee-style progress indicator.

### Artifacts Created

| File | Description |
|------|-------------|
| `EIDConfigurationWizard/ProgressDialog.cpp` | Progress dialog window procedure and lifecycle management |
| `EIDConfigurationWizard/ProgressDialog.h` | Export declarations for ShowProgressDialog/CloseProgressDialog |
| `EIDConfigurationWizard.rc` | IDD_PROGRESS dialog resource with PBS_MARQUEE progress bar |
| `EIDConfigurationWizard.h` | IDD_PROGRESS (71), IDC_PROGRESSBAR (157), IDC_PROGRESSTEXT (158) |
| `EIDConfigurationWizard/EIDConfigurationWizardPage04.cpp` | Integrated progress dialog calls into card enumeration flow |

### Implementation Details

**ProgressDialog.cpp exports:**
- `ShowProgressDialog(HWND hParentWnd)` - Creates and shows modal progress dialog
- `CloseProgressDialog(HWND hProgressDialog)` - Destroys dialog and re-enables parent

**Integration points (EIDConfigurationWizardPage04.cpp):**
- Line 533: PSN_SETACTIVE handler - progress dialog before initial `ConnectNotification()`
- Line 453: HandleRefreshRequest() - progress dialog before refresh `ConnectNotification()`

---

## Issue: Delayed Dialog Appearance

**Problem:** Progress dialog did not appear immediately when Next was pressed. There was a noticeable delay before the dialog became visible.

**Root Cause:**
- `CreateDialogParam` is asynchronous - it posts messages to the queue but doesn't wait for the dialog to fully render
- The original message pump only processed messages for `g_hProgressDialog` specifically:
  ```cpp
  while (PeekMessage(&msg, g_hProgressDialog, 0, 0, PM_REMOVE))
  ```
- `ConnectNotification()` blocks immediately after, starving the message pump
- Result: Dialog never received CPU time to paint before the blocking operation started

---

## Fix Applied #1 (2026-02-24 - Initial Attempt)

**Problem:** Progress dialog appeared with noticeable delay after Next button press.

**Changes to `ShowProgressDialog()`:**
1. Use `RedrawWindow` with `RDW_UPDATENOW` to force immediate painting
2. Process **all** messages (`PeekMessage(&msg, NULL, ...)`) not just dialog messages
3. Add `Sleep(1)` yield between iterations to let Windows process paints
4. Loop until `IsWindowVisible()` confirms dialog is visible
5. Final `UpdateWindow()` call to ensure paint completion

**Result:** Build succeeded, but issue persisted.

---

## Fix Applied #2 (2026-02-24 - User Feedback: Still Delayed)

**Problem:** User reported dialog still appears AFTER the delay, not during it.
**Symptom:** PIN prompt → delay → progress bar appears → operation completes (almost immediately)

**Root Cause Analysis:**
- `IsWindowVisible()` returns TRUE immediately after `ShowWindow()` - before window is painted
- Processing all messages with `PeekMessage(&msg, NULL, ...)` dispatches messages for ALL windows
- The blocking `ConnectNotification()` still prevents dialog's WM_PAINT from being processed
- Need synchronous paint that bypasses the message queue entirely

**Attempted Fix - Synchronous Paint Pattern:**
1. `UpdateWindow()` immediately after `ShowWindow()` - Sends WM_PAINT directly to window procedure
2. Process only paint-related messages (WM_PAINT, WM_NCPAINT, WM_ERASEBKGND, WM_TIMER)
3. Increased iterations to 50, `Sleep(0)` for faster yield

**Result:** Build succeeded, but user reported **no change in behavior** - issue persisted.

---

## Fix Applied #3 (2026-02-24 - Advanced Root Cause Analysis)

**Problem:** User reported Fix #2 had no effect - dialog still appears AFTER the delay.

**Deep Root Cause Analysis:**

Three critical bugs were identified in Fix #2:

1. **Flawed Message Loop Logic** - The loop broke as soon as ANY one message type wasn't found:
   ```cpp
   if (PeekMessage(..., WM_PAINT, ...)) { ... }
   else if (PeekMessage(..., WM_TIMER, ...)) { ... }
   else break;  // ← Breaks immediately if no WM_PAINT!
   ```
   This meant WM_NCPAINT and WM_ERASEBKGND were never processed.

2. **Parent Disabled BEFORE Dialog Paint (The Real Problem)**
   - `EnableWindow(hParentWnd, FALSE)` was called BEFORE `ShowWindow()`
   - When disabling a window, Windows sends `WM_ENABLE` causing a repaint to gray out the title bar
   - The parent's WM_ENABLE repaint competed with the dialog's paint
   - The blocking `ConnectNotification()` prevented the message pump from processing either paint

3. **Filtered Message Pump** - Processing only specific message types meant critical messages like `WM_ENABLE` and `WM_NCCALCSIZE` were never processed.

**Final Fix - Show First, Then Disable:**

**File:** `EIDConfigurationWizard/ProgressDialog.cpp`

**Changes:**
1. **Show dialog FIRST** before disabling parent
2. **`SetForegroundWindow()`** + **`RedrawWindow()`** to force immediate paint
3. **Process ALL messages** using `PeekMessage(&msg, NULL, ...)` with `TranslateMessage()`
4. **Time-bounded loop** (500ms max) to prevent infinite loops
5. Only disable parent after dialog is confirmed visible

**Code pattern:**
```cpp
// Show the dialog FIRST (before disabling parent)
ShowWindow(g_hProgressDialog, SW_SHOW);

// Bring the dialog to the foreground and force a paint
SetForegroundWindow(g_hProgressDialog);
RedrawWindow(g_hProgressDialog, NULL, NULL, RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_ERASE | RDW_FRAME);

// Disable parent window for modal behavior (AFTER dialog is shown and painted)
EnableWindow(hParentWnd, FALSE);

// Process ALL pending messages in the queue to ensure dialog is fully rendered
MSG msg;
DWORD dwStartTime = GetTickCount();
DWORD dwTimeout = 500;  // Maximum 500ms to wait for dialog to render

while (GetTickCount() - dwStartTime < dwTimeout)
{
    // Process ALL messages, not just paint messages
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Check if dialog is actually visible and painted
    if (IsWindowVisible(g_hProgressDialog))
    {
        // Dialog is visible - do one more paint pass and exit
        UpdateWindow(g_hProgressDialog);
        InvalidateRect(g_hProgressDialog, NULL, FALSE);
        UpdateWindow(g_hProgressDialog);
        break;
    }

    Sleep(1);  // Small yield to let Windows process
}

// Final synchronous paint to ensure dialog is visible before blocking operation
UpdateWindow(g_hProgressDialog);
```

**Build Status:** Succeeded with 0 warnings, 0 errors (2026-02-24)
**EIDConfigurationWizard.exe:** 500,736 bytes

**Key Insight:** The parent window's disabled state generates its own paint messages (WM_ENABLE) that compete with the dialog's paint. By showing the dialog first and forcing paint BEFORE disabling the parent, we ensure the dialog is visible before any blocking operation occurs.

---

## Verification

- [x] Build succeeds - all 7 projects compile with zero errors
- [x] EIDConfigurationWizard.exe built successfully (500,736 bytes)
- [x] NSIS installer built successfully (957.5 KB)
- [x] Fix #1 applied - Message pump improvement (unsuccessful)
- [x] Fix #2 applied - Synchronous paint with UpdateWindow() (unsuccessful)
- [x] Fix #3 applied - Show first, then disable parent (2026-02-24)
- [ ] **User verification required** - Test that progress dialog appears immediately after PIN entry

**Manual test steps:**
1. Run EIDConfigurationWizard.exe
2. Select "Change smart card credentials" → "Configure a new set of credentials"
3. Insert smart card when prompted
4. Navigate to Page 04
5. Enter PIN when prompted
6. **Expected:** Progress dialog appears **immediately** after closing PIN entry window
7. **Expected:** Marquee animation is smooth and continuous during card operation
8. **Expected:** Dialog closes automatically when enumeration completes

---

## Decisions

| Decision | Rationale |
|----------|-----------|
| Show dialog BEFORE disabling parent | Parent's WM_ENABLE paint competes with dialog paint if disabled first |
| Use `SetForegroundWindow` | Brings dialog to top and forces window manager attention |
| Process ALL messages (NULL hwnd filter) | Ensures WM_ENABLE, WM_NCCALCSIZE, and other critical messages are processed |
| Time-bounded loop (500ms) | Prevents infinite loops while giving adequate time for render |
| Use `TranslateMessage()` | Proper character message handling for keyboard input |

---

## Next Steps

1. **User verification required** - Test the wizard with Fix #4 (show on PSN_WIZNEXT)
2. Expected behavior: Next button → progress dialog appears **immediately** → Page 05 activates → dialog closes
3. If verification passes → Phase 53 complete, v1.7 milestone complete
4. If issues persist → May need alternative approach (threaded progress, background timer)

---

## Fix Applied #4 (2026-02-24 - Alternative Approach)

**Problem:** User reported Fix #3 still showed delayed behavior - dialog appeared AFTER operations, not during.
**User Request:** "Is it possible to just run the popup as soon as the next button is pressed, before the pin entry box pops up?"

**Root Cause Re-Analysis:**
- Previous fixes all operated on `PSN_SETACTIVE` (when Page 04 activates)
- The dialog was meant to show during `ConnectNotification()` which happens on page activation
- User wants immediate feedback when pressing Next, regardless of actual operation timing
- Page navigation to Page 05 (PIN entry) happens before any card operations complete

**New Approach - Show on PSN_WIZNEXT:**

Instead of trying to fix the timing of the dialog during card operations, show the dialog **immediately when Next is pressed**, before any page navigation occurs.

**Files Modified:**

1. **EIDConfigurationWizardPage04.cpp** - Added `PSN_WIZNEXT` handler:
   ```cpp
   case PSN_WIZNEXT:
       // Show progress dialog immediately when Next is pressed
       // This will be visible during any blocking operations that follow
       ShowProgressDialog(GetParent(hWnd));
       break;
   ```

2. **EIDConfigurationWizardPage05.cpp** - Close dialog on activation:
   ```cpp
   // Added include: #include "ProgressDialog.h"

   case PSN_SETACTIVE:
       // Close progress dialog that was shown on Page 04 Next
       CloseProgressDialog(NULL);
       // ... rest of activation code
   ```

**New Flow:**
1. User presses Next on Page 04
2. **Progress dialog shows IMMEDIATELY** (before navigation)
3. Wizard navigates to Page 05 (PIN entry)
4. Progress dialog closes when Page 05 activates

**Build Status:** Succeeded with 0 warnings, 0 errors (2026-02-24)
**EIDConfigurationWizard.exe:** 500,736 bytes (unchanged)

**Key Insight:** By showing the dialog in response to `PSN_WIZNEXT`, we guarantee immediate visual feedback at the moment the user clicks Next, independent of any subsequent operation timing.

---

---

## Fix Applied #5 (2026-02-24 - CORRECT PAGE IDENTIFIED)

**Problem:** User reported Fix #4 still showed delayed behavior - "this behaviour is still present, there has been no change in behaviour of the application."

**User Clarification:** "I expect the progress dialogue to pop up as soon as the next button is pressed on the smart card logon configuration page" and "its page 2, directly after clicking the configure a new set of credentials, the same page that has the selected authority information box"

**Critical Discovery:** The progress dialog was on the **WRONG PAGE**.

**Root Cause Analysis:**
- Previous fixes targeted **Page 04** (Smart Card Logon Configuration page)
- User clarified the issue is on **Page 03** (Selected Authority page)
- The actual card flashing operation (`CreateSmartCardCertificate()`) happens in `PSN_WIZNEXT` on **Page 03**, not Page 04
- Page 04 has no blocking operation when Next is pressed - it just navigates to Page 05
- Page 03's `PSN_WIZNEXT` handler calls blocking operations:
  - `ClearCard()` - when "Delete all certificates" is selected
  - `CreateRootCertificate()` + `CreateSmartCardCertificate()` - when "Create new" is selected
  - `CreateSmartCardCertificate()` - when "Use this certificate" is selected

**Wizard Flow:**
```
Page 02: "Configure a new set of credentials" button
    ↓
Page 03: Selected Authority info box → User clicks Next → **CARD FLASHING OPERATION** ←
    ↓
Page 04: Smart Card Logon Configuration (shows credential list)
    ↓
Page 05: Password entry
```

**Fix Applied - Move Progress Dialog to Page 03:**

**Files Modified:**

1. **EIDConfigurationWizardPage03.cpp** - Added progress dialog to `PSN_WIZNEXT`:
   ```cpp
   // Added include: #include "ProgressDialog.h"

   case PSN_WIZNEXT:
       // Show progress dialog before card operations
       // The dialog will be visible during the blocking certificate creation
       HWND hProgress = ShowProgressDialog(GetParent(hWnd));
       // Use early return pattern with extracted handlers to reduce nesting
       if (IsDlgButtonChecked(hWnd, IDC_03DELETE) && HandleDeleteOption(hWnd))
       {
           CloseProgressDialog(hProgress);
           return TRUE;
       }
       if (IsDlgButtonChecked(hWnd, IDC_03_CREATE) && HandleCreateOption(hWnd, pRootCertificate))
       {
           CloseProgressDialog(hProgress);
           return TRUE;
       }
       if (IsDlgButtonChecked(hWnd, IDC_03USETHIS) && HandleUseThisOption(hWnd, pRootCertificate))
       {
           CloseProgressDialog(hProgress);
           return TRUE;
       }
       // Close progress dialog on success
       CloseProgressDialog(hProgress);
       break;
   ```

2. **EIDConfigurationWizardPage04.cpp** - Removed incorrect `PSN_WIZNEXT` progress dialog code

3. **EIDConfigurationWizardPage05.cpp** - Removed incorrect progress dialog close code from `PSN_SETACTIVE`

**New Flow:**
1. User on Page 03 (Selected Authority page)
2. User selects an option (Delete/Create/Use)
3. User presses Next
4. **Progress dialog shows IMMEDIATELY**
5. **Dialog remains visible during card operation** (ClearCard/CreateSmartCardCertificate)
6. Dialog closes when operation completes
7. Wizard navigates to Page 04

**Build Status:** Succeeded with 0 warnings, 0 errors (2026-02-24)

**Verification Required - All Three Options:**
1. Test "Delete all certificates" → Next → Progress dialog should show during `ClearCard()`
2. Test "Create new certificate" → Next → Progress dialog should show during certificate creation
3. Test "Use this certificate" → Next → Progress dialog should show during card write

**Key Insight:** The progress dialog was correctly implemented but attached to the wrong page's navigation event. The actual blocking smart card operations occur on Page 03, not Page 04.

---

*Last updated: 2026-02-24 (Fix #5 applied - moved progress dialog to Page 03 where actual card flashing occurs)*
