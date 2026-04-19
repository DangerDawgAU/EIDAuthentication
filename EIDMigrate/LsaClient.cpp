// File: EIDMigrate/LsaClient.cpp
// LSA IPC client functions

#include "LsaClient.h"
#include "Tracing.h"
#include "../EIDCardLibrary/StoredCredentialManagement.h"  // For EID_PRIVATE_DATA
#include <lm.h>
#include <ntstatus.h>
#include <sddl.h>  // For ConvertSidToStringSidW
#include <vector>

#pragma comment(lib, "netapi32.lib")

// Forward declarations from EIDCardLibrary/Package.h
extern PTSTR GetUsernameFromRid(__in DWORD dwRid);
extern DWORD GetRidFromUsername(LPTSTR szUsername);

// Forward declarations for IPC functions from EIDCardLibrary/Package.cpp
HRESULT LsaEIDEnumerateCredentials(_Out_ std::vector<EIDM_CREDENTIAL_SUMMARY>& summaries);
HRESULT LsaEIDExportCredential(_In_ DWORD dwRid, _Out_ PEIDM_EXPORT_RESPONSE* ppResponse);
HRESULT LsaEIDImportCredential(
    _In_ const EIDM_IMPORT_REQUEST* pRequest,
    _In_reads_bytes_(cbPrivateData) const BYTE* pbPrivateData,
    _In_ DWORD cbPrivateData,
    _In_reads_bytes_(cbCertificate) const BYTE* pbCertificate,
    _In_ DWORD cbCertificate,
    _In_reads_bytes_(cbPassword) const BYTE* pbPassword,
    _In_ DWORD cbPassword,
    _Out_ PEIDM_IMPORT_RESPONSE* ppResponse);

// NT_SUCCESS macro
#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

