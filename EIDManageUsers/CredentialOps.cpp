// CredentialOps.cpp - LSA credential operations
#include "CredentialOps.h"
#include "../EIDMigrate/LsaClient.h"
#include "../EIDCardLibrary/StoredCredentialManagement.h"
#include "../EIDCardLibrary/Package.h"
#include "../EIDMigrate/Tracing.h"

// Remove EID credential by RID
HRESULT RemoveEIDCredential(_In_ DWORD dwRid)
{
    CStoredCredentialManager* pManager = CStoredCredentialManager::Instance();
    if (!pManager)
        return E_NOINTERFACE;

    if (pManager->RemoveStoredCredential(dwRid))
    {
        EIDM_TRACE_INFO(L"Removed EID credential for RID %u", dwRid);
        return S_OK;
    }

    return E_FAIL;
}

// Remove EID credential by username
HRESULT RemoveEIDCredential(_In_ const std::wstring& wsUsername)
{
    if (wsUsername.empty())
        return E_INVALIDARG;

    // Get RID from username
    DWORD dwRid = LookupRidByUsername(wsUsername);
    if (dwRid == 0)
    {
        EIDM_TRACE_ERROR(L"Failed to get RID for username '%ls'", wsUsername.c_str());
        return HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER);
    }

    return RemoveEIDCredential(dwRid);
}

// Check if user has EID credential by RID
HRESULT HasEIDCredential(_In_ DWORD dwRid, _Out_ BOOL& pfHasCredential)
{
    pfHasCredential = FALSE;

    CStoredCredentialManager* pManager = CStoredCredentialManager::Instance();
    if (!pManager)
        return E_NOINTERFACE;

    pfHasCredential = pManager->HasStoredCredential(dwRid);
    return S_OK;
}

// Check if user has EID credential by username
HRESULT HasEIDCredential(_In_ const std::wstring& wsUsername, _Out_ BOOL& pfHasCredential)
{
    if (wsUsername.empty())
        return E_INVALIDARG;

    DWORD dwRid = LookupRidByUsername(wsUsername);
    if (dwRid == 0)
    {
        pfHasCredential = FALSE;
        return HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER);
    }

    return HasEIDCredential(dwRid, pfHasCredential);
}

// Get credential info for a user
HRESULT GetCredentialInfo(_In_ DWORD dwRid, _Out_ CredentialInfo& info)
{
    return ExportLsaCredential(dwRid, info);
}
