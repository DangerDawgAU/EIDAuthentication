// UserManagement.h - User account and profile management operations
#pragma once
#include <Windows.h>
#include <string>
#include <vector>

// Delete a user account
HRESULT DeleteUserAccount(_In_ const std::wstring& wsUsername);

// Delete user profile directory
HRESULT DeleteUserProfile(_In_ const std::wstring& wsUsername);

// Get user profile directory
HRESULT GetUserProfilePath(_In_ const std::wstring& wsUsername, _Out_ std::wstring& wsProfilePath);

// Remove user from specified groups
HRESULT RemoveUserFromGroups(_In_ const std::wstring& wsUsername,
    _In_ const std::vector<std::wstring>& groupsToRemove);

// Get list of groups a user belongs to
HRESULT GetUserGroups(_In_ const std::wstring& wsUsername,
    _Out_ std::vector<std::wstring>& groups);

// Check if user is currently logged on
BOOL IsUserLoggedIn(_In_ const std::wstring& wsUsername);

// Get all local users
HRESULT EnumerateLocalUsers(_Out_ std::vector<std::wstring>& users);
