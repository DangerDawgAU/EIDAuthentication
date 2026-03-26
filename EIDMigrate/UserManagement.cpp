// File: EIDMigrate/UserManagement.cpp
// User account management functions

#include "UserManagement.h"
#include "LsaClient.h"
#include "Tracing.h"
#include <lm.h>

#pragma comment(lib, "netapi32.lib")

HRESULT UserExists(_In_ const std::wstring& wsUsername, _Out_ BOOL& pfExists)
{
    USER_INFO_0* pInfo = nullptr;
    DWORD dwError = 0;

    pfExists = FALSE;

    NET_API_STATUS status = NetUserGetInfo(nullptr, wsUsername.c_str(), 0,
        reinterpret_cast<LPBYTE*>(&pInfo));

    if (status == NERR_Success)
    {
        pfExists = TRUE;
        NetApiBufferFree(pInfo);
        return S_OK;
    }
    else if (status == NERR_UserNotFound)
    {
        return S_OK;
    }

    return HRESULT_FROM_WIN32(status);
}

HRESULT GetUserInfo(_In_ const std::wstring& wsUsername, _Out_ UserInfo& info)
{
    USER_INFO_1* pInfo = nullptr;
    DWORD dwError = 0;

    NET_API_STATUS status = NetUserGetInfo(nullptr, wsUsername.c_str(), 1,
        reinterpret_cast<LPBYTE*>(&pInfo));

    if (status != NERR_Success)
        return HRESULT_FROM_WIN32(status);

    info.wsUsername = wsUsername;
    info.dwAccountId = 0;  // Not directly available in USER_INFO_1
    info.fEnabled = !(pInfo->usri1_flags & UF_ACCOUNTDISABLE);

    NetApiBufferFree(pInfo);

    // Get RID
    DWORD dwRid = 0;
    HRESULT hr = GetUserRid(wsUsername, dwRid);
    if (SUCCEEDED(hr))
    {
        info.dwRid = dwRid;
    }
    else
    {
        info.dwRid = 0;
    }

    // Get SID
    std::wstring wsSid;
    hr = GetUserSid(wsUsername, wsSid);
    if (SUCCEEDED(hr))
    {
        info.wsSid = wsSid;
    }
    else
    {
        info.wsSid.clear();
    }

    return S_OK;
}

HRESULT GetUserRid(_In_ const std::wstring& wsUsername, _Out_ DWORD& pdwRid)
{
    pdwRid = LookupRidByUsername(wsUsername);
    return (pdwRid != 0) ? S_OK : E_FAIL;
}

HRESULT GetUserSid(_In_ const std::wstring& wsUsername, _Out_ std::wstring& wsSid)
{
    wsSid = LookupSidByUsername(wsUsername);
    return (!wsSid.empty()) ? S_OK : E_FAIL;
}

HRESULT CreateLocalUserAccount(
    _In_ const std::wstring& wsUsername,
    _In_ const std::wstring& wsFullName,
    _In_ const std::wstring& wsComment,
    _In_opt_ PCWSTR pwszPassword,
    _In_ BOOL fEnabled,
    _In_ BOOL fPasswordNeverExpires)
{
    USER_INFO_1 ui1 = {};
    ui1.usri1_name = const_cast<LPWSTR>(wsUsername.c_str());
    ui1.usri1_password = const_cast<LPWSTR>((pwszPassword) ? pwszPassword : L"");
    ui1.usri1_priv = USER_PRIV_USER;
    ui1.usri1_home_dir = nullptr;
    ui1.usri1_comment = const_cast<LPWSTR>(wsComment.c_str());
    ui1.usri1_flags = UF_SCRIPT | UF_NORMAL_ACCOUNT |
        (fEnabled ? 0 : UF_ACCOUNTDISABLE) |
        (fPasswordNeverExpires ? UF_DONT_EXPIRE_PASSWD : 0);
    ui1.usri1_script_path = nullptr;

    NET_API_STATUS status = NetUserAdd(nullptr, 1,
        reinterpret_cast<LPBYTE>(&ui1), nullptr);

    return HRESULT_FROM_WIN32(status);
}