// Enumerate all EID credentials from LSA
// Uses direct LSA access instead of the authentication package IPC
HRESULT EnumerateLsaCredentials(_Out_ std::vector<CredentialInfo>& credentials)
{
    // BUG FIX #19: C++ exception handling for LSA operations
    // Ensures proper cleanup even if C++ exceptions occur
    HRESULT hr = S_OK;
    HANDLE hLsa = nullptr;
    LPUSER_INFO_0 pUserInfoArray = nullptr;

    try
    {
        EIDM_TRACE_VERBOSE(L"Enumerating LSA credentials via direct LSA access...");

        NTSTATUS status;
        LSA_OBJECT_ATTRIBUTES objectAttributes = {};
        objectAttributes.Length = sizeof(LSA_OBJECT_ATTRIBUTES);

        // Open LSA policy with access to read private data
        status = LsaOpenPolicy(
            nullptr,
            &objectAttributes,
            POLICY_GET_PRIVATE_INFORMATION | POLICY_VIEW_LOCAL_INFORMATION,
            &hLsa);

        if (!NT_SUCCESS(status))
        {
            EIDM_TRACE_ERROR(L"Failed to open LSA policy: 0x%08X", status);
            return HRESULT_FROM_NT(status);
        }

        // Enumerate local users
        DWORD dwEntriesRead = 0;
        DWORD dwTotalEntries = 0;

        DWORD dwNetStatus = NetUserEnum(nullptr, 0, FILTER_NORMAL_ACCOUNT,
            reinterpret_cast<LPBYTE*>(&pUserInfoArray), // NOSONAR - Windows Net API requires LPBYTE* for output parameter
            MAX_PREFERRED_LENGTH,
            &dwEntriesRead,
            &dwTotalEntries,
            nullptr);

        if (dwNetStatus != NERR_Success)
        {
            EIDM_TRACE_ERROR(L"Failed to enumerate users: %u", dwNetStatus);
            hr = HRESULT_FROM_WIN32(dwNetStatus);
            goto cleanup;
        }

        EIDM_TRACE_VERBOSE(L"Found %u local user(s) to check for credentials", dwEntriesRead);

    // Check each user for EID credentials
    for (DWORD i = 0; i < dwEntriesRead; i++)
    {
        if (!pUserInfoArray[i].usri0_name)
            continue;

        EIDM_TRACE_VERBOSE(L"Checking user '%ls'", pUserInfoArray[i].usri0_name);

        // Get SID for this user
        DWORD dwSidSize = 0;
        DWORD dwDomainSize = 0;
        SID_NAME_USE use = SidTypeUser;
        PSID pSid = nullptr;

        // First call to get SID size
        if (!LookupAccountNameW(nullptr, pUserInfoArray[i].usri0_name, nullptr, &dwSidSize,
            nullptr, &dwDomainSize, &use))
        {
            // Expected to fail, but should set buffer sizes
        }

        if (dwSidSize == 0)
        {
            EIDM_TRACE_WARN(L"Failed to get SID size for user '%ls'", pUserInfoArray[i].usri0_name);
            continue;
        }

        pSid = static_cast<PSID>(malloc(dwSidSize)); // NOSONAR - LookupAccountNameW requires malloc/free for SID buffer
        if (!pSid)
        {
            EIDM_TRACE_ERROR(L"Failed to allocate %u bytes for SID", dwSidSize);
            continue;
        }
        SecureZeroMemory(pSid, dwSidSize);

        // Allocate domain buffer
        PWSTR pwszDomain = static_cast<PWSTR>(malloc(dwDomainSize * sizeof(WCHAR))); // NOSONAR - LookupAccountNameW requires malloc/free for domain buffer
        if (!pwszDomain)
        {
            EIDM_TRACE_ERROR(L"Failed to allocate domain buffer");
            free(pSid);
            continue;
        }

        // Second call to get actual SID
        if (!LookupAccountNameW(nullptr, pUserInfoArray[i].usri0_name, pSid, &dwSidSize,
            pwszDomain, &dwDomainSize, &use))
        {
            EIDM_TRACE_WARN(L"LookupAccountNameW failed for '%ls'", pUserInfoArray[i].usri0_name);
            free(pSid);
            free(pwszDomain);
            continue;
        }
        free(pwszDomain);

        // Validate the SID structure
        if (!IsValidSid(pSid))
        {
            EIDM_TRACE_ERROR(L"Invalid SID returned for '%ls'", pUserInfoArray[i].usri0_name);
            free(pSid);
            continue;
        }

        // Extract RID from SID
        DWORD dwSubAuthCount = *GetSidSubAuthorityCount(pSid);
        if (dwSubAuthCount == 0)
        {
            EIDM_TRACE_ERROR(L"SID has no subauthorities for '%ls'", pUserInfoArray[i].usri0_name);
            free(pSid);
            continue;
        }

        DWORD dwRid = *GetSidSubAuthority(pSid, dwSubAuthCount - 1);
        free(pSid);

        EIDM_TRACE_VERBOSE(L"Checking user '%ls' (RID %u)", pUserInfoArray[i].usri0_name, dwRid);

        // Check if LSA secret exists for this RID
        // Format must match StoredCredentialManagement.cpp: L"%s_%08X" where CREDENTIAL_LSAPREFIX = L"L$_EID_"
        // Result: L$_EID__<RID> (note: double underscore since CREDENTIAL_LSAPREFIX already ends with _)
        WCHAR wszSecretName[256];
        swprintf_s(wszSecretName, ARRAYSIZE(wszSecretName), L"L$_EID__%08X", dwRid);

        LSA_UNICODE_STRING lsaSecretName;
        lsaSecretName.Buffer = wszSecretName;
        lsaSecretName.Length = static_cast<USHORT>(wcslen(wszSecretName) * sizeof(WCHAR)); // NOSONAR - wszSecretName is stack-allocated buffer, never NULL
        lsaSecretName.MaximumLength = lsaSecretName.Length + sizeof(WCHAR);

        PLSA_UNICODE_STRING pSecretData = nullptr;
        status = LsaRetrievePrivateData(hLsa, &lsaSecretName, &pSecretData);

        if (NT_SUCCESS(status) && pSecretData && pSecretData->Buffer)
        {
            EIDM_TRACE_INFO(L"Found credential for RID %u (%ls)", dwRid, pUserInfoArray[i].usri0_name);

            CredentialInfo info;
            info.dwRid = dwRid;
            info.wsUsername = pUserInfoArray[i].usri0_name;

            // Get SID separately (outside the secret parsing context)
            info.wsSid = LookupSidByUsername(info.wsUsername);
            if (info.wsSid.empty())
            {
                EIDM_TRACE_WARN(L"Could not lookup SID for '%ls'", info.wsUsername.c_str());
            }

            // Parse the secret data to get certificate hash and encryption type
            // The secret format is: EID_PRIVATE_DATA structure
            constexpr DWORD dwMinPrivateDataSize = offsetof(EID_PRIVATE_DATA, Hash) + CERT_HASH_LENGTH;

            if (pSecretData->Length >= dwMinPrivateDataSize)
            {
                PEID_PRIVATE_DATA pPrivateData = reinterpret_cast<PEID_PRIVATE_DATA>(pSecretData->Buffer); // NOSONAR - Cast from PBYTE* to PEID_PRIVATE_DATA required for LSA private data structure

                // Validate structure fields before accessing
                if (pPrivateData->dwType >= static_cast<EID_PRIVATE_DATA_TYPE>(1) &&
                    pPrivateData->dwType <= static_cast<EID_PRIVATE_DATA_TYPE>(3))
                {
                    info.EncryptionType = pPrivateData->dwType;
                }
                else
                {
                    EIDM_TRACE_WARN(L"Invalid encryption type %d for RID %u", pPrivateData->dwType, dwRid);
                    info.EncryptionType = static_cast<EID_PRIVATE_DATA_TYPE>(0);
                }

                // Copy certificate hash with bounds checking
                DWORD dwHashCopySize = min(CERT_HASH_LENGTH, pSecretData->Length - offsetof(EID_PRIVATE_DATA, Hash));
                memcpy(info.CertificateHash, pPrivateData->Hash, dwHashCopySize);
                // Zero out any remaining hash bytes
                if (dwHashCopySize < CERT_HASH_LENGTH)
                {
                    SecureZeroMemory(info.CertificateHash + dwHashCopySize, CERT_HASH_LENGTH - dwHashCopySize);
                }

                // BUG FIX #13: Extract certificate, key, and password data from LSA secret
                // Previously, enumeration only extracted hash and type, causing exports
                // to have empty certificate fields. This prevented successful import.
                //
                // IMPORTANT: The offsets in EID_PRIVATE_DATA are relative to the Data field,
                // NOT the beginning of the structure!
                // - dwCertificatOffset = 0 means certificate starts at pPrivateData->Data
                // - dwSymetricKeyOffset is relative to Data field
                // - dwPasswordOffset is relative to Data field
                //
                // Extract certificate from the Data field at dwCertificatOffset
                if (pPrivateData->dwCertificatSize > 0)
                {
                    // Certificate offset is relative to Data field, which is at pPrivateData->Data
                    BYTE* pCertificate = pPrivateData->Data + pPrivateData->dwCertificatOffset;

                    // Validate the certificate data is within the secret buffer
                    DWORD dwCertOffset = static_cast<DWORD>(pCertificate - reinterpret_cast<BYTE*>(pPrivateData)); // NOSONAR - Cast PEID_PRIVATE_DATA* to BYTE* required for pointer arithmetic within LSA secret buffer
                    DWORD dwCertEnd = dwCertOffset + pPrivateData->dwCertificatSize;

                    if (dwCertEnd <= pSecretData->Length)
                    {
                        info.Certificate.assign(pCertificate, pCertificate + pPrivateData->dwCertificatSize);
                        EIDM_TRACE_VERBOSE(L"Extracted certificate: %u bytes at offset %u",
                            pPrivateData->dwCertificatSize, pPrivateData->dwCertificatOffset);

                        // Verify it looks like a DER certificate (starts with 0x30)
                        if (pCertificate[0] == 0x30)
                        {
                            EIDM_TRACE_VERBOSE(L"Certificate data verified (DER format)");
                        }
                        else
                        {
                            EIDM_TRACE_WARN(L"Certificate data doesn't look like DER (starts with 0x%02X)", pCertificate[0]);
                        }
                    }
                    else
                    {
                        EIDM_TRACE_WARN(L"Certificate data extends beyond buffer (offset=%u, size=%u, total=%u)",
                            dwCertOffset, pPrivateData->dwCertificatSize, pSecretData->Length);
                    }
                }

                // Extract symmetric key
                // IMPORTANT: Offsets are relative to Data field, not struct start
                if (pPrivateData->dwSymetricKeySize > 0)
                {
                    DWORD dwKeyEnd = pPrivateData->dwSymetricKeyOffset + pPrivateData->dwSymetricKeySize;
                    // Check bounds relative to Data field size
                    DWORD dwDataSize = pSecretData->Length - sizeof(EID_PRIVATE_DATA);
                    if (dwKeyEnd <= dwDataSize)
                    {
                        BYTE* pKey = pPrivateData->Data + pPrivateData->dwSymetricKeyOffset;
                        info.SymmetricKey.assign(pKey, pKey + pPrivateData->dwSymetricKeySize);
                        EIDM_TRACE_VERBOSE(L"Extracted symmetric key: %u bytes", pPrivateData->dwSymetricKeySize);
                    }
                    else
                    {
                        EIDM_TRACE_WARN(L"Key offset out of bounds (offset=%u, size=%u, data_size=%u), skipping key data",
                            pPrivateData->dwSymetricKeyOffset, pPrivateData->dwSymetricKeySize, dwDataSize);
                    }
                }

                // Extract encrypted password
                // IMPORTANT: Offsets are relative to Data field, not struct start
                if (pPrivateData->usPasswordLen > 0)
                {
                    DWORD dwPwdEnd = pPrivateData->dwPasswordOffset + pPrivateData->usPasswordLen;
                    // Check bounds relative to Data field size
                    DWORD dwDataSize = pSecretData->Length - sizeof(EID_PRIVATE_DATA);
                    if (dwPwdEnd <= dwDataSize)
                    {
                        BYTE* pPassword = pPrivateData->Data + pPrivateData->dwPasswordOffset;
                        info.EncryptedPassword.assign(pPassword, pPassword + pPrivateData->usPasswordLen);
                        EIDM_TRACE_VERBOSE(L"Extracted encrypted password: %u bytes", pPrivateData->usPasswordLen);
                    }
                    else
                    {
                        EIDM_TRACE_WARN(L"Password offset out of bounds (offset=%u, size=%u, data_size=%u), skipping password data",
                            pPrivateData->dwPasswordOffset, pPrivateData->usPasswordLen, dwDataSize);
                    }
                }

                EIDM_TRACE_VERBOSE(L"Parsed credential: type=%d, hash_size=%u, cert_size=%zu",
                    info.EncryptionType, dwHashCopySize, info.Certificate.size());
            }
            else
            {
                EIDM_TRACE_ERROR(L"Secret data too small: %u < %u", pSecretData->Length, dwMinPrivateDataSize);
                LsaFreeMemory(pSecretData);
                continue;
            }

            credentials.push_back(info);
            LsaFreeMemory(pSecretData);
        }
        else
        {
            EIDM_TRACE_VERBOSE(L"No credential for RID %u", dwRid);
        }
    }

    hr = S_OK;
    }
    catch (...)
    {
        EIDM_TRACE_ERROR(L"Exception occurred during LSA enumeration");
        hr = E_FAIL;
    }

cleanup:
    // Cleanup: always executed regardless of exception
    if (pUserInfoArray)
    {
        NetApiBufferFree(pUserInfoArray);
    }
    if (hLsa)
    {
        LsaClose(hLsa);
    }

    EIDM_TRACE_INFO(L"Enumeration complete: %zu credential(s) found", credentials.size());
    return hr;
}

