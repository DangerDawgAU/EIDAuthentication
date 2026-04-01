// Page14_GroupSelect.cpp - Group Selection Page Implementation
// This page is used for both export and import flows

#include "Page14_GroupSelect.h"
#include "EIDMigrateUI.h"
#include "../EIDMigrate/GroupManagement.h"

INT_PTR CALLBACK WndProc_14_GroupSelect(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        // Get the listbox
        HWND hList = GetDlgItem(hwndDlg, IDC_14_GROUP_LIST);
        if (!hList)
            return FALSE;

        // Clear any existing items
        ListBox_ResetContent(hList);

        // Enumerate local groups and populate the listbox
        if (g_currentFlow == FLOW_EXPORT)
        {
            // For export, get local groups from the machine
            std::vector<LocalGroupInfo> groups;
            HRESULT hr = EnumerateLocalGroups(groups);
            if (SUCCEEDED(hr))
            {
                for (const auto& group : groups)
                {
                    // Add group name to listbox
                    int index = static_cast<int>(SendMessageW(hList, LB_ADDSTRING, 0,
                        reinterpret_cast<LPARAM>(group.wsName.c_str())));
                    if (index != LB_ERR)
                    {
                        // Store group name as item data
                        SendMessageW(hList, LB_SETITEMDATA, index,
                            reinterpret_cast<LPARAM>(group.wsName.c_str()));
                    }
                }
            }
        }
        else if (g_currentFlow == FLOW_IMPORT)
        {
            // For import, use groups from the decrypted file
            for (const auto& group : g_wizardData.groups)
            {
                int index = static_cast<int>(SendMessageW(hList, LB_ADDSTRING, 0,
                    reinterpret_cast<LPARAM>(group.wsName.c_str())));
                if (index != LB_ERR)
                {
                    // Store group name as item data
                    SendMessageW(hList, LB_SETITEMDATA, index,
                        reinterpret_cast<LPARAM>(group.wsName.c_str()));
                }
            }
        }

        // Update the count label
        WCHAR szCount[64];
        swprintf_s(szCount, ARRAYSIZE(szCount), L"Selected: 0 of %d groups",
            SendMessageW(hList, LB_GETCOUNT, 0, 0));
        SetDlgItemText(hwndDlg, IDC_14_SELECTED_COUNT, szCount);

        return TRUE;
    }

    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        switch (pnmh->code)
        {
        case PSN_SETACTIVE:
        {
            PropSheet_SetWizButtons(hwndDlg, PSWIZB_BACK | PSWIZB_NEXT);

            // Update the title based on flow
            HWND hwndParent = GetParent(hwndDlg);
            if (hwndParent)
            {
                WCHAR szTitle[128];
                if (g_currentFlow == FLOW_EXPORT)
                    wcscpy_s(szTitle, ARRAYSIZE(szTitle), L"Select Groups to Export");
                else
                    wcscpy_s(szTitle, ARRAYSIZE(szTitle), L"Select Groups to Import");
                SetWindowTextW(hwndParent, szTitle);
            }
            return TRUE;
        }

        case PSN_WIZNEXT:
        {
            // Store selected groups
            g_wizardData.SelectedGroups.clear();

            HWND hList = GetDlgItem(hwndDlg, IDC_14_GROUP_LIST);
            if (hList)
            {
                int count = static_cast<int>(SendMessageW(hList, LB_GETCOUNT, 0, 0));
                int selCount = 0;

                for (int i = 0; i < count; i++)
                {
                    if (SendMessageW(hList, LB_GETSEL, i, 0))
                    {
                        WCHAR szName[256];
                        if (SendMessageW(hList, LB_GETTEXT, i,
                            reinterpret_cast<LPARAM>(szName)) != LB_ERR)
                        {
                            g_wizardData.SelectedGroups.push_back(szName);
                            selCount++;
                        }
                    }
                }

                EIDM_TRACE_VERBOSE(L"User selected %d groups for %s",
                    selCount, (g_currentFlow == FLOW_EXPORT) ? L"export" : L"import");
            }
            return TRUE;
        }

        default:
            break;
        }
        break;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_14_SELECT_ALL:
        {
            HWND hList = GetDlgItem(hwndDlg, IDC_14_GROUP_LIST);
            if (hList)
            {
                int count = static_cast<int>(SendMessageW(hList, LB_GETCOUNT, 0, 0));
                for (int i = 0; i < count; i++)
                    SendMessageW(hList, LB_SETSEL, TRUE, i);

                WCHAR szCount[64];
                swprintf_s(szCount, ARRAYSIZE(szCount), L"Selected: %d of %d groups", count, count);
                SetDlgItemText(hwndDlg, IDC_14_SELECTED_COUNT, szCount);
            }
            return TRUE;
        }

        case IDC_14_CLEAR_ALL:
        {
            HWND hList = GetDlgItem(hwndDlg, IDC_14_GROUP_LIST);
            if (hList)
            {
                int count = static_cast<int>(SendMessageW(hList, LB_GETCOUNT, 0, 0));
                for (int i = 0; i < count; i++)
                    SendMessageW(hList, LB_SETSEL, FALSE, i);

                WCHAR szCount[64];
                swprintf_s(szCount, ARRAYSIZE(szCount), L"Selected: 0 of %d groups", count);
                SetDlgItemText(hwndDlg, IDC_14_SELECTED_COUNT, szCount);
            }
            return TRUE;
        }

        case IDC_14_GROUP_LIST:
        {
            // Update selection count when selection changes
            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                HWND hList = GetDlgItem(hwndDlg, IDC_14_GROUP_LIST);
                if (hList)
                {
                    int count = static_cast<int>(SendMessageW(hList, LB_GETCOUNT, 0, 0));
                    int selCount = 0;

                    for (int i = 0; i < count; i++)
                    {
                        if (SendMessageW(hList, LB_GETSEL, i, 0))
                            selCount++;
                    }

                    WCHAR szCount[64];
                    swprintf_s(szCount, ARRAYSIZE(szCount), L"Selected: %d of %d groups", selCount, count);
                    SetDlgItemText(hwndDlg, IDC_14_SELECTED_COUNT, szCount);
                }
            }
            return TRUE;
        }

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
