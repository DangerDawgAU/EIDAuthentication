// UserManagement.cpp - User account and profile management
#include "UserManagement.h"
#include "../EIDMigrate/Tracing.h"
#include <lm.h>
#include <userenv.h>
#include <sddl.h>
#include <vector>

#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "userenv.lib")

// Delete user account
HRESULT DeleteUserAccount(_In_ const std::wstring& wsUsername)
{
    if (wsUsername.empty())
        return E_INVALIDARG;

    NET_API_STATUS status = NetUserDel(nullptr, wsUsername.c_str());

    if (status == NERR_Success)
        return S_OK;
    else if (status == NERR_UserNotFound)
        return S_FALSE;
    else
        return HRESULT_FROM_WIN32(status);
}

// Get user profile directory
HRESULT GetUserProfilePath(_In_ const std::wstring& wsUsername, _Out_ std::wstring& wsProfilePath)
{
    wsProfilePath.clear();

    // Get user SID
    DWORD dwSidSize = 0;
    DWORD dwDomainSize = 0;
    SID_NAME_USE use;

    LookupAccountNameW(nullptr, wsUsername.c_str(),
        nullptr, &dwSidSize,
        nullptr, &dwDomainSize, &use);

    if (dwSidSize == 0)
        return HRESULT_FROM_WIN32(GetLastError());

    std::vector<BYTE> sidBuffer(dwSidSize);
    std::vector<WCHAR> domainBuffer(dwDomainSize);

    if (!LookupAccountNameW(nullptr, wsUsername.c_str(),
        reinterpret_cast<PSID>(sidBuffer.data()), &dwSidSize,
        domainBuffer.data(), &dwDomainSize, &use))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Get profile directory
    WCHAR szProfilePath[MAX_PATH];
    DWORD dwPathSize = MAX_PATH;
    HRESULT hr = GetProfilesDirectoryW(szProfilePath, &dwPathSize);
    if (FAILED(hr))
        return hr;

    // Convert SID to string for folder name
    LPWSTR pwszSid = nullptr;
    if (!ConvertSidToStringSidW(reinterpret_cast<PSID>(sidBuffer.data()), &pwszSid))
        return HRESULT_FROM_WIN32(GetLastError());

    wsProfilePath = szProfilePath;
    if (wsProfilePath.back() != L'\\')
        wsProfilePath += L'\\';
    wsProfilePath += pwszSid;

    LocalFree(pwszSid);
    return S_OK;
}