// Export a single credential from LSA by RID
HRESULT ExportLsaCredential(_In_ DWORD dwRid, _Out_ CredentialInfo& info)
{
    EIDM_TRACE_VERBOSE(L"Exporting credential for RID %u via direct LSA access...", dwRid);

    NTSTATUS status;
    HANDLE hLsa = nullptr;
    LSA_OBJECT_ATTRIBUTES objectAttributes = {};
    objectAttributes.Length = sizeof(LSA_OBJECT_ATTRIBUTES);

    // Open LSA policy
    status = LsaOpenPolicy(
        nullptr,
        &objectAttributes,
        POLICY_GET_PRIVATE_INFORMATION,
        &hLsa);

    if (!NT_SUCCESS(status))
    {
        EIDM_TRACE_ERROR(L"[ERROR] LsaOpenPolicy failed: 0x%08X", status);
        return HRESULT_FROM_NT(status);
    }

    // Retrieve the LSA secret for this RID
    // Format: L$_EID__<RID> (double underscore)
    WCHAR wszSecretName[256];
    swprintf_s(wszSecretName, ARRAYSIZE(wszSecretName), L"L$_EID__%08X", dwRid);

    LSA_UNICODE_STRING lsaSecretName;
    lsaSecretName.Buffer = wszSecretName;
    lsaSecretName.Length = static_cast<USHORT>(wcslen(wszSecretName) * sizeof(WCHAR)); // NOSONAR - wszSecretName is stack-allocated buffer, never NULL
    lsaSecretName.MaximumLength = lsaSecretName.Length + sizeof(WCHAR);

    PLSA_UNICODE_STRING pSecretData = nullptr;
    status = LsaRetrievePrivateData(hLsa, &lsaSecretName, &pSecretData);

    if (NT_SUCCESS(status) && pSecretData && pSecretData->Buffer)
    {
        PEID_PRIVATE_DATA pPrivateData = reinterpret_cast<PEID_PRIVATE_DATA>(pSecretData->Buffer);

        info.dwRid = dwRid;
        info.EncryptionType = pPrivateData->dwType;
        memcpy(info.CertificateHash, pPrivateData->Hash, CERT_HASH_LENGTH);

        // Get username from RID
        info.wsUsername = LookupUsernameByRid(dwRid);
        info.wsSid = LookupSidByUsername(info.wsUsername);

        // Extract certificate, key, and password from the Data field
        // IMPORTANT: Offsets are relative to Data field, and certificate offset can be 0!
        DWORD dwDataSize = pSecretData->Length - sizeof(EID_PRIVATE_DATA);

        // Extract certificate
        if (pPrivateData->dwCertificatSize > 0)
        {
            // Certificate offset is relative to Data field (can be 0!)
            DWORD dwCertEnd = pPrivateData->dwCertificatOffset + pPrivateData->dwCertificatSize;
            if (dwCertEnd <= dwDataSize)
            {
                BYTE* pCertificate = pPrivateData->Data + pPrivateData->dwCertificatOffset;
                info.Certificate.assign(pCertificate, pCertificate + pPrivateData->dwCertificatSize);
                EIDM_TRACE_VERBOSE(L"ExportLsaCredential: Extracted certificate: %u bytes", pPrivateData->dwCertificatSize);
            }
            else
            {
                EIDM_TRACE_WARN(L"ExportLsaCredential: Certificate data out of bounds");
            }
        }

        // Extract symmetric key
        if (pPrivateData->dwSymetricKeySize > 0)
        {
            DWORD dwKeyEnd = pPrivateData->dwSymetricKeyOffset + pPrivateData->dwSymetricKeySize;
            if (dwKeyEnd <= dwDataSize)
            {
                BYTE* pKey = pPrivateData->Data + pPrivateData->dwSymetricKeyOffset;
                info.SymmetricKey.assign(pKey, pKey + pPrivateData->dwSymetricKeySize);
                EIDM_TRACE_VERBOSE(L"ExportLsaCredential: Extracted symmetric key: %u bytes", pPrivateData->dwSymetricKeySize);
            }
            else
            {
                EIDM_TRACE_WARN(L"ExportLsaCredential: Symmetric key data out of bounds");
            }
        }

        // Extract encrypted password
        if (pPrivateData->usPasswordLen > 0)
        {
            DWORD dwPwdEnd = pPrivateData->dwPasswordOffset + pPrivateData->usPasswordLen;
            if (dwPwdEnd <= dwDataSize)
            {
                BYTE* pPassword = pPrivateData->Data + pPrivateData->dwPasswordOffset;
                info.EncryptedPassword.assign(pPassword, pPassword + pPrivateData->usPasswordLen);
                EIDM_TRACE_VERBOSE(L"ExportLsaCredential: Extracted encrypted password: %u bytes", pPrivateData->usPasswordLen);
            }
            else
            {
                EIDM_TRACE_WARN(L"ExportLsaCredential: Encrypted password data out of bounds");
            }
        }

        LsaFreeMemory(pSecretData);
        LsaClose(hLsa);
        return S_OK;
    }
    else
    {
        LsaClose(hLsa);
        return HRESULT_FROM_NT(status);
    }
}

