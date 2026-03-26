// Page05_ExportComplete.cpp - Export Complete Page Implementation
#include "Page05_ExportComplete.h"
#include <shellapi.h>

INT_PTR CALLBACK WndProc_05_ExportComplete(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
            // Show results
            WCHAR szTemp[64]; // NOSONAR - C-style array required for Windows API swprintf_s/SetDlgItemText

            swprintf_s(szTemp, ARRAYSIZE(szTemp), L"%u", g_wizardData.dwExportedCount);
            SetDlgItemText(hwndDlg, IDC_05_TOTAL_CREDENTIALS, szTemp);
            SetDlgItemText(hwndDlg, IDC_05_CERT_ENCRYPTED, szTemp);
            SetDlgItemText(hwndDlg, IDC_05_DPAPI_ENCRYPTED, L"0");

            swprintf_s(szTemp, ARRAYSIZE(szTemp), L"%u", g_wizardData.groups.size());
            SetDlgItemText(hwndDlg, IDC_05_GROUPS_EXPORTED, szTemp);

            SetDlgItemText(hwndDlg, IDC_05_OUTPUT_PATH, g_wizardData.wsOutputFile.c_str());

            // Load shield icon
            HMODULE hDll = LoadLibraryW(L"imageres.dll");
            if (hDll) {
                HICON hIcon = LoadIcon(hDll, MAKEINTRESOURCE(58));
                if (hIcon) {
                    SendDlgItemMessage(hwndDlg, IDC_05_SHIELD, STM_SETICON, (WPARAM)hIcon, 0);
                }
                FreeLibrary(hDll);
            }

            // Change Next button to Finish
            HWND hwndPS = GetParent(hwndDlg);
            if (hwndPS) {
                SetWindowTextW(GetDlgItem(hwndPS, IDOK), L"Finish");
            }

            PropSheet_SetWizButtons(hwndDlg, PSWIZB_BACK | PSWIZB_FINISH);
            return TRUE;
        }

        case PSN_WIZFINISH:
            // Allow wizard to close
            return TRUE;

        default:
            break;
        }
        break;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == IDC_05_OPEN_FOLDER) {
            // Open folder containing the exported file
            std::wstring wsFolder = g_wizardData.wsOutputFile;
            size_t pos = wsFolder.find_last_of(L"\\/");
            if (pos != std::wstring::npos) {
                wsFolder = wsFolder.substr(0, pos);
                ShellExecuteW(nullptr, L"explore", wsFolder.c_str(), nullptr, nullptr, SW_SHOW);
            }
            return TRUE;
        }
        break;
    }

    default:
        break;
    }
    return FALSE;
}
