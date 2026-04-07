// MainPage.h - Main page UI functions
#pragma once
#include "EIDManageUsers.h"

// Initialize the main page controls
void InitializeMainPage(HWND hwndDlg);

// Refresh the user list based on current filter
HRESULT RefreshUserList(HWND hwndDlg);

// Populate the list view with users
void PopulateUserList(HWND hList, _In_ const std::vector<UserInfo>& users);

// Show details dialog for selected user
void ShowDetailsDialog(HWND hwndParent);

// Show manage dialog for selected users
void ShowManageDialog(HWND hwndParent);

// Get currently selected users from list view
HRESULT GetSelectedUsers(HWND hList, _Out_ std::vector<UserInfo>& selectedUsers);

// Update filter radio buttons
void UpdateFilterControls(HWND hwndDlg, FilterMode filter);

// Helper: Get encryption type string
PCWSTR GetEncryptionTypeString(EID_PRIVATE_DATA_TYPE type);
