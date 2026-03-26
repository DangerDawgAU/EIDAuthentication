#pragma once

// File: EIDMigrate/List.h
// List credentials functionality

#include "EIDMigrate.h"
#include <vector>

// Forward declarations
struct CredentialInfo;

// List options
struct LIST_OPTIONS
{
    BOOL fLocal;           // List local LSA credentials
    BOOL fInputFile;       // List credentials from import file
    std::wstring wsInputPath;
    BOOL fVerbose;

    LIST_OPTIONS() :
        fLocal(FALSE),
        fInputFile(FALSE),
        fVerbose(FALSE)
    {}
};

// List credentials (either local or from file)
HRESULT ListCredentials(
    _In_ const LIST_OPTIONS& options);

// List local LSA credentials
HRESULT ListLocalCredentials(
    _In_ BOOL fVerbose);

// List credentials from import file
HRESULT ListImportFileCredentials(
    _In_ const std::wstring& wsInputPath,
    _In_ BOOL fVerbose);

// Display credential summary
void DisplayCredentialSummary(
    _In_ const CredentialInfo& info,
    _In_ BOOL fVerbose);

// Display credentials in JSON format
HRESULT DisplayCredentialsAsJson(
    _In_ const std::vector<CredentialInfo>& credentials);
