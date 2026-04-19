// CredentialOps.h - LSA credential operations
#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include "../EIDMigrate/LsaClient.h"

// Remove EID credential (LSA secret) for a user
HRESULT RemoveEIDCredential(_In_ DWORD dwRid);

// Remove EID credential by username
HRESULT RemoveEIDCredential(_In_ const std::wstring& wsUsername);

// Check if user has EID credential
HRESULT HasEIDCredential(_In_ DWORD dwRid, _Out_ BOOL& pfHasCredential);

// Check if user has EID credential by username
HRESULT HasEIDCredential(_In_ const std::wstring& wsUsername, _Out_ BOOL& pfHasCredential);

// Get credential info for a user
HRESULT GetCredentialInfo(_In_ DWORD dwRid, _Out_ CredentialInfo& info);
