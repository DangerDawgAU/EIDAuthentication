#pragma once

// File: EIDMigrate/Export.h
// Credential export functionality

#include "EIDMigrate.h"
#include "SecureMemory.h"
#include "GroupManagement.h"
#include <vector>

// Forward declarations
struct CredentialInfo;
struct GroupInfo;

// Export statistics
struct EXPORT_STATS
{
    DWORD dwTotalCredentials;
    DWORD dwCertificateEncrypted;
    DWORD dwDpapiEncrypted;
    DWORD dwSkipped;
    DWORD dwGroupsExported;

    EXPORT_STATS() :
        dwTotalCredentials(0),  // NOSONAR - INIT-01: member initialized in ctor init list for clarity
        dwCertificateEncrypted(0),  // NOSONAR - INIT-01: member initialized in ctor init list for clarity
        dwDpapiEncrypted(0),  // NOSONAR - INIT-01: member initialized in ctor init list for clarity
        dwSkipped(0),  // NOSONAR - INIT-01: member initialized in ctor init list for clarity
        dwGroupsExported(0)  // NOSONAR - INIT-01: member initialized in ctor init list for clarity
    {}
};

// Export options
struct EXPORT_OPTIONS
{
    BOOL fValidateCerts;
    BOOL fIncludeGroups;
    std::vector<std::wstring> SelectedGroups;  // Specific groups to export (empty = all)
    std::wstring wsSourceMachine;

    EXPORT_OPTIONS() :
        fValidateCerts(FALSE),  // NOSONAR - INIT-01: member initialized in ctor init list for clarity
        fIncludeGroups(TRUE)  // NOSONAR - INIT-01: member initialized in ctor init list for clarity
    {}
};

// Export credentials from LSA to encrypted file
HRESULT ExportCredentials(
    _In_ const std::wstring& wsOutputPath,
    _In_ const SecureWString& wsPassword,
    _In_ const EXPORT_OPTIONS& options,
    _Out_ EXPORT_STATS& stats);

// Enumerate all local users with EID credentials
HRESULT EnumerateEidCredentials(
    _Out_ std::vector<CredentialInfo>& credentials);

// Export a single credential from LSA
HRESULT ExportSingleCredential(
    _In_ DWORD dwRid,
    _Out_ CredentialInfo& info);

// Validate certificate for export
HRESULT ValidateCertificateForExport(
    _In_ PCCERT_CONTEXT pCertContext,
    _Out_ BOOL& pfTrusted,
    _Out_ std::wstring& wsWarning);

// Get group memberships for a user
HRESULT GetUserGroupMemberships(
    _In_ DWORD dwRid,
    _Out_ std::vector<std::wstring>& groupNames);

// Get current date/time in ISO 8601 format for export
std::string GetExportDate();