// Check if user has stored EID credential
HRESULT HasStoredCredential(_In_ DWORD dwRid, _Out_ BOOL& pfHasCredential)
{
    pfHasCredential = FALSE;

    if (dwRid == 0)
    {
        return E_INVALIDARG;
    }

    HANDLE hLsa = nullptr;
    LSA_OBJECT_ATTRIBUTES objectAttributes = {};
    objectAttributes.Length = sizeof(LSA_OBJECT_ATTRIBUTES);

    NTSTATUS status = LsaOpenPolicy(
        nullptr,
        &objectAttributes,
        POLICY_GET_PRIVATE_INFORMATION,
        &hLsa);

    if (!NT_SUCCESS(status))
    {
        EIDM_TRACE_ERROR(L"HasStoredCredential: LsaOpenPolicy failed: 0x%08X", status);
        return HRESULT_FROM_NT(status);
    }

    // Secret name must match StoredCredentialManagement.cpp's format
    // ("%s_%08X" with CREDENTIAL_LSAPREFIX = L"L$_EID_") -> L$_EID__<RID>.
    WCHAR wszSecretName[256];
    swprintf_s(wszSecretName, ARRAYSIZE(wszSecretName), L"L$_EID__%08X", dwRid);

    LSA_UNICODE_STRING lsaSecretName;
    lsaSecretName.Buffer = wszSecretName;
    lsaSecretName.Length = static_cast<USHORT>(wcslen(wszSecretName) * sizeof(WCHAR));
    lsaSecretName.MaximumLength = lsaSecretName.Length + sizeof(WCHAR);

    PLSA_UNICODE_STRING pSecretData = nullptr;
    status = LsaRetrievePrivateData(hLsa, &lsaSecretName, &pSecretData);

    HRESULT hr = S_OK;
    if (NT_SUCCESS(status) && pSecretData && pSecretData->Buffer && pSecretData->Length > 0)
    {
        pfHasCredential = TRUE;
    }
    else if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        // Expected when user has no EID credential - not an error.
        pfHasCredential = FALSE;
    }
    else if (!NT_SUCCESS(status))
    {
        EIDM_TRACE_WARN(L"HasStoredCredential: LsaRetrievePrivateData returned 0x%08X for RID %u", status, dwRid);
        hr = HRESULT_FROM_NT(status);
    }

    if (pSecretData)
    {
        LsaFreeMemory(pSecretData);
    }
    LsaClose(hLsa);

    return hr;
}

