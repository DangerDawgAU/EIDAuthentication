// MainPage.cpp - Main page implementation
#include "MainPage.h"
#include "EventLogReader.h"
#include "EIDUserManagement.h"
#include "../EIDMigrate/LsaClient.h"
#include <lm.h>
#include <windowsx.h>
#include <commctrl.h>

#pragma comment(lib, "netapi32.lib")

// Helper: Get encryption type as display string
PCWSTR GetEncryptionTypeString(EID_PRIVATE_DATA_TYPE type)
{
    switch (type)
    {
    case EID_PRIVATE_DATA_TYPE::eidpdtCrypted:
        return L"Certificate";
    case EID_PRIVATE_DATA_TYPE::eidpdtDPAPI:
        return L"DPAPI";
    case EID_PRIVATE_DATA_TYPE::eidpdtClearText:
        return L"None";
    default:
        return L"Unknown";
    }
}

// Initialize main page controls
void InitializeMainPage(HWND hwndDlg)
{
    // Get list view control
    HWND hList = GetDlgItem(hwndDlg, IDC_MAIN_LIST);
    if (!hList)
        return;

    // Set list view extended styles
    ListView_SetExtendedListViewStyle(hList,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);

    // Add columns
    LVCOLUMN lvc = {0};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;

    // Column 1: Username
    lvc.pszText = const_cast<LPWSTR>(L"Username");
    lvc.cx = 150;
    ListView_InsertColumn(hList, 0, &lvc);

    // Column 2: RID
    lvc.pszText = const_cast<LPWSTR>(L"RID");
    lvc.cx = 60;
    ListView_InsertColumn(hList, 1, &lvc);

    // Column 3: Encryption
    lvc.pszText = const_cast<LPWSTR>(L"Encryption");
    lvc.cx = 100;
    ListView_InsertColumn(hList, 2, &lvc);

    // Column 4: Last Login
    lvc.pszText = const_cast<LPWSTR>(L"Last Login");
    lvc.cx = 140;
    ListView_InsertColumn(hList, 3, &lvc);

    // Set default filter (EID only)
    HWND hFilterEID = GetDlgItem(hwndDlg, IDC_MAIN_FILTER_EID);
    HWND hFilterAll = GetDlgItem(hwndDlg, IDC_MAIN_FILTER_ALL);

    if (hFilterEID)
        Button_SetCheck(hFilterEID, BST_CHECKED);
    if (hFilterAll)
        Button_SetCheck(hFilterAll, BST_UNCHECKED);

    // Handle filter radio button changes
    HWND hFilter = GetDlgItem(hwndDlg, IDC_MAIN_FILTER_EID);
    if (hFilter)
    {
        // Subclass for radio button handling is done via WM_COMMAND in parent
    }

    // Initial refresh
    RefreshUserList(hwndDlg);
}

// Refresh user list
HRESULT RefreshUserList(HWND hwndDlg)
{
    HWND hList = GetDlgItem(hwndDlg, IDC_MAIN_LIST);
    if (!hList)
        return E_INVALIDARG;

    ListView_DeleteAllItems(hList);
    g_appState.users.clear();

    // Enumerate local users
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    LPUSER_INFO_0 pUserInfoArray = nullptr;

    DWORD dwNetStatus = NetUserEnum(nullptr, 0, FILTER_NORMAL_ACCOUNT,
        reinterpret_cast<LPBYTE*>(&pUserInfoArray),
        MAX_PREFERRED_LENGTH,
        &dwEntriesRead,
        &dwTotalEntries,
        nullptr);

    if (dwNetStatus != NERR_Success)
    {
        MessageBoxW(hwndDlg,
            (std::wstring(L"Failed to enumerate users.\n\nError: ") +
            std::to_wstring(dwNetStatus)).c_str(),
            EIDMANAGE_APP_NAME,
            MB_ICONERROR);
        return HRESULT_FROM_WIN32(dwNetStatus);
    }

    // Process each user
    for (DWORD i = 0; i < dwEntriesRead; i++)
    {
        if (!pUserInfoArray[i].usri0_name)
            continue;

        UserInfo info;
        info.wsUsername = pUserInfoArray[i].usri0_name;

        // Get SID
        info.wsSid = LookupSidByUsername(info.wsUsername);

        // Get RID
        info.dwRid = LookupRidByUsername(info.wsUsername);

        // Check for EID credential
        BOOL fHasCred = FALSE;
        HRESULT hr = HasStoredCredential(info.dwRid, fHasCred);
        if (SUCCEEDED(hr))
        {
            info.fHasEIDCredential = fHasCred;
        }

        // Get last login from event log
        FILETIME ftLastLogin = {0};
        if (SUCCEEDED(GetLastEIDLoginTime(info.wsUsername, ftLastLogin)))
        {
            info.wsLastLogin = FormatFileTime(ftLastLogin);
        }
        else
        {
            info.wsLastLogin = L"Never";
        }

        // Get encryption type if has credential
        if (info.fHasEIDCredential)
        {
            CredentialInfo credInfo;
            if (SUCCEEDED(ExportLsaCredential(info.dwRid, credInfo)))
            {
                info.EncryptionType = credInfo.EncryptionType;
                memcpy_s(info.CertificateHash, sizeof(info.CertificateHash),
                    credInfo.CertificateHash, CERT_HASH_LENGTH);
            }
        }

        // Get user groups
        GetUserGroups(info.wsUsername, info.wsGroups);

        // Apply filter
        BOOL fInclude = FALSE;
        if (g_appState.currentFilter == FILTER_EID_ONLY)
        {
            fInclude = info.fHasEIDCredential;
        }
        else
        {
            fInclude = TRUE;
        }

        if (fInclude)
        {
            g_appState.users.push_back(info);
        }
    }

    NetApiBufferFree(pUserInfoArray);

    // Populate list view
    PopulateUserList(hList, g_appState.users);

    // Update selection count label
    HWND hCount = GetDlgItem(hwndDlg, IDC_MAIN_SELECTED_COUNT);
    if (hCount)
    {
        SetWindowTextW(hCount,
            (std::to_wstring(g_appState.users.size()) + L" users").c_str());
    }

    return S_OK;
}

