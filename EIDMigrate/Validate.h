#pragma once

// File: EIDMigrate/Validate.h
// Import file validation functionality

#include "EIDMigrate.h"
#include "LsaClient.h"
#include "SecureMemory.h"
#include <string>
#include <vector>

// Validate options
struct VALIDATE_OPTIONS
{
    std::wstring wsInputPath;
    BOOL fRequireSmartCard;
    BOOL fVerbose;

    VALIDATE_OPTIONS() :
        fRequireSmartCard(FALSE),  // NOSONAR - INIT-01: constructor initializer list retained for clarity
        fVerbose(FALSE)  // NOSONAR - INIT-01: constructor initializer list retained for clarity
    {}
};

// Validation result
struct VALIDATION_RESULT
{
    BOOL fValidFormat;
    BOOL fValidHeader;
    BOOL fValidHmac;
    BOOL fValidJson;
    BOOL fAllCertsTrusted;
    DWORD dwCredentialCount;
    DWORD dwWarningCount;
    DWORD dwErrorCount;
    std::vector<std::wstring> warnings;
    std::vector<std::wstring> errors;

    // Credential details from the file
    std::vector<CredentialInfo> credentials;

    // File metadata
    std::wstring wsSourceMachine;
    std::wstring wsExportDate;
    std::wstring wsExportedBy;

    VALIDATION_RESULT() :
        fValidFormat(FALSE),  // NOSONAR - INIT-01: constructor initializer list retained for clarity
        fValidHeader(FALSE),  // NOSONAR - INIT-01: constructor initializer list retained for clarity
        fValidHmac(FALSE),  // NOSONAR - INIT-01: constructor initializer list retained for clarity
        fValidJson(FALSE),  // NOSONAR - INIT-01: constructor initializer list retained for clarity
        fAllCertsTrusted(TRUE),  // NOSONAR - INIT-01: constructor initializer list retained for clarity
        dwCredentialCount(0),  // NOSONAR - INIT-01: constructor initializer list retained for clarity
        dwWarningCount(0),  // NOSONAR - INIT-01: constructor initializer list retained for clarity
        dwErrorCount(0)  // NOSONAR - INIT-01: constructor initializer list retained for clarity
    {}

    BOOL IsValid() const
    {
        return fValidFormat && fValidHeader && fValidHmac && fValidJson;
    }
};

// Validate import file
HRESULT ValidateImportFile(
    _In_ const std::wstring& wsInputPath,
    _In_ const SecureWString& wsPassword,
    _In_ const VALIDATE_OPTIONS& options,
    _Out_ VALIDATION_RESULT& result);

// Validate file format (without decrypting)
HRESULT ValidateFileFormat(
    _In_ const std::wstring& wsInputPath,
    _Out_ VALIDATION_RESULT& result);

// Validate encrypted content (decrypt and parse)
HRESULT ValidateEncryptedContent(
    _In_ const std::wstring& wsInputPath,
    _In_ const SecureWString& wsPassword,
    _In_ const VALIDATE_OPTIONS& options,
    _Out_ VALIDATION_RESULT& result);

// Validate certificates
HRESULT ValidateCertificates(_Out_ VALIDATION_RESULT& result);

// Display validation report
void DisplayValidationReport(_In_ const VALIDATION_RESULT& result);
