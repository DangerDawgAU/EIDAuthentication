// Page08_ImportPreview.cpp - Import Preview Page Implementation
#include "Page08_ImportPreview.h"

INT_PTR CALLBACK WndProc_08_ImportPreview(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        // Setup listview columns
        HWND hList = GetDlgItem(hwndDlg, IDC_08_LIST);
        if (hList) {
            LVCOLUMN lvc = {0};
            lvc.mask = LVCF_TEXT | LVCF_WIDTH;
            lvc.pszText = const_cast<LPWSTR>(L"Username"); // Safe: ListView won't modify
            lvc.cx = 100;
            ListView_InsertColumn(hList, 0, &lvc);
            lvc.pszText = const_cast<LPWSTR>(L"RID"); // Safe: ListView won't modify
            lvc.cx = 50;
            ListView_InsertColumn(hList, 1, &lvc);
            lvc.pszText = const_cast<LPWSTR>(L"Encryption"); // Safe: ListView won't modify
            lvc.cx = 80;
            ListView_InsertColumn(hList, 2, &lvc);
            lvc.pszText = const_cast<LPWSTR>(L"Status"); // Safe: ListView won't modify
            lvc.cx = 100;
            ListView_InsertColumn(hList, 3, &lvc);

            // Set extended style
            ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
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
            // Populate listview with credentials
            HWND hList = GetDlgItem(hwndDlg, IDC_08_LIST);
            if (hList) {
                ListView_DeleteAllItems(hList);

                for (const auto& cred : g_wizardData.credentials) {
                    LVITEM lvi = {0};
                    lvi.mask = LVIF_TEXT;
                    lvi.pszText = const_cast<LPWSTR>(cred.wsUsername.c_str()); // Safe: ListView won't modify
                    int iItem = ListView_InsertItem(hList, &lvi);

                    WCHAR szRID[32];
                    swprintf_s(szRID, ARRAYSIZE(szRID), L"%u", cred.dwRid);
                    ListView_SetItemText(hList, iItem, 1, szRID);

                    PCWSTR pwszEnc = (cred.EncryptionType == EID_PRIVATE_DATA_TYPE::eidpdtCrypted) ? L"Certificate" : L"DPAPI";
                    ListView_SetItemText(hList, iItem, 2, const_cast<LPWSTR>(pwszEnc)); // Safe: ListView won't modify

                    // Check if user exists
                    std::wstring wsStatus = L"User exists";
                    // TODO: Check user existence
                    ListView_SetItemText(hList, iItem, 3, const_cast<LPWSTR>(wsStatus.c_str())); // Safe: ListView won't modify
                }
            }

            // Build warnings text
            std::wstring wsWarnings;
            if (g_wizardData.fDryRun) {
                wsWarnings = L"DRY-RUN MODE: No changes will be made.\r\n\r\n";
            } else {
                wsWarnings = L"WARNING: This will import credentials to this machine.\r\n\r\n";
            }

            if (!g_wizardData.fDryRun) {
                wsWarnings += L"Click Next to continue with the import.";
            } else {
                wsWarnings += L"Click Next to proceed (preview mode).";
            }
            SetDlgItemText(hwndDlg, IDC_08_WARNINGS, wsWarnings.c_str());

            // Load shield icon
            HMODULE hDll = LoadLibraryW(L"imageres.dll");
            if (hDll) {
                HICON hIcon = LoadIcon(hDll, MAKEINTRESOURCE(58));
                if (hIcon) {
                    SendDlgItemMessage(hwndDlg, IDC_08_SHIELD, STM_SETICON, (WPARAM)hIcon, 0);
                }
                FreeLibrary(hDll);
            }

            PropSheet_SetWizButtons(hwndDlg, PSWIZB_BACK | PSWIZB_NEXT);
            return TRUE;
        }

        case PSN_WIZNEXT:
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
