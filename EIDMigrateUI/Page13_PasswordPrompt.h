// Page13_PasswordPrompt.h - Password Prompt Dialog Header
#pragma once

#include <Windows.h>
#include <string>

// Show password prompt dialog for a user account
// Parameters:
//   hwndParent - Parent window handle
//   wsUsername - Username to display
//   wsPassword - Output password if set
//   pfSkipped - Output TRUE if user skipped
// Returns:
//   TRUE if password was set, FALSE if skipped or cancelled
BOOL ShowPasswordPromptDialog(
    _In_ HWND hwndParent,
    _In_ const std::wstring& wsUsername,
    _Out_ std::wstring& wsPassword,
    _Out_ BOOL& pfSkipped);

// Set password for a local user account
HRESULT SetUserPassword(
    _In_ const std::wstring& wsUsername,
    _In_ const std::wstring& wsPassword);