HRESULT SetUserPassword(_In_ const std::wstring& wsUsername, _In_ PCWSTR pwszPassword)
{
    USER_INFO_1003 ui1003 = {};
    ui1003.usri1003_password = const_cast<LPWSTR>(pwszPassword);

    NET_API_STATUS status = NetUserSetInfo(nullptr, wsUsername.c_str(),
        1003, reinterpret_cast<LPBYTE>(&ui1003), nullptr);

    return HRESULT_FROM_WIN32(status);
}

HRESULT SetUserEnabled(_In_ const std::wstring& wsUsername, _In_ BOOL fEnabled)
{
    // First get current flags to preserve them
    USER_INFO_1* pInfo = nullptr;
    NET_API_STATUS status = NetUserGetInfo(nullptr, wsUsername.c_str(), 1,
        reinterpret_cast<LPBYTE*>(&pInfo));

    if (status != NERR_Success)
        return HRESULT_FROM_WIN32(status);

    // Modify only the disabled flag
    DWORD dwFlags = pInfo->usri1_flags;
    if (fEnabled)
        dwFlags &= ~UF_ACCOUNTDISABLE;  // Clear the disable flag
    else
        dwFlags |= UF_ACCOUNTDISABLE;   // Set the disable flag

    NetApiBufferFree(pInfo);

    USER_INFO_1008 ui1008 = {};
    ui1008.usri1008_flags = dwFlags;

    status = NetUserSetInfo(nullptr, wsUsername.c_str(),
        1008, reinterpret_cast<LPBYTE>(&ui1008), nullptr);

    return HRESULT_FROM_WIN32(status);
}

HRESULT SetUserPasswordNeverExpires(_In_ const std::wstring& wsUsername, _In_ BOOL fNeverExpires)
{
    // First get current flags
    USER_INFO_1* pInfo = nullptr;
    NET_API_STATUS status = NetUserGetInfo(nullptr, wsUsername.c_str(), 1,
        reinterpret_cast<LPBYTE*>(&pInfo));

    if (status != NERR_Success)
        return HRESULT_FROM_WIN32(status);

    DWORD dwFlags = pInfo->usri1_flags;
    if (fNeverExpires)
        dwFlags |= UF_DONT_EXPIRE_PASSWD;
    else
        dwFlags &= ~UF_DONT_EXPIRE_PASSWD;

    NetApiBufferFree(pInfo);

    USER_INFO_1008 ui1008 = {};
    ui1008.usri1008_flags = dwFlags;

    status = NetUserSetInfo(nullptr, wsUsername.c_str(),
        1008, reinterpret_cast<LPBYTE>(&ui1008), nullptr);

    return HRESULT_FROM_WIN32(status);
}

HRESULT EnumerateLocalUsers(_Out_ std::vector<UserInfo>& users)
{
    LPUSER_INFO_0 pBuf = nullptr;
    DWORD dwEntriesRead = 0;
    DWORD dwTotalEntries = 0;
    NET_API_STATUS status;

    status = NetUserEnum(nullptr, 0, FILTER_NORMAL_ACCOUNT,
        reinterpret_cast<LPBYTE*>(&pBuf),
        MAX_PREFERRED_LENGTH,
        &dwEntriesRead,
        &dwTotalEntries,
        nullptr);

    if (status != NERR_Success)
        return HRESULT_FROM_WIN32(status);

    for (DWORD i = 0; i < dwEntriesRead; i++)
    {
        UserInfo info;
        if (SUCCEEDED(GetUserInfo(pBuf[i].usri0_name, info)))
        {
            users.push_back(info);
        }
    }

    NetApiBufferFree(pBuf);
    return S_OK;
}

void DisplayUserInfo(_In_ const UserInfo& info)
{
    EIDM_TRACE_INFO(L"  User: %ls", info.wsUsername.c_str());
    EIDM_TRACE_INFO(L"    RID: %u", info.dwRid);
    EIDM_TRACE_INFO(L"    SID: %ls", info.wsSid.c_str());
    EIDM_TRACE_INFO(L"    Enabled: %ls", info.fEnabled ? L"Yes" : L"No");
}
