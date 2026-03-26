// Page01_Welcome.cpp - Welcome/Action Selection Page Implementation
// Copyright (c) 2026

#include "Page01_Welcome.h"

INT_PTR CALLBACK WndProc_01_Welcome(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        // Set command link note text
        HWND hwndExport = GetDlgItem(hwndDlg, IDC_01_EXPORT);
        if (hwndExport) {
            SendMessage(hwndExport, BCM_SETNOTE, 0,
                (LPARAM)L"Export EID credentials from this machine to an encrypted file");
        }

        HWND hwndImport = GetDlgItem(hwndDlg, IDC_01_IMPORT);
        if (hwndImport) {
            SendMessage(hwndImport, BCM_SETNOTE, 0,
                (LPARAM)L"Import EID credentials from an encrypted export file");
        }

        HWND hwndList = GetDlgItem(hwndDlg, IDC_01_LIST);
        if (hwndList) {
            SendMessage(hwndList, BCM_SETNOTE, 0,
                (LPARAM)L"View all EID credentials on this machine or in an export file");
        }

        HWND hwndValidate = GetDlgItem(hwndDlg, IDC_01_VALIDATE);
        if (hwndValidate) {
            SendMessage(hwndValidate, BCM_SETNOTE, 0,
                (LPARAM)L"Check the integrity and contents of an export file");
        }

        // Reset wizard state
        g_wizardData.Reset();

        // Hide the Next button since we use direct action buttons
        HWND hwndParent = GetParent(hwndDlg);
        if (hwndParent) {
            ShowWindow(GetDlgItem(hwndParent, IDOK), SW_HIDE);
        }

        return TRUE;
    }

    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;

        switch (pnmh->code)
        {
        case PSN_SETACTIVE:
        {
            // Hide Next button, only show Cancel
            HWND hwndParent = GetParent(hwndDlg);
            if (hwndParent) {
                ShowWindow(GetDlgItem(hwndParent, IDOK), SW_HIDE);
                PropSheet_SetWizButtons(hwndDlg, 0);  // No navigation buttons
            }
            return TRUE;
        }

        case PSN_WIZNEXT:
        case PSN_WIZBACK:
            // Prevent default navigation
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
            return TRUE;

        default:
            break;
        }
        break;
    }

    case WM_COMMAND:
    {
        // Handle button clicks directly
        switch (LOWORD(wParam))
        {
        case IDC_01_EXPORT:
        {
            g_wizardData.selectedFlow = FLOW_EXPORT;
            // Navigate to Export Select page (page index 1)
            HWND hwndParent = GetParent(hwndDlg);
            if (hwndParent) {
                PropSheet_SetCurSel(hwndParent, nullptr, 1);
            }
            return TRUE;
        }

        case IDC_01_IMPORT:
        {
            g_wizardData.selectedFlow = FLOW_IMPORT;
            // Navigate to Import Select page (page index 5)
            HWND hwndParent = GetParent(hwndDlg);
            if (hwndParent) {
                PropSheet_SetCurSel(hwndParent, nullptr, 5);
            }
            return TRUE;
        }

        case IDC_01_LIST:
        {
            g_wizardData.selectedFlow = FLOW_LIST;
            // Navigate to List page (page index 10)
            HWND hwndParent = GetParent(hwndDlg);
            if (hwndParent) {
                PropSheet_SetCurSel(hwndParent, nullptr, 10);
            }
            return TRUE;
        }

        case IDC_01_VALIDATE:
        {
            g_wizardData.selectedFlow = FLOW_VALIDATE;
            // Navigate to Validate page (page index 11)
            HWND hwndParent = GetParent(hwndDlg);
            if (hwndParent) {
                PropSheet_SetCurSel(hwndParent, nullptr, 11);
            }
            return TRUE;
        }

        default:
            break;
        }
        break;
    }

    case WM_DESTROY:
        return TRUE;

    default:
        break;
    }

    return FALSE;
}
