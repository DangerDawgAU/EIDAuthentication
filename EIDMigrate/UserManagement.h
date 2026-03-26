#pragma once

// File: EIDMigrate/UserManagement.h
// User account management functions

#include "EIDMigrate.h"
#include <LM.h>
#include <vector>

// User information structure
struct UserInfo
{
    std::wstring wsUsername;
    std::wstring wsFullName;
    std::wstring wsComment;
    DWORD dwRid;
    std::wstring wsSid;
    DWORD dwAccountId;  // USER_ACCOUNT_ID from NetUserGetInfo
    BOOL fEnabled;
    BOOL fPasswordExpired;

    UserInfo() :
        dwRid(0),
        dwAccountId(0),
        fEnabled(TRUE),
        fPasswordExpired(FALSE)
    {}
};

// Check if user account exists
HRESULT UserExists(
    _In_ const std::wstring& wsUsername,
    _Out_ BOOL& pfExists);

// Get user information
HRESULT GetUserInfo(
    _In_ const std::wstring& wsUsername,
    _Out_ UserInfo& info);

// Get user RID
HRESULT GetUserRid(
    _In_ const std::wstring& wsUsername,
    _Out_ DWORD& pdwRid);

// Get user SID
HRESULT GetUserSid(
    _In_ const std::wstring& wsUsername,
    _Out_ std::wstring& wsSid);

// Create local user account
HRESULT CreateLocalUserAccount(
    _In_ const std::wstring& wsUsername,
    _In_ const std::wstring& wsFullName,
    _In_ const std::wstring& wsComment,
    _In_opt_ PCWSTR pwszPassword,
    _In_ BOOL fEnabled,
    _In_ BOOL fPasswordNeverExpires);

// Set user password
HRESULT SetUserPassword(
    _In_ const std::wstring& wsUsername,
    _In_ PCWSTR pwszPassword);

// Enable/disable user account
HRESULT SetUserEnabled(
    _In_ const std::wstring& wsUsername,
    _In_ BOOL fEnabled);

// Set "password never expires" flag
HRESULT SetUserPasswordNeverExpires(
    _In_ const std::wstring& wsUsername,
    _In_ BOOL fNeverExpires);

// Enumerate local users
HRESULT EnumerateLocalUsers(
    _Out_ std::vector<UserInfo>& users);

// Display user information
void DisplayUserInfo(_In_ const UserInfo& info);
