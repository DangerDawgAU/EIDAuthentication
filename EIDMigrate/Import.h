#pragma once

// File: EIDMigrate/Import.h
// Credential import functionality

#include "EIDMigrate.h"
#include "SecureMemory.h"
#include <vector>

// Forward declarations
struct CredentialInfo;
struct GroupInfo;

// Import statistics
struct IMPORT_STATS
{
    DWORD dwTotalCredentials;
    DWORD dwSuccessfullyImported;
    DWORD dwFailed;
    DWORD dwUsersCreated;
    DWORD dwGroupsCreated;
    DWORD dwWarnings;

    IMPORT_STATS() :
        dwTotalCredentials(0),
        dwSuccessfullyImported(0),
        dwFailed(0),
        dwUsersCreated(0),
        dwGroupsCreated(0),
        dwWarnings(0)
    {}
};

// Import options
struct IMPORT_OPTIONS
{
    BOOL fDryRun;
    BOOL fForce;
    BOOL fCreateUsers;
    BOOL fContinueOnError;
    std::vector<std::wstring> SelectedGroups;  // Specific groups to import (empty = all)

    // Optional: Map of username to password to set for user accounts
    // This allows setting a fallback password for smart card users
    std::vector<std::pair<std::wstring, std::wstring>> userPasswords;

    IMPORT_OPTIONS() :
        fDryRun(FALSE),
        fForce(FALSE),
        fCreateUsers(FALSE),
        fContinueOnError(FALSE)
    {}
};

// Import credentials from encrypted file to LSA
HRESULT ImportCredentials(
    _In_ const std::wstring& wsInputPath,
    _In_ const SecureWString& wsPassword,
    _In_ const IMPORT_OPTIONS& options,
    _Out_ IMPORT_STATS& stats);

// Read and parse import file
HRESULT ReadImportFile(
    _In_ const std::wstring& wsInputPath,
    _In_ const SecureWString& wsPassword,
    _Out_ std::vector<CredentialInfo>& credentials,
    _Out_ std::vector<GroupInfo>& groups);

// Read import file with metadata (for displaying export info in UI)
HRESULT ReadImportFileWithMetadata(
    _In_ const std::wstring& wsInputPath,
    _In_ const SecureWString& wsPassword,
    _Out_ std::vector<CredentialInfo>& credentials,
    _Out_ std::vector<GroupInfo>& groups,
    _Out_opt_ std::wstring* pwsSourceMachine,
    _Out_opt_ std::wstring* pwsExportDate,
    _Out_opt_ std::wstring* pwsExportedBy);

// Import a single credential
HRESULT ImportSingleCredential(
    _In_ const CredentialInfo& info,
    _In_ const IMPORT_OPTIONS& options,
    _Out_ BOOL& pfCreated);

// Validate certificate on import
HRESULT ValidateCertificateForImport(
    _In_ PCCERT_CONTEXT pCertContext,
    _Out_ BOOL& pfTrusted,
    _Out_ std::vector<std::wstring>& warnings);

// Prompt for smart card PIN
BOOL PromptForSmartCardPin(
    _In_ PCCERT_CONTEXT pCertContext,
    _Out_ SecureWString& wsPin);

// Validate smart card PIN
BOOL ValidateSmartCardPin(
    _In_ PCCERT_CONTEXT pCertContext,
    _In_ PCWSTR pwszPin);
