// SettingsDialog.cpp - Settings dialog implementation
#include "SettingsDialog.h"
#include "RegistryConfig.h"
#include <windowsx.h>

// Store temporary settings
static std::vector<std::wstring> g_tempEIDGroups;
static BOOL g_tempConfirmActions = TRUE;
static BOOL g_tempShowWarnings = TRUE;

// Initialize settings dialog
void InitializeSettingsDialog(HWND hwndDlg)
{
    // Copy current settings to temp
    g_tempEIDGroups = g_appState.EIDGroups;
    g_tempConfirmActions = g_appState.fConfirmActions;
    g_tempShowWarnings = g_appState.fShowWarnings;

    // Update groups listbox
    UpdateGroupsList(hwndDlg);

    // Set checkboxes
    HWND hConfirm = GetDlgItem(hwndDlg, IDC_SETTINGS_CONFIRM);
    HWND hWarnings = GetDlgItem(hwndDlg, IDC_SETTINGS_WARNINGS);

    if (hConfirm)
        Button_SetCheck(hConfirm, g_tempConfirmActions ? BST_CHECKED : BST_UNCHECKED);
    if (hWarnings)
        Button_SetCheck(hWarnings, g_tempShowWarnings ? BST_CHECKED : BST_UNCHECKED);
}

// Save settings from dialog
void SaveSettingsDialog(HWND hwndDlg)
{
    // Get checkbox values
    HWND hConfirm = GetDlgItem(hwndDlg, IDC_SETTINGS_CONFIRM);
    HWND hWarnings = GetDlgItem(hwndDlg, IDC_SETTINGS_WARNINGS);

    if (hConfirm)
        g_tempConfirmActions = (Button_GetCheck(hConfirm) == BST_CHECKED);
    if (hWarnings)
        g_tempShowWarnings = (Button_GetCheck(hWarnings) == BST_CHECKED);

    // Apply to global state
    g_appState.EIDGroups = g_tempEIDGroups;
    g_appState.fConfirmActions = g_tempConfirmActions;
    g_appState.fShowWarnings = g_tempShowWarnings;

    // Save to registry
    SaveSettings();
}

// Update groups listbox
void UpdateGroupsList(HWND hwndDlg)
{
    HWND hList = GetDlgItem(hwndDlg, IDC_SETTINGS_GROUPS_LIST);
    if (!hList)
        return;

    ListBox_ResetContent(hList);

    for (const auto& group : g_tempEIDGroups)
    {
        ListBox_AddString(hList, group.c_str());
    }

    // Update count label
    HWND hCount = GetDlgItem(hwndDlg, IDC_SETTINGS_GROUPS_COUNT);
    if (hCount)
    {
        WCHAR szCount[64];
        swprintf_s(szCount, ARRAYSIZE(szCount),
            L"%zu group(s)", g_tempEIDGroups.size());
        SetWindowTextW(hCount, szCount);
    }
}

// Add a new EID-related group
void AddEIDGroup(HWND hwndDlg)
{
    // Get group name from edit box
    WCHAR szGroupName[256];
    szGroupName[0] = L'\0';

    HWND hEdit = GetDlgItem(hwndDlg, IDC_SETTINGS_GROUP_EDIT);
    if (hEdit)
    {
        GetWindowTextW(hEdit, szGroupName, ARRAYSIZE(szGroupName));
        if (szGroupName[0] != L'\0')
        {
            // Add to list
            g_tempEIDGroups.push_back(szGroupName);
            UpdateGroupsList(hwndDlg);
            SetWindowTextW(hEdit, L"");
        }
    }
    else
    {
        MessageBoxW(hwndDlg,
            L"Enter the group name in the edit box and click Add.",
            L"Add Group",
            MB_ICONINFORMATION);
    }
}

// Remove selected EID-related groups
void RemoveEIDGroup(HWND hwndDlg)
{
    HWND hList = GetDlgItem(hwndDlg, IDC_SETTINGS_GROUPS_LIST);
    if (!hList)
        return;

    // Get selected index
    int nSel = ListBox_GetCurSel(hList);
    if (nSel == LB_ERR)
    {
        MessageBoxW(hwndDlg,
            L"Please select a group to remove.",
            L"Remove Group",
            MB_ICONINFORMATION);
        return;
    }

    // Remove from vector
    if (nSel >= 0 && static_cast<size_t>(nSel) < g_tempEIDGroups.size())
    {
        g_tempEIDGroups.erase(g_tempEIDGroups.begin() + nSel);
        UpdateGroupsList(hwndDlg);
    }
}

// Settings dialog procedure
INT_PTR CALLBACK WndProc_Settings(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        SetWindowIcon(hwndDlg);
        InitializeSettingsDialog(hwndDlg);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_SETTINGS_GROUP_ADD:
            AddEIDGroup(hwndDlg);
            return TRUE;

        case IDC_SETTINGS_GROUP_REMOVE:
            RemoveEIDGroup(hwndDlg);
            return TRUE;

        case IDOK:
            SaveSettingsDialog(hwndDlg);
            EndDialog(hwndDlg, IDOK);
            return TRUE;

        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
        }
        break;

    default:
        break;
    }

    return FALSE;
}
