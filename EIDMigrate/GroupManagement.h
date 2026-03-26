#pragma once

// File: EIDMigrate/GroupManagement.h
// Local group management functions

#include "EIDMigrate.h"
#include <LM.h>
#include <vector>
#include <set>

// Built-in local groups to recognize
namespace BuiltinGroups
{
    constexpr const wchar_t* ADMINISTRATORS = L"Administrators";
    constexpr const wchar_t* USERS = L"Users";
    constexpr const wchar_t* GUESTS = L"Guests";
    constexpr const wchar_t* POWER_USERS = L"Power Users";
    constexpr const wchar_t* REMOTE_DESKTOP_USERS = L"Remote Desktop Users";
    constexpr const wchar_t* REMOTE_MANAGEMENT_USERS = L"Remote Management Users";

    BOOL IsBuiltin(_In_ const std::wstring& wsGroupName);
    BOOL IsBuiltinSid(_In_ PSID pSid);
}

// Group information structure
struct LocalGroupInfo
{
    std::wstring wsName;
    std::wstring wsComment;
    std::vector<std::wstring> wsMembers;  // Username strings
    SID_NAME_USE sidUse;
    std::vector<BYTE> sidBytes;
    BOOL fBuiltin;

    LocalGroupInfo() :
        sidUse(SidTypeInvalid),
        fBuiltin(FALSE)
    {}
};

// Check if group exists
HRESULT GroupExists(
    _In_ const std::wstring& wsGroupName,
    _Out_ BOOL& pfExists);

// Get group information
HRESULT GetGroupInfo(
    _In_ const std::wstring& wsGroupName,
    _Out_ LocalGroupInfo& info);

// Create local group
HRESULT CreateLocalGroup(
    _In_ const std::wstring& wsGroupName,
    _In_ const std::wstring& wsComment);

// Set group comment
HRESULT SetGroupComment(
    _In_ const std::wstring& wsGroupName,
    _In_ const std::wstring& wsComment);

// Add user to group
HRESULT AddUserToGroup(
    _In_ const std::wstring& wsUsername,
    _In_ const std::wstring& wsGroupName);

// Remove user from group
HRESULT RemoveUserFromGroup(
    _In_ const std::wstring& wsUsername,
    _In_ const std::wstring& wsGroupName);

// Get group members
HRESULT GetGroupMembers(
    _In_ const std::wstring& wsGroupName,
    _Out_ std::vector<std::wstring>& wsMembers);

// Enumerate all local groups
HRESULT EnumerateLocalGroups(
    _Out_ std::vector<LocalGroupInfo>& groups);

// Enumerate groups for a specific user
HRESULT GetUserGroups(
    _In_ const std::wstring& wsUsername,
    _Out_ std::vector<std::wstring>& wsGroupNames);

// Synchronize group memberships (add/remove as needed)
HRESULT SynchronizeGroupMemberships(
    _In_ const std::wstring& wsUsername,
    _In_ const std::vector<std::wstring>& wsTargetGroups,
    _Out_ DWORD& pdwChanges);
