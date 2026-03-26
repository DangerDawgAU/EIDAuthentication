#pragma once

// File: EIDMigrate/LsaClient.h
// LSA IPC client functions for credential enumeration and export

#include "EIDMigrate.h"
#include "../EIDCardLibrary/EIDCardLibrary.h"
#include <vector>

// Credential summary information
struct CredentialInfo
{
    DWORD dwRid;
    std::wstring wsUsername;
    std::wstring wsSid;
    UCHAR CertificateHash[32];
    EID_PRIVATE_DATA_TYPE EncryptionType;
    std::vector<BYTE> Certificate;
    std::vector<BYTE> EncryptedPassword;
    std::vector<BYTE> SymmetricKey;  // Only for certificate encryption
    std::wstring wsAlgorithm;

    // Optional metadata
    std::wstring wsCertSubject;
    std::wstring wsCertIssuer;
    FILETIME ftCertValidFrom;
    FILETIME ftCertValidTo;
    DWORD dwPasswordLength;

    CredentialInfo() :
        dwRid(0),
        EncryptionType(EID_PRIVATE_DATA_TYPE::eidpdtClearText),
        wsAlgorithm(L"AES-256-CBC"),
        dwPasswordLength(0)
    {
        SecureZeroMemory(CertificateHash, sizeof(CertificateHash));
        ftCertValidFrom.dwHighDateTime = 0;
        ftCertValidFrom.dwLowDateTime = 0;
        ftCertValidTo.dwHighDateTime = 0;
        ftCertValidTo.dwLowDateTime = 0;
    }
};

// Group information
struct GroupInfo
{
    std::wstring wsName;
    std::wstring wsComment;
    std::vector<std::wstring> wsMembers;
    BOOL fBuiltin;

    GroupInfo() :
        fBuiltin(FALSE)
    {}
};

// Enumerate all EID credentials from LSA
HRESULT EnumerateLsaCredentials(
    _Out_ std::vector<CredentialInfo>& credentials);

// Export a single credential from LSA by RID
HRESULT ExportLsaCredential(
    _In_ DWORD dwRid,
    _Out_ CredentialInfo& info);

// Check if user has stored EID credential
HRESULT HasStoredCredential(
    _In_ DWORD dwRid,
    _Out_ BOOL& pfHasCredential);

// Get username from RID
std::wstring LookupUsernameByRid(_In_ DWORD dwRid);

// Get RID from username
DWORD LookupRidByUsername(_In_ const std::wstring& wsUsername);

// Get SID from username
std::wstring LookupSidByUsername(_In_ const std::wstring& wsUsername);

// Import credential to LSA
HRESULT ImportLsaCredential(
    _In_ const CredentialInfo& info,
    _In_ DWORD dwFlags,
    _Out_ BOOL& pfUserCreated);
