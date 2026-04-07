// CertificateOps.h - Certificate removal operations
#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <wincrypt.h>

// Remove all certificates for a user from their certificate store
HRESULT RemoveUserCertificates(_In_ const std::wstring& wsUsername);

// Enumerate certificates in user's MY store
HRESULT EnumerateUserCertificates(_In_ const std::wstring& wsUsername,
    _Out_ std::vector<PCCERT_CONTEXT>& certs);

// Remove certificates matching specific criteria
HRESULT RemoveCertificatesByHash(_In_ const std::wstring& wsUsername,
    _In_reads_(cbHash) const BYTE* pbHash, DWORD cbHash);

// Free certificate context array
void FreeCertificateArray(_Inout_ std::vector<PCCERT_CONTEXT>& certs);

// Helper: Get user certificate store path
HRESULT GetUserCertStorePath(_In_ const std::wstring& wsUsername, _Out_ std::wstring& wsStorePath);
