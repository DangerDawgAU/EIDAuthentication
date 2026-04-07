// DetailsDialog.h - User details dialog
#pragma once
#include "EIDManageUsers.h"

// Initialize details dialog for a user
void InitializeDetailsDialog(HWND hwndDlg, _In_ const UserInfo* pUser);

// Format user information for display
std::wstring FormatUserDetails(_In_ const UserInfo& user);
