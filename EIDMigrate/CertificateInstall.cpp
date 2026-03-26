// File: EIDMigrate/CertificateInstall.cpp
// Certificate installation for import functionality

#include "CertificateInstall.h"
#include "Tracing.h"
#include <lm.h>
#include <sddl.h>

#pragma comment(lib, "crypt32.lib")

// Helper: Get user SID from username
static HRESULT GetUserSid(_In_ const std::wstring& wsUsername, _Out_ std::wstring& wsSid)
{
    if (wsUsername.empty())
    {
        // Use current user
        WCHAR szUserName[UNLEN + 1];
        DWORD dwSize = ARRAYSIZE(szUserName);
        if (!GetUserNameW(szUserName, &dwSize))
        {
            EIDM_TRACE_ERROR(L"GetUserNameW failed: %u", GetLastError());
            return HRESULT_FROM_WIN32(GetLastError());
        }

        DWORD dwSidSize = 0;
        DWORD dwDomainSize = 0;
        SID_NAME_USE use;

        // First call to get sizes
        LookupAccountNameW(NULL, szUserName, NULL, &dwSidSize,
            NULL, &dwDomainSize, &use);

        if (dwSidSize == 0)
        {
            EIDM_TRACE_ERROR(L"LookupAccountNameW failed to get SID size");
            return HRESULT_FROM_WIN32(GetLastError());
        }

        std::vector<BYTE> sidBuffer(dwSidSize);
        std::vector<WCHAR> domainBuffer(dwDomainSize);

        if (!LookupAccountNameW(NULL, szUserName, reinterpret_cast<PSID>(sidBuffer.data()),
            &dwSidSize, domainBuffer.data(), &dwDomainSize, &use))
        {
            EIDM_TRACE_ERROR(L"LookupAccountNameW failed for current user");
            return HRESULT_FROM_WIN32(GetLastError());
        }

        LPWSTR pwszSid = NULL;
        if (!ConvertSidToStringSidW(reinterpret_cast<PSID>(sidBuffer.data()), &pwszSid))
        {
            EIDM_TRACE_ERROR(L"ConvertSidToStringSidW failed");
            return HRESULT_FROM_WIN32(GetLastError());
        }

        wsSid = pwszSid;
        LocalFree(pwszSid);
        return S_OK;
    }

    // Get SID for specified username
    DWORD dwSidSize = 0;
    DWORD dwDomainSize = 0;
    SID_NAME_USE use;

    LookupAccountNameW(NULL, wsUsername.c_str(), NULL, &dwSidSize,
        NULL, &dwDomainSize, &use);

    if (dwSidSize == 0)
    {
        EIDM_TRACE_ERROR(L"LookupAccountNameW failed for user '%ls'", wsUsername.c_str());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    std::vector<BYTE> sidBuffer(dwSidSize);
    std::vector<WCHAR> domainBuffer(dwDomainSize);

    if (!LookupAccountNameW(NULL, wsUsername.c_str(), reinterpret_cast<PSID>(sidBuffer.data()),
        &dwSidSize, domainBuffer.data(), &dwDomainSize, &use))
    {
        EIDM_TRACE_ERROR(L"LookupAccountNameW failed for '%ls'", wsUsername.c_str());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    LPWSTR pwszSid = NULL;
    if (!ConvertSidToStringSidW(reinterpret_cast<PSID>(sidBuffer.data()), &pwszSid))
    {
        EIDM_TRACE_ERROR(L"ConvertSidToStringSidW failed");
        return HRESULT_FROM_WIN32(GetLastError());
    }

    wsSid = pwszSid;
    LocalFree(pwszSid);
    return S_OK;
}

// Install certificate to user's MY store
HRESULT InstallCertificateToUserStore(
    _In_reads_bytes_(cbCertificate) const BYTE* pbCertificate,
    _In_ DWORD cbCertificate,
    _In_opt_ const std::wstring& wsUsername)
{
    if (!pbCertificate || cbCertificate == 0)
    {
        EIDM_TRACE_ERROR(L"Invalid certificate data");
        return E_INVALIDARG;
    }

    EIDM_TRACE_INFO(L"Installing certificate for user '%ls'", wsUsername.empty() ? L"[current]" : wsUsername.c_str());

    // Create certificate context from DER-encoded bytes
    PCCERT_CONTEXT pCertContext = CertCreateCertificateContext(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        pbCertificate,
        cbCertificate);

    if (!pCertContext)
    {
        DWORD dwError = GetLastError();
        EIDM_TRACE_ERROR(L"CertCreateCertificateContext failed: %u", dwError);
        return HRESULT_FROM_WIN32(dwError);
    }

    HRESULT hr = S_OK;
    HCERTSTORE hCertStore = NULL;
    PCCERT_CONTEXT pExistingCert = NULL;  // Declare before goto target

    // If username is specified, we need to open their store
    if (!wsUsername.empty())
    {
        // For local user accounts, we can open the store directly with their SID
        std::wstring wsSid;
        hr = GetUserSid(wsUsername, wsSid);
        if (SUCCEEDED(hr))
        {
            // Build store path: \\.\<SID>\My
            std::wstring wsStorePath = L"\\\\.\\" + wsSid + L"\\My";

            hCertStore = CertOpenStore(
                CERT_STORE_PROV_SYSTEM,
                0,
                NULL,  // Was: nullptr - changed to NULL for HCRYPTPROV_LEGACY
                CERT_SYSTEM_STORE_CURRENT_USER,
                wsStorePath.c_str());

            if (hCertStore)
            {
                EIDM_TRACE_VERBOSE(L"Opened certificate store for user '%ls'", wsUsername.c_str());
            }
            else
            {
                // Fallback: try opening as current user (running as admin)
                EIDM_TRACE_WARN(L"Could not open user store, trying current user store");
                hCertStore = CertOpenStore(
                    CERT_STORE_PROV_SYSTEM,
                    0,
                    NULL,  // Was: nullptr - changed to NULL for HCRYPTPROV_LEGACY
                    CERT_SYSTEM_STORE_CURRENT_USER,
                    L"My");
            }
        }
    }
    else
    {
        // Open current user's MY store
        hCertStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM,
            0,
            NULL,  // Was: nullptr - changed to NULL for HCRYPTPROV_LEGACY
            CERT_SYSTEM_STORE_CURRENT_USER,
            L"My");
    }

    if (!hCertStore)
    {
        DWORD dwError = GetLastError();
        EIDM_TRACE_ERROR(L"CertOpenStore failed: %u", dwError);
        hr = HRESULT_FROM_WIN32(dwError);
        goto cleanup;
    }

    // Check if certificate already exists
    pExistingCert = CertFindCertificateInStore(
        hCertStore,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        0,
        CERT_FIND_EXISTING,
        pCertContext,
        NULL);

    if (pExistingCert)
    {
        EIDM_TRACE_INFO(L"Certificate already exists in store, updating");
        CertFreeCertificateContext(pExistingCert);
        pExistingCert = NULL;
    }

    // Add certificate to store
    if (!CertAddCertificateContextToStore(
        hCertStore,
        pCertContext,
        CERT_STORE_ADD_ALWAYS,  // Overwrite if exists
        NULL))
    {
        DWORD dwError = GetLastError();
        EIDM_TRACE_ERROR(L"CertAddCertificateContextToStore failed: %u", dwError);
        hr = HRESULT_FROM_WIN32(dwError);
        goto cleanup;
    }

    EIDM_TRACE_INFO(L"Certificate successfully installed to user's MY store");

cleanup:
    if (hCertStore)
        CertCloseStore(hCertStore, 0);
    if (pCertContext)
        CertFreeCertificateContext(pCertContext);
    if (pExistingCert)
        CertFreeCertificateContext(pExistingCert);

    return hr;
}

// Install certificate from DER-encoded bytes (wrapper)
HRESULT InstallCertificateFromDER(
    _In_ const std::vector<BYTE>& certificate,
    _In_opt_ const std::wstring& wsUsername)
{
    if (certificate.empty())
    {
        EIDM_TRACE_ERROR(L"Empty certificate data");
        return E_INVALIDARG;
    }

    return InstallCertificateToUserStore(
        certificate.data(),
        static_cast<DWORD>(certificate.size()),
        wsUsername);
}

// Check if certificate is already installed
BOOL IsCertificateInstalled(
    _In_reads_bytes_(cbCertificate) const BYTE* pbCertificate,
    _In_ DWORD cbCertificate)
{
    if (!pbCertificate || cbCertificate == 0)
        return FALSE;

    PCCERT_CONTEXT pCertContext = CertCreateCertificateContext(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        pbCertificate,
        cbCertificate);

    if (!pCertContext)
        return FALSE;

    HCERTSTORE hStore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM,
        0,
        NULL,  // Was: nullptr - changed to NULL for HCRYPTPROV_LEGACY
        CERT_SYSTEM_STORE_CURRENT_USER,
        L"My");

    if (!hStore)
    {
        CertFreeCertificateContext(pCertContext);
        return FALSE;
    }

    PCCERT_CONTEXT pExisting = CertFindCertificateInStore(
        hStore,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        0,
        CERT_FIND_EXISTING,
        pCertContext,
        NULL);

    BOOL fFound = (pExisting != NULL);

    if (pExisting)
        CertFreeCertificateContext(pExisting);
    CertCloseStore(hStore, 0);
    CertFreeCertificateContext(pCertContext);

    return fFound;
}