// Delete user profile directory
HRESULT DeleteUserProfile(_In_ const std::wstring& wsUsername)
{
    std::wstring wsProfilePath;
    HRESULT hr = GetUserProfilePath(wsUsername, wsProfilePath);
    if (FAILED(hr))
        return hr;

    // Check if profile directory exists
    DWORD dwAttrib = GetFileAttributesW(wsProfilePath.c_str());
    if (dwAttrib == INVALID_FILE_ATTRIBUTES)
    {
        DWORD dwError = GetLastError();
        if (dwError == ERROR_FILE_NOT_FOUND)
            return S_FALSE;  // No profile to delete
        return HRESULT_FROM_WIN32(dwError);
    }

    if (!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
        return HRESULT_FROM_WIN32(ERROR_DIRECTORY);

    // Delete the profile directory and all contents
    // Use SHFileOperation for better deletion handling
    SHFILEOPSTRUCTW fileOp = {0};
    fileOp.wFunc = FO_DELETE;
    fileOp.pFrom = wsProfilePath.c_str();
    fileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

    // Double-null terminate for SHFileOperation
    std::vector<WCHAR> doubleNullPath(wsProfilePath.begin(), wsProfilePath.end());
    doubleNullPath.push_back(0);
    doubleNullPath.push_back(0);
    fileOp.pFrom = doubleNullPath.data();

    int nResult = SHFileOperationW(&fileOp);
    if (nResult == 0)
        return S_OK;
    else
        return HRESULT_FROM_WIN32(nResult);
}

// Remove user from groups
HRESULT RemoveUserFromGroups(_In_ const std::wstring& wsUsername,
    _In_ const std::vector<std::wstring>& groupsToRemove)
{
    if (wsUsername.empty() || groupsToRemove.empty())
        return E_INVALIDARG;

    HRESULT hrOverall = S_OK;
    DWORD dwFailedCount = 0;

    for (const auto& wsGroup : groupsToRemove)
    {
        LOCALGROUP_MEMBERS_INFO_0 memberInfo;
        memberInfo.lgrmi0_sid = nullptr;

        // Get user SID
        DWORD dwSidSize = 0;
        DWORD dwDomainSize = 0;
        SID_NAME_USE use;

        LookupAccountNameW(nullptr, wsUsername.c_str(),
            nullptr, &dwSidSize,
            nullptr, &dwDomainSize, &use);

        if (dwSidSize == 0)
            continue;

        std::vector<BYTE> sidBuffer(dwSidSize);
        std::vector<WCHAR> domainBuffer(dwDomainSize);

        if (!LookupAccountNameW(nullptr, wsUsername.c_str(),
            reinterpret_cast<PSID>(sidBuffer.data()), &dwSidSize,
            domainBuffer.data(), &dwDomainSize, &use))
        {
            dwFailedCount++;
            continue;
        }

        memberInfo.lgrmi0_sid = reinterpret_cast<PSID>(sidBuffer.data());

        NET_API_STATUS status = NetLocalGroupDelMembers(nullptr,
            const_cast<LPWSTR>(wsGroup.c_str()),
            0,
            reinterpret_cast<LPBYTE>(&memberInfo),
            1);

        if (status != NERR_Success && status != 2314)  // 2314 = NERR_MemberNotInGroup
        {
            dwFailedCount++;
        }
    }

    if (dwFailedCount > 0)
        hrOverall = S_FALSE;

    return hrOverall;
}

// Get user groups
HRESULT GetUserGroups(_In_ const std::wstring& wsUsername,
    _Out_ std::vector<std::wstring>& groups)
{
    groups.clear();

    // Get user SID
    DWORD dwSidSize = 0;
    DWORD dwDomainSize = 0;
    SID_NAME_USE use;

    LookupAccountNameW(nullptr, wsUsername.c_str(),
        nullptr, &dwSidSize,
        nullptr, &dwDomainSize, &use);

    if (dwSidSize == 0)
        return HRESULT_FROM_WIN32(GetLastError());

    std::vector<BYTE> sidBuffer(dwSidSize);
    std::vector<WCHAR> domainBuffer(dwDomainSize);

    if (!LookupAccountNameW(nullptr, wsUsername.c_str(),
        reinterpret_cast<PSID>(sidBuffer.data()), &dwSidSize,
        domainBuffer.data(), &dwDomainSize, &use))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    PSID pSid = reinterpret_cast<PSID>(sidBuffer.data());

    // Enumerate local groups
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    PLOCALGROUP_USERS_INFO_0 pGroups = nullptr;

    NET_API_STATUS status = NetUserGetLocalGroups(nullptr,
        const_cast<LPWSTR>(wsUsername.c_str()),
        0,
        LG_INCLUDE_INDIRECT,
        reinterpret_cast<LPBYTE*>(&pGroups),
        MAX_PREFERRED_LENGTH,
        &dwEntriesRead,
        &dwTotalEntries);

    if (status == NERR_Success)
    {
        for (DWORD i = 0; i < dwEntriesRead; i++)
        {
            if (pGroups[i].lgrui0_name)
            {
                groups.push_back(pGroups[i].lgrui0_name);
            }
        }
        NetApiBufferFree(pGroups);
    }

    return S_OK;
}

// Check if user is logged on
BOOL IsUserLoggedIn(_In_ const std::wstring& wsUsername)
{
    // Enumerate sessions to check if user is logged on
    // This is a simplified check - a full implementation would use
    // WTSEnumerateSessions or similar

    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return FALSE;

    // For now, return FALSE (assume not logged on)
    // A full implementation would query WTS or check session state
    if (hToken)
        CloseHandle(hToken);

    return FALSE;
}

// Enumerate all local users
HRESULT EnumerateLocalUsers(_Out_ std::vector<std::wstring>& users)
{
    users.clear();

    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    LPUSER_INFO_0 pUserInfoArray = nullptr;

    NET_API_STATUS status = NetUserEnum(nullptr, 0, FILTER_NORMAL_ACCOUNT,
        reinterpret_cast<LPBYTE*>(&pUserInfoArray),
        MAX_PREFERRED_LENGTH,
        &dwEntriesRead,
        &dwTotalEntries,
        nullptr);

    if (status != NERR_Success)
        return HRESULT_FROM_WIN32(status);

    for (DWORD i = 0; i < dwEntriesRead; i++)
    {
        if (pUserInfoArray[i].usri0_name)
        {
            users.push_back(pUserInfoArray[i].usri0_name);
        }
    }

    NetApiBufferFree(pUserInfoArray);
    return S_OK;
}
