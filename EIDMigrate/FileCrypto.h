#pragma once

// File: EIDMigrate/FileCrypto.h
// File encryption/decryption and JSON serialization

#include "EIDMigrate.h"
#include "LsaClient.h"
#include "SecureMemory.h"
#include <string>
#include <vector>

// JSON export/import structure
struct ExportFileData
{
    DWORD dwVersion;
    std::string formatVersion;
    std::string exportDate;
    std::wstring wsSourceMachine;
    std::wstring wsExportedBy;
    std::vector<CredentialInfo> credentials;
    std::vector<GroupInfo> groups;

    struct Statistics
    {
        DWORD totalCredentials;
        DWORD certificateEncrypted;
        DWORD dpapiEncrypted;
        DWORD skipped;
    } stats;

    ExportFileData() :
        dwVersion(1),
        formatVersion("EIDMigrate-v1.0"),
        stats{0}
    {}
};

// Write encrypted export file
HRESULT WriteEncryptedFile(
    _In_ const std::wstring& wsOutputPath,
    _In_ const SecureWString& wsPassword,
    _In_ const ExportFileData& data);

// Read and decrypt export file
HRESULT ReadEncryptedFile(
    _In_ const std::wstring& wsInputPath,
    _In_ const SecureWString& wsPassword,
    _Out_ ExportFileData& data);

// Validate file header (without decryption)
HRESULT ValidateFileHeader(
    _In_ const std::wstring& wsInputPath,
    _Out_ BOOL& pfValid,
    _Out_ std::wstring& wsError);

// Convert credential info to JSON
std::string CredentialToJson(_In_ const CredentialInfo& info);

// Convert group info to JSON
std::string GroupToJson(_In_ const GroupInfo& group);

// Convert export data to complete JSON
std::string ExportDataToJson(_In_ const ExportFileData& data);

// Parse credential from JSON
HRESULT JsonToCredential(
    _In_ const std::string& json,
    _Out_ CredentialInfo& info);

// Parse group from JSON
HRESULT JsonToGroup(
    _In_ const std::string& json,
    _Out_ GroupInfo& group);

// Parse export file JSON
HRESULT JsonToExportData(
    _In_ const std::string& json,
    _Out_ ExportFileData& data);

// Validate JSON schema
HRESULT ValidateJsonSchema(_In_ const std::string& json);
