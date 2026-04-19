// Page10_ImportComplete.cpp - Import Complete Page Implementation
#include "Page10_ImportComplete.h"

INT_PTR CALLBACK WndProc_10_ImportComplete(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
            WCHAR szTemp[64]; // NOSONAR - C-style array required for Windows API swprintf_s/SetDlgItemText compatibility
            DWORD dwTotal = g_wizardData.dwFileCredentialCount;

            swprintf_s(szTemp, ARRAYSIZE(szTemp), L"%u", dwTotal);
            SetDlgItemText(hwndDlg, IDC_10_TOTAL, szTemp);

            swprintf_s(szTemp, ARRAYSIZE(szTemp), L"%u", g_wizardData.dwImportedCount);
            SetDlgItemText(hwndDlg, IDC_10_IMPORTED, szTemp);

            swprintf_s(szTemp, ARRAYSIZE(szTemp), L"%u", g_wizardData.dwFailedCount);
            SetDlgItemText(hwndDlg, IDC_10_FAILED, szTemp);

            SetDlgItemText(hwndDlg, IDC_10_USERS_CREATED, L"0");
            SetDlgItemText(hwndDlg, IDC_10_GROUPS_CREATED, L"0");

            // Set summary text
            std::wstring wsSummary;
            if (g_wizardData.fDryRun) {
                wsSummary = L"Preview complete. No changes were made.";
            } else {
                wsSummary = L"Import completed successfully!";
            }
            SetDlgItemText(hwndDlg, IDC_10_SUMMARY_TEXT, wsSummary.c_str());

            // Warnings
            std::wstring wsWarnings;
            if (g_wizardData.dwFailedCount > 0) {
                swprintf_s(szTemp, ARRAYSIZE(szTemp), L"%u credentials failed to import.",
                    g_wizardData.dwFailedCount);
                wsWarnings = szTemp;
            }
            SetDlgItemText(hwndDlg, IDC_10_WARNINGS, wsWarnings.c_str());

            // Load shield icon
            HMODULE hDll = LoadLibraryW(L"imageres.dll");
            if (hDll) {
                HICON hIcon = LoadIcon(hDll, MAKEINTRESOURCE(58));
                if (hIcon) {
                    SendDlgItemMessage(hwndDlg, IDC_10_SHIELD, STM_SETICON, (WPARAM)hIcon, 0);
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
            return TRUE;

        default:
            break;
        }
        break;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == IDC_10_VIEW_LOG) {
            WCHAR szModulePath[MAX_PATH] = { 0 }; // NOSONAR - Windows API GetModuleFileNameW requires WCHAR buffer
            if (GetModuleFileNameW(nullptr, szModulePath, ARRAYSIZE(szModulePath)) == 0) {
                MessageBoxW(hwndDlg, L"Unable to locate installation directory.",
                    L"View Log", MB_ICONWARNING);
                return TRUE;
            }
            std::wstring wsLogManager(szModulePath);
            size_t pos = wsLogManager.find_last_of(L"\\/");
            if (pos == std::wstring::npos) {
                MessageBoxW(hwndDlg, L"Unable to locate installation directory.",
                    L"View Log", MB_ICONWARNING);
                return TRUE;
            }
            wsLogManager = wsLogManager.substr(0, pos + 1) + L"EIDLogManager.exe";

            HINSTANCE hResult = ShellExecuteW(hwndDlg, L"open", wsLogManager.c_str(),
                nullptr, nullptr, SW_SHOWNORMAL);
            if ((INT_PTR)hResult <= 32) {
                MessageBoxW(hwndDlg,
                    L"Unable to launch EIDLogManager.exe. Open it manually from the "
                    L"installation directory or the Start Menu (EID Authentication -> Log Manager).",
                    L"View Log", MB_ICONWARNING);
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
