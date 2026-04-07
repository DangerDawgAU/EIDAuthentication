// ManageDialog.h - Manage selected users dialog
#pragma once
#include "EIDManageUsers.h"

// Initialize manage dialog
void InitializeManageDialog(HWND hwndDlg, _In_ const std::vector<UserInfo>* pUsers);

// Execute selected management operations
HRESULT ExecuteManagementOperations(HWND hwndDlg, _In_ const std::vector<UserInfo>& users);

// Show confirmation dialog with details
BOOL ShowManagementConfirmation(HWND hwndDlg,
    _In_ const std::vector<UserInfo>& users,
    DWORD dwOperations);

// Update operation checkboxes based on selections
void UpdateOperationDisplay(HWND hwndDlg);
