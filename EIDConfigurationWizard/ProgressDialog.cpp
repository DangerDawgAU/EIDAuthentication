#include "ProgressDialog.h"
#include "EIDConfigurationWizard.h"
#include <CommCtrl.h>

static HWND g_hProgressDialog = NULL;  // NOSONAR - RUNTIME-01: Dialog handle, set at runtime
static HWND g_hParentWnd = NULL;       // NOSONAR - RUNTIME-01: Parent handle for re-enable

// Dialog procedure for the progress dialog
static INT_PTR CALLBACK ProgressDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            // Suppress unused parameter warnings
            (void)wParam;
            (void)lParam;

            // Center dialog on parent
            RECT rcParent, rcDlg;
            GetWindowRect(g_hParentWnd, &rcParent);
            GetWindowRect(hDlg, &rcDlg);
            int x = rcParent.left + (rcParent.right - rcParent.left - (rcDlg.right - rcDlg.left)) / 2;
            int y = rcParent.top + (rcParent.bottom - rcParent.top - (rcDlg.bottom - rcDlg.top)) / 2;
            SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

            // Start marquee animation
            SendDlgItemMessage(hDlg, IDC_PROGRESSBAR, PBM_SETMARQUEE, TRUE, 50);
        }
        return TRUE;

    case WM_DESTROY:
        // Stop marquee animation
        SendDlgItemMessage(hDlg, IDC_PROGRESSBAR, PBM_SETMARQUEE, FALSE, 0);
        g_hProgressDialog = NULL;
        break;
    }
    return FALSE;
}

HWND ShowProgressDialog(HWND hParentWnd)
{
    if (!hParentWnd || g_hProgressDialog)
        return g_hProgressDialog;  // Already showing or invalid parent

    g_hParentWnd = hParentWnd;

    // Create the modeless dialog
    g_hProgressDialog = CreateDialogParam(
        GetModuleHandle(NULL),
        MAKEINTRESOURCE(IDD_PROGRESS),
        hParentWnd,
        ProgressDialogProc,
        0);

    if (g_hProgressDialog)
    {
        // Disable parent window for modal behavior
        EnableWindow(hParentWnd, FALSE);

        // Show and update the dialog
        ShowWindow(g_hProgressDialog, SW_SHOW);
        UpdateWindow(g_hProgressDialog);

        // Process any pending messages to ensure dialog paints
        MSG msg;
        while (PeekMessage(&msg, g_hProgressDialog, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return g_hProgressDialog;
}

void CloseProgressDialog(HWND hProgressDialog)
{
    if (!hProgressDialog)
        hProgressDialog = g_hProgressDialog;

    if (hProgressDialog)
    {
        // Re-enable parent window BEFORE destroying dialog
        if (g_hParentWnd)
        {
            EnableWindow(g_hParentWnd, TRUE);
            SetForegroundWindow(g_hParentWnd);
            g_hParentWnd = NULL;
        }

        // Destroy the dialog
        DestroyWindow(hProgressDialog);
        g_hProgressDialog = NULL;
    }
}
