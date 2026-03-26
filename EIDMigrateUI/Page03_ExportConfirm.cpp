// Page03_ExportConfirm.cpp - Export Confirmation Page Implementation
#include "Page03_ExportConfirm.h"

INT_PTR CALLBACK WndProc_03_ExportConfirm(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        return TRUE;

    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        switch (pnmh->code)
        {
        case PSN_SETACTIVE:
        {
            // Enumerate credentials to show count
            std::vector<CredentialInfo> credentials;
            HRESULT hr = EnumerateLsaCredentials(credentials);

            WCHAR szCount[32]; // NOSONAR - C-style array required for Windows API swprintf_s/SetDlgItemText
            swprintf_s(szCount, ARRAYSIZE(szCount), L"%zu", credentials.size());
            SetDlgItemText(hwndDlg, IDC_03_CREDENTIAL_COUNT, szCount);

            SetDlgItemText(hwndDlg, IDC_03_OUTPUT_PATH, g_wizardData.wsOutputFile.c_str());

            // Load shield icon
            HMODULE hDll = LoadLibraryW(L"imageres.dll");
            if (hDll) {
                HICON hIcon = LoadIcon(hDll, MAKEINTRESOURCE(58));
                if (hIcon) {
                    SendDlgItemMessage(hwndDlg, IDC_03_SHIELD, STM_SETICON, (WPARAM)hIcon, 0);
                }
                FreeLibrary(hDll);
            }

            PropSheet_SetWizButtons(hwndDlg, PSWIZB_BACK | PSWIZB_NEXT);
            return TRUE;
        }

        case PSN_WIZNEXT:
            // Proceed to export progress page
            return TRUE;

        default:
            break;
        }
        break;
    }

    default:
        break;
    }
    return FALSE;
}
