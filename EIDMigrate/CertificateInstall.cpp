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

        // BUG FIX #16: TOCTOU race condition mitigation - use retry loop for SID allocation
        constexpr DWORD MAX_RETRIES = 3;
        DWORD dwRetryCount = 0;
        DWORD dwSidSize = 0;
        DWORD dwDomainSize = 0;
        SID_NAME_USE use;
        std::vector<BYTE> sidBuffer;
        std::vector<WCHAR> domainBuffer;

        // First call to get sizes
        LookupAccountNameW(NULL, szUserName, NULL, &dwSidSize, // NOSONAR - Windows API requires NULL for LPCTSTR parameters
            NULL, &dwDomainSize, &use); // NOSONAR - Windows API requires NULL

        if (dwSidSize == 0)
        {
            EIDM_TRACE_ERROR(L"LookupAccountNameW failed to get SID size");
            return HRESULT_FROM_WIN32(GetLastError());
        }

        // Retry loop for SID allocation (handles TOCTOU race condition)
        BOOL fSuccess = FALSE;
        for (dwRetryCount = 0; dwRetryCount < MAX_RETRIES; dwRetryCount++)
        {
            // Allocate buffers with current size requirements
            sidBuffer.resize(dwSidSize);
            domainBuffer.resize(dwDomainSize);

            // Zero out buffers to avoid uninitialized memory issues
            SecureZeroMemory(sidBuffer.data(), dwSidSize);
            SecureZeroMemory(domainBuffer.data(), dwDomainSize * sizeof(WCHAR));

            // Second call to get actual SID
            fSuccess = LookupAccountNameW(NULL, szUserName, reinterpret_cast<PSID>(sidBuffer.data()), // NOSONAR - Windows API requires NULL; reinterpret_cast from BYTE* to PSID required for SID structure
                &dwSidSize, domainBuffer.data(), &dwDomainSize, &use);

            if (fSuccess)
            {
                break;  // Success - exit retry loop
            }

            DWORD dwError = GetLastError();
            if (dwError == ERROR_INSUFFICIENT_BUFFER)
            {
                // Buffer size changed between check and use (TOCTOU race condition)
                EIDM_TRACE_WARN(L"TOCTOU race condition on retry %u for current user: buffer size changed (retrying...)",
                    dwRetryCount + 1);
                // Loop will continue with new dwSidSize and dwDomainSize values
            }
            else
            {
                // Different error - not a TOCTOU issue, don't retry
                EIDM_TRACE_ERROR(L"LookupAccountNameW failed for current user: error %u", dwError);
                return HRESULT_FROM_WIN32(dwError);
            }
        }

        if (!fSuccess)
        {
            EIDM_TRACE_ERROR(L"Exhausted %u retries for SID lookup of current user (possible persistent race condition)",
                MAX_RETRIES);
            return HRESULT_FROM_WIN32(GetLastError());
        }

        LPWSTR pwszSid = NULL; // NOSONAR - NULL required for Windows API compatibility
        if (!ConvertSidToStringSidW(reinterpret_cast<PSID>(sidBuffer.data()), &pwszSid)) // NOSONAR - reinterpret_cast from BYTE* to PSID required for Windows SID API // NOSONAR - Windows API requires PSID cast from BYTE buffer
        {
            EIDM_TRACE_ERROR(L"ConvertSidToStringSidW failed");
            return HRESULT_FROM_WIN32(GetLastError());
        }

        wsSid = pwszSid;
        LocalFree(pwszSid);
        return S_OK;
    }

    // BUG FIX #16: TOCTOU race condition mitigation - use retry loop for SID allocation
    constexpr DWORD MAX_RETRIES = 3;
    DWORD dwRetryCount = 0;
    DWORD dwSidSize = 0;
    DWORD dwDomainSize = 0;
    SID_NAME_USE use;
    std::vector<BYTE> sidBuffer;
    std::vector<WCHAR> domainBuffer;

    // First call to get sizes
    LookupAccountNameW(NULL, wsUsername.c_str(), NULL, &dwSidSize, // NOSONAR - Windows API requires NULL
        NULL, &dwDomainSize, &use); // NOSONAR - Windows API requires NULL

    if (dwSidSize == 0)
    {
        EIDM_TRACE_ERROR(L"LookupAccountNameW failed for user '%ls'", wsUsername.c_str());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    // Retry loop for SID allocation (handles TOCTOU race condition)
    BOOL fSuccess = FALSE;
    for (dwRetryCount = 0; dwRetryCount < MAX_RETRIES; dwRetryCount++)
    {
        // Allocate buffers with current size requirements
        sidBuffer.resize(dwSidSize);
        domainBuffer.resize(dwDomainSize);

        // Zero out buffers to avoid uninitialized memory issues
        SecureZeroMemory(sidBuffer.data(), dwSidSize);
        SecureZeroMemory(domainBuffer.data(), dwDomainSize * sizeof(WCHAR));

        // Second call to get actual SID
        fSuccess = LookupAccountNameW(NULL, wsUsername.c_str(), reinterpret_cast<PSID>(sidBuffer.data()), // NOSONAR - Windows API requires NULL; reinterpret_cast from BYTE* to PSID required for SID structure
            &dwSidSize, domainBuffer.data(), &dwDomainSize, &use);

        if (fSuccess)
        {
            break;  // Success - exit retry loop
        }

        DWORD dwError = GetLastError();
        if (dwError == ERROR_INSUFFICIENT_BUFFER)
        {
            // Buffer size changed between check and use (TOCTOU race condition)
            EIDM_TRACE_WARN(L"TOCTOU race condition on retry %u for '%ls': buffer size changed (retrying...)",
                dwRetryCount + 1, wsUsername.c_str());
            // Loop will continue with new dwSidSize and dwDomainSize values
        }
        else
        {
            // Different error - not a TOCTOU issue, don't retry
            EIDM_TRACE_ERROR(L"LookupAccountNameW failed for '%ls': error %u", wsUsername.c_str(), dwError);
            return HRESULT_FROM_WIN32(dwError);
        }
    }

    if (!fSuccess)
    {
        EIDM_TRACE_ERROR(L"Exhausted %u retries for SID lookup of '%ls' (possible persistent race condition)",
            MAX_RETRIES, wsUsername.c_str());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    LPWSTR pwszSid = NULL; // NOSONAR - Windows API requires NULL
    if (!ConvertSidToStringSidW(reinterpret_cast<PSID>(sidBuffer.data()), &pwszSid)) // NOSONAR - reinterpret_cast from BYTE* to PSID required for Windows SID API
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
    HCERTSTORE hCertStore = NULL; // NOSONAR - NULL required for Windows API compatibility
    PCCERT_CONTEXT pExistingCert = NULL; // NOSONAR - NULL required for Windows API compatibility; Declare before goto target

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
                NULL,  // NOSONAR - Was: nullptr - changed to NULL for HCRYPTPROV_LEGACY; Windows API requires NULL
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
                    NULL,  // NOSONAR - Was: nullptr - changed to NULL for HCRYPTPROV_LEGACY; Windows API requires NULL
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
            NULL,  // NOSONAR - Was: nullptr - changed to NULL for HCRYPTPROV_LEGACY; Windows API requires NULL
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
        NULL); // NOSONAR - Windows API requires NULL

    if (pExistingCert)
    {
        EIDM_TRACE_INFO(L"Certificate already exists in store, updating");
        CertFreeCertificateContext(pExistingCert);
        pExistingCert = NULL; // NOSONAR - Windows API requires NULL
    }

    // Add certificate to store
    if (!CertAddCertificateContextToStore(
        hCertStore,
        pCertContext,
        CERT_STORE_ADD_ALWAYS,  // Overwrite if exists
        NULL)) // NOSONAR - Windows API requires NULL
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
        NULL); // NOSONAR - Windows API requires NULL

    BOOL fFound = (pExisting != NULL); // NOSONAR - Windows API requires NULL

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
        NULL); // NOSONAR - Windows API requires NULL

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