// Populate list view
void PopulateUserList(HWND hList, _In_ const std::vector<UserInfo>& users)
{
    ListView_DeleteAllItems(hList);

    for (size_t i = 0; i < users.size(); i++)
    {
        const UserInfo& user = users[i];

        LVITEM lvi = {0};
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.pszText = const_cast<LPWSTR>(user.wsUsername.c_str());
        lvi.iItem = static_cast<int>(i);
        lvi.lParam = static_cast<LPARAM>(i);
        int iItem = ListView_InsertItem(hList, &lvi);

        // RID
        WCHAR szRID[32];
        swprintf_s(szRID, ARRAYSIZE(szRID), L"%u", user.dwRid);
        ListView_SetItemText(hList, iItem, 1, szRID);

        // Encryption type
        ListView_SetItemText(hList, iItem, 2,
            const_cast<LPWSTR>(GetEncryptionTypeString(user.EncryptionType)));

        // Last login
        ListView_SetItemText(hList, iItem, 3,
            const_cast<LPWSTR>(user.wsLastLogin.c_str()));

        // Auto-check EID-enabled users
        ListView_SetCheckState(hList, iItem, user.fHasEIDCredential);
    }
}

// Get selected users
HRESULT GetSelectedUsers(HWND hList, _Out_ std::vector<UserInfo>& selectedUsers)
{
    selectedUsers.clear();

    int nCount = ListView_GetItemCount(hList);
    for (int i = 0; i < nCount; i++)
    {
        if (ListView_GetCheckState(hList, i))
        {
            LVITEM lvi = {0};
            lvi.mask = LVIF_PARAM;
            lvi.iItem = i;
            if (ListView_GetItem(hList, &lvi))
            {
                size_t index = static_cast<size_t>(lvi.lParam);
                if (index < g_appState.users.size())
                {
                    selectedUsers.push_back(g_appState.users[index]);
                }
            }
        }
    }

    return selectedUsers.empty() ? S_FALSE : S_OK;
}

// Update filter controls
void UpdateFilterControls(HWND hwndDlg, FilterMode filter)
{
    HWND hFilterEID = GetDlgItem(hwndDlg, IDC_MAIN_FILTER_EID);
    HWND hFilterAll = GetDlgItem(hwndDlg, IDC_MAIN_FILTER_ALL);

    if (hFilterEID)
        Button_SetCheck(hFilterEID,
            (filter == FILTER_EID_ONLY) ? BST_CHECKED : BST_UNCHECKED);
    if (hFilterAll)
        Button_SetCheck(hFilterAll,
            (filter == FILTER_ALL_USERS) ? BST_CHECKED : BST_UNCHECKED);

    g_appState.currentFilter = filter;
}

// Show details dialog
void ShowDetailsDialog(HWND hwndParent)
{
    HWND hList = GetDlgItem(hwndParent, IDC_MAIN_LIST);
    if (!hList)
        return;

    // Get selected item
    int iItem = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    if (iItem == -1)
    {
        MessageBoxW(hwndParent,
            L"Please select a user to view details.",
            EIDMANAGE_APP_NAME,
            MB_ICONINFORMATION);
        return;
    }

    LVITEM lvi = {0};
    lvi.mask = LVIF_PARAM;
    lvi.iItem = iItem;
    if (!ListView_GetItem(hList, &lvi))
        return;

    size_t index = static_cast<size_t>(lvi.lParam);
    if (index >= g_appState.users.size())
        return;

    const UserInfo& user = g_appState.users[index];

    // Show details dialog
    DialogBoxParamW(g_hinst, MAKEINTRESOURCE(IDD_DETAILS),
        hwndParent, WndProc_Details,
        reinterpret_cast<LPARAM>(&user));
}

// Show manage dialog
void ShowManageDialog(HWND hwndParent)
{
    HWND hList = GetDlgItem(hwndParent, IDC_MAIN_LIST);
    if (!hList)
        return;

    std::vector<UserInfo> selectedUsers;
    HRESULT hr = GetSelectedUsers(hList, selectedUsers);

    if (hr == S_FALSE || selectedUsers.empty())
    {
        MessageBoxW(hwndParent,
            L"Please select at least one user to manage.",
            EIDMANAGE_APP_NAME,
            MB_ICONINFORMATION);
        return;
    }

    // Show manage dialog
    DialogBoxParamW(g_hinst, MAKEINTRESOURCE(IDD_MANAGE),
        hwndParent, WndProc_Manage,
        reinterpret_cast<LPARAM>(&selectedUsers));

    // Refresh list after operations
    RefreshUserList(hwndParent);
}