std::wstring LookupUsernameByRid(_In_ DWORD dwRid)
{
    PTSTR pszUsername = ::GetUsernameFromRid(dwRid);
    if (pszUsername)
    {
        std::wstring wsResult(pszUsername);
        ::EIDFree(pszUsername);  // GetUsernameFromRid returns EIDAlloc'd memory
        return wsResult;
    }
    return std::wstring();
}

DWORD LookupRidByUsername(_In_ const std::wstring& wsUsername)
{
    return ::GetRidFromUsername(const_cast<PWSTR>(wsUsername.c_str()));
}

std::wstring LookupSidByUsername(_In_ const std::wstring& wsUsername)
{
    // Get required buffer size
    DWORD dwSidSize = 0;
    DWORD dwDomainSize = 0;
    SID_NAME_USE use;

    BOOL fResult = LookupAccountNameW(nullptr, wsUsername.c_str(), nullptr, &dwSidSize,
        nullptr, &dwDomainSize, &use);

    // After first call, check if we got the size
    if (dwSidSize == 0 || dwDomainSize == 0)
    {
        EIDM_TRACE_WARN(L"LookupAccountNameW failed to get buffer sizes for '%ls'", wsUsername.c_str());
        return std::wstring();
    }

    // Allocate SID buffer
    std::vector<BYTE> sidBytes(dwSidSize);
    std::vector<WCHAR> domainBuffer(dwDomainSize);

    // Zero out buffers to avoid uninitialized memory issues
    SecureZeroMemory(sidBytes.data(), dwSidSize);
    SecureZeroMemory(domainBuffer.data(), dwDomainSize * sizeof(WCHAR));

    fResult = LookupAccountNameW(nullptr, wsUsername.c_str(),
        reinterpret_cast<PSID>(sidBytes.data()), &dwSidSize,
        domainBuffer.data(), &dwDomainSize, &use);

    if (!fResult)
    {
        EIDM_TRACE_WARN(L"LookupAccountNameW failed for '%ls'", wsUsername.c_str());
        return std::wstring();
    }

    // Validate the SID before using it
    if (!IsValidSid(reinterpret_cast<PSID>(sidBytes.data())))
    {
        EIDM_TRACE_ERROR(L"Invalid SID returned for '%ls'", wsUsername.c_str());
        return std::wstring();
    }

    // Convert SID to string
    LPWSTR pwszSid = nullptr;
    if (!ConvertSidToStringSidW(reinterpret_cast<PSID>(sidBytes.data()), &pwszSid))
    {
        EIDM_TRACE_ERROR(L"ConvertSidToStringSidW failed for '%ls'", wsUsername.c_str());
        return std::wstring();
    }

    std::wstring wsResult(pwszSid);
    LocalFree(pwszSid);
    return wsResult;
}

