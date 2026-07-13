// File: EIDMigrate/CertificateInstall.h
// Certificate installation for import functionality

#pragma once

#include <windows.h>  // NOSONAR - INCLUDE-01: include order/casing significant for Windows SDK
#include <wincrypt.h>
#include <vector>
#include <string>

// Install certificate to user's MY store
// This is required for EID authentication to work after import
HRESULT InstallCertificateToUserStore(
    _In_reads_bytes_(cbCertificate) const BYTE* pbCertificate,
    _In_ DWORD cbCertificate,
    _In_opt_ const std::wstring& wsUsername = L"");

// Install certificate from DER-encoded bytes
HRESULT InstallCertificateFromDER(
    _In_ const std::vector<BYTE>& certificate,
    _In_opt_ const std::wstring& wsUsername = L"");

// Check if certificate is already installed in user's store
BOOL IsCertificateInstalled(
    _In_reads_bytes_(cbCertificate) const BYTE* pbCertificate,
    _In_ DWORD cbCertificate);

// Remove certificate from user's store (for cleanup/testing)
HRESULT RemoveCertificateFromStore(
    _In_reads_bytes_(cbCertificate) const BYTE* pbCertificate,
    _In_ DWORD cbCertificate);

// Install a CRL (Certificate Revocation List) into the LocalMachine CA store for offline
// revocation checking (M1). Accepts DER or PEM/base64. The CRL is installed ONLY if its
// signature verifies against a CA already trusted on this machine (Root/CA store), so a forged
// CRL cannot be planted. Requires administrator (writes the machine store).
HRESULT InstallCrlFromFile(_In_ const std::wstring& wsCrlPath);
