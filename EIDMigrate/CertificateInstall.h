// File: EIDMigrate/CertificateInstall.h
// Certificate installation for import functionality

#pragma once

#include <windows.h>
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