HRESULT ImportLsaCredential(
    _In_ const CredentialInfo& info,
    _In_ DWORD dwFlags,
    _Out_ BOOL& pfUserCreated)
{
    EIDM_TRACE_VERBOSE(L"Importing credential for RID %u via direct LSA access...", info.dwRid);

    pfUserCreated = FALSE;

    NTSTATUS status;
    HANDLE hLsa = nullptr;
    LSA_OBJECT_ATTRIBUTES objectAttributes = {};
    objectAttributes.Length = sizeof(LSA_OBJECT_ATTRIBUTES);

    // Open LSA policy
    status = LsaOpenPolicy(
        nullptr,
        &objectAttributes,
        POLICY_CREATE_SECRET,
        &hLsa);

    if (!NT_SUCCESS(status))
    {
        EIDM_TRACE_ERROR(L"[ERROR] LsaOpenPolicy failed: 0x%08X", status);
        return HRESULT_FROM_NT(status);
    }

    // Build EID_PRIVATE_DATA structure
    // Calculate total size needed
    DWORD dwPrivateDataSize = sizeof(EID_PRIVATE_DATA) +
        static_cast<DWORD>(info.Certificate.size()) +
        static_cast<DWORD>(info.SymmetricKey.size()) +
        static_cast<DWORD>(info.EncryptedPassword.size());

    std::vector<BYTE> buffer(dwPrivateDataSize);
    PEID_PRIVATE_DATA pPrivateData = reinterpret_cast<PEID_PRIVATE_DATA>(buffer.data());

    // Fill in the structure
    // IMPORTANT: All offsets are relative to the Data field, starting at 0
    // This matches the format used by StoredCredentialManagement.cpp
    pPrivateData->dwType = info.EncryptionType;

    // Certificate always at offset 0
    pPrivateData->dwCertificatOffset = 0;
    pPrivateData->dwCertificatSize = static_cast<USHORT>(info.Certificate.size());

    // Symmetric key follows certificate
    pPrivateData->dwSymetricKeyOffset = pPrivateData->dwCertificatOffset + pPrivateData->dwCertificatSize;
    pPrivateData->dwSymetricKeySize = static_cast<USHORT>(info.SymmetricKey.size());

    // Encrypted password follows symmetric key
    pPrivateData->dwPasswordOffset = pPrivateData->dwSymetricKeyOffset + pPrivateData->dwSymetricKeySize;
    pPrivateData->usPasswordLen = static_cast<USHORT>(info.EncryptedPassword.size());

    memcpy(pPrivateData->Hash, info.CertificateHash, CERT_HASH_LENGTH);

    // Copy certificate, key, and password to Data field
    // Use the offsets we just calculated
    if (!info.Certificate.empty())
    {
        memcpy(pPrivateData->Data + pPrivateData->dwCertificatOffset,
               info.Certificate.data(), info.Certificate.size());
    }
    if (!info.SymmetricKey.empty())
    {
        memcpy(pPrivateData->Data + pPrivateData->dwSymetricKeyOffset,
               info.SymmetricKey.data(), info.SymmetricKey.size());
    }
    if (!info.EncryptedPassword.empty())
    {
        memcpy(pPrivateData->Data + pPrivateData->dwPasswordOffset,
               info.EncryptedPassword.data(), info.EncryptedPassword.size());
    }

    // Store the LSA secret
    // Format: L$_EID__<RID> (double underscore)
    WCHAR wszSecretName[256];
    swprintf_s(wszSecretName, ARRAYSIZE(wszSecretName), L"L$_EID__%08X", info.dwRid);

    LSA_UNICODE_STRING lsaSecretName;
    lsaSecretName.Buffer = wszSecretName;
    lsaSecretName.Length = static_cast<USHORT>(wcslen(wszSecretName) * sizeof(WCHAR)); // NOSONAR - wszSecretName is stack-allocated buffer, never NULL
    lsaSecretName.MaximumLength = lsaSecretName.Length + sizeof(WCHAR);

    LSA_UNICODE_STRING lsaSecretData;
    lsaSecretData.Buffer = reinterpret_cast<PWSTR>(buffer.data());
    lsaSecretData.Length = static_cast<USHORT>(dwPrivateDataSize);
    lsaSecretData.MaximumLength = lsaSecretData.Length;

    status = LsaStorePrivateData(hLsa, &lsaSecretName, &lsaSecretData);

    LsaClose(hLsa);

    if (NT_SUCCESS(status))
    {
        EIDM_TRACE_ERROR(L"[DEBUG] Credential stored successfully for RID %u", info.dwRid);
        return S_OK;
    }
    else
    {
        EIDM_TRACE_ERROR(L"[ERROR] LsaStorePrivateData failed: 0x%08X", status);
        return HRESULT_FROM_NT(status);
    }
}