// Remove certificate from store
HRESULT RemoveCertificateFromStore(
    _In_reads_bytes_(cbCertificate) const BYTE* pbCertificate,
    _In_ DWORD cbCertificate)
{
    if (!pbCertificate || cbCertificate == 0)
        return E_INVALIDARG;

    PCCERT_CONTEXT pCertContext = CertCreateCertificateContext(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        pbCertificate,
        cbCertificate);

    if (!pCertContext)
        return HRESULT_FROM_WIN32(GetLastError());

    HCERTSTORE hStore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM,
        0,
        NULL,  // Was: nullptr - changed to NULL for HCRYPTPROV_LEGACY
        CERT_SYSTEM_STORE_CURRENT_USER,
        L"My");

    if (!hStore)
    {
        CertFreeCertificateContext(pCertContext);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    PCCERT_CONTEXT pExisting = CertFindCertificateInStore(
        hStore,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        0,
        CERT_FIND_EXISTING,
        pCertContext,
        NULL);

    HRESULT hr = S_OK;
    if (pExisting)
    {
        if (!CertDeleteCertificateFromStore(pExisting))
            hr = HRESULT_FROM_WIN32(GetLastError());
        CertFreeCertificateContext(pExisting);
        EIDM_TRACE_INFO(L"Certificate removed from store");
    }
    else
    {
        hr = S_FALSE;  // Not found
    }

    CertCloseStore(hStore, 0);
    CertFreeCertificateContext(pCertContext);

    return hr;
}
