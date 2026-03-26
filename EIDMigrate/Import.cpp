// File: EIDMigrate/Import.cpp
// Credential import functionality

#include "Import.h"
#include "LsaClient.h"
#include "UserManagement.h"
#include "GroupManagement.h"
#include "FileCrypto.h"
#include "AuditLogging.h"
#include "Tracing.h"
#include "Utils.h"
#include "PinPrompt.h"
#include "CertificateInstall.h"
#include <map>

HRESULT CommandImport(_In_ const COMMAND_OPTIONS& options)
{
    // Check dry-run vs force
    if (options.DryRun && options.Force)
    {
        EIDM_TRACE_ERROR(L"Error: Cannot specify both -dry-run and -force.");
        return E_INVALIDARG;
    }

    // Determine actual dry-run state
    BOOL fDryRun = options.DryRun;
    if (!options.Force && !options.DryRun)
    {
        EIDM_TRACE_WARN(L"Warning: Use -force to perform actual import.");
        EIDM_TRACE_WARN(L"Running in dry-run mode by default.");
        fDryRun = TRUE;
    }

    if (fDryRun)
    {
        EIDM_TRACE_INFO(L"DRY-RUN MODE: No changes will be made.");
    }

    // Get passphrase
    SecureWString wsPassword;
    if (options.Password.empty())
    {
        wsPassword = PromptForPassphrase(L"Enter import passphrase: ", FALSE);
        if (wsPassword.empty())
        {
            EIDM_TRACE_ERROR(L"Passphrase is required.");
            return E_FAIL;
        }
    }
    else
    {
        wsPassword = SecureWString(options.Password.c_str());
    }

    IMPORT_OPTIONS opts;
    opts.fDryRun = fDryRun;
    opts.fForce = options.Force;
    opts.fCreateUsers = options.CreateUsers;
    opts.fContinueOnError = options.ContinueOnError;

    IMPORT_STATS stats;
    HRESULT hr = ImportCredentials(options.InputFile, wsPassword, opts, stats);

    if (SUCCEEDED(hr))
    {
        EIDM_TRACE_INFO(L"Import complete:");
        EIDM_TRACE_INFO(L"  Total credentials: %u", stats.dwTotalCredentials);
        EIDM_TRACE_INFO(L"  Successfully imported: %u", stats.dwSuccessfullyImported);
        EIDM_TRACE_INFO(L"  Failed: %u", stats.dwFailed);
        EIDM_TRACE_INFO(L"  Users created: %u", stats.dwUsersCreated);
        EIDM_TRACE_INFO(L"  Groups created: %u", stats.dwGroupsCreated);
        EIDM_TRACE_INFO(L"  Warnings: %u", stats.dwWarnings);
    }

    return hr;
}

HRESULT ImportCredentials(
    _In_ const std::wstring& wsInputPath,
    _In_ const SecureWString& wsPassword,
    _In_ const IMPORT_OPTIONS& options,
    _Out_ IMPORT_STATS& stats)
{
    HRESULT hr = S_OK;

    {
        // Read and parse import file
        std::vector<CredentialInfo> credentials;
        std::vector<GroupInfo> groups;

        hr = ReadImportFile(wsInputPath, wsPassword, credentials, groups);
        if (FAILED(hr))
        {
            EIDM_TRACE_ERROR(L"Failed to read import file: 0x%08X", hr);
            return hr;
        }

        stats.dwTotalCredentials = static_cast<DWORD>(credentials.size());

        if (credentials.empty())
        {
            EIDM_TRACE_WARN(L"Warning: No credentials found in import file.");
            hr = S_FALSE;
            return hr;
        }

        if (options.fDryRun)
        {
            EIDM_TRACE_INFO(L"");
            EIDM_TRACE_INFO(L"=== DRY RUN MODE ===");
            EIDM_TRACE_INFO(L"Would import %u credentials:", stats.dwTotalCredentials);
            EIDM_TRACE_INFO(L"");

            // Validate each credential
            for (const auto& cred : credentials)
            {
                EIDM_TRACE_INFO(L"  User: %ls", cred.wsUsername.c_str());
                EIDM_TRACE_INFO(L"    Export RID: %u", cred.dwRid);

                // Check if user exists and get local RID
                BOOL fExists = FALSE;
                HRESULT hrCheck = UserExists(cred.wsUsername, fExists);
                if (SUCCEEDED(hrCheck))
                {
                    if (!fExists && options.fCreateUsers)
                    {
                        EIDM_TRACE_INFO(L"    Would create user account");
                    }
                    else if (!fExists)
                    {
                        EIDM_TRACE_WARN(L"    User does not exist (use -create-users)");
                        EIDM_TRACE_WARN(L"    SKIPPED: User account required");
                        continue;
                    }
                    else
                    {
                        DWORD dwLocalRid = 0;
                        if (SUCCEEDED(GetUserRid(cred.wsUsername, dwLocalRid)) && dwLocalRid != 0)
                        {
                            EIDM_TRACE_INFO(L"    User exists (local RID: %u)", dwLocalRid);
                            if (dwLocalRid != cred.dwRid)
                            {
                                EIDM_TRACE_INFO(L"    Note: RID differs from export (will use local RID)");
                            }
                        }
                    }
                }

                // Check certificate
                if (cred.Certificate.size() > 0)
                {
                    EIDM_TRACE_INFO(L"    Certificate: %u bytes - would be installed to MY store",
                        static_cast<DWORD>(cred.Certificate.size()));

                    // Check if already installed
                    if (IsCertificateInstalled(cred.Certificate.data(), static_cast<DWORD>(cred.Certificate.size())))
                    {
                        EIDM_TRACE_INFO(L"    Certificate already in store (will be updated)");
                    }
                }
                else
                {
                    EIDM_TRACE_WARN(L"    No certificate data!");
                }

                EIDM_TRACE_INFO(L"    LSA Secret: Would store for user %ls", cred.wsUsername.c_str());
                EIDM_TRACE_INFO(L"");
            }

            EIDM_TRACE_INFO(L"=== END OF DRY RUN ===");
            EIDM_TRACE_INFO(L"Use -force to perform actual import");
            EIDM_TRACE_INFO(L"");

            hr = S_OK;
            return hr;
        }

        // Actual import
        EIDM_TRACE_INFO(L"Importing credentials...");

        // Create groups first
        for (const auto& group : groups)
        {
            if (group.fBuiltin)
                continue;

            BOOL fExists = FALSE;
            GroupExists(group.wsName, fExists);

            if (!fExists)
            {
                if (SUCCEEDED(CreateLocalGroup(group.wsName, group.wsComment)))
                {
                    stats.dwGroupsCreated++;
                    EIDM_TRACE_VERBOSE(L"Created group: %ls", group.wsName.c_str());
                }
                else
                {
                    EIDM_TRACE_WARN(L"Warning: Could not create group: %ls", group.wsName.c_str());
                    stats.dwWarnings++;
                }
            }
        }

        // Import each credential
        for (const auto& cred : credentials)
        {
            BOOL fCreated = FALSE;
            HRESULT hrCred = ImportSingleCredential(cred, options, fCreated);

            if (fCreated)
                stats.dwUsersCreated++;

            if (SUCCEEDED(hrCred))
            {
                stats.dwSuccessfullyImported++;
                EIDM_TRACE_VERBOSE(L"Imported: %ls", cred.wsUsername.c_str());
            }
            else
            {
                stats.dwFailed++;

                if (!options.fContinueOnError)
                {
                    EIDM_TRACE_ERROR(L"Error importing credential for %ls: 0x%08X",
                        cred.wsUsername.c_str(), hrCred);
                    EIDM_TRACE_ERROR(L"Aborting import (use -continue-on-error to continue)");
                    hr = hrCred;
                    return hr;
                }
                else
                {
                    EIDM_TRACE_WARN(L"Warning: Failed to import %ls: 0x%08X",
                        cred.wsUsername.c_str(), hrCred);
                    stats.dwWarnings++;
                }
            }
        }

        // Sync group memberships
        EIDM_TRACE_INFO(L"Synchronizing group memberships...");

        // Build a map of username -> groups from the export file
        std::map<std::wstring, std::vector<std::wstring>> userGroupMap;

        for (const auto& group : groups)
        {
            for (const auto& member : group.wsMembers)
            {
                userGroupMap[member].push_back(group.wsName);
            }
        }

        // S memberships for each user
        for (const auto& cred : credentials)
        {
            const std::wstring& wsUsername = cred.wsUsername;

            // Skip if user wasn't successfully imported
            BOOL fUserExists = FALSE;
            if (FAILED(UserExists(wsUsername, fUserExists)) || !fUserExists)
            {
                continue;
            }

            // Get target groups for this user
            auto it = userGroupMap.find(wsUsername);
            if (it == userGroupMap.end())
            {
                EIDM_TRACE_VERBOSE(L"No group memberships found for %ls", wsUsername.c_str());
                continue;
            }

            const std::vector<std::wstring>& wsTargetGroups = it->second;

            // Synchronize group memberships
            DWORD dwChanges = 0;
            HRESULT hrGroup = SynchronizeGroupMemberships(wsUsername, wsTargetGroups, dwChanges);

            if (SUCCEEDED(hrGroup))
            {
                if (dwChanges > 0)
                {
                    EIDM_TRACE_INFO(L"Synchronized %ls: %u group changes applied",
                        wsUsername.c_str(), dwChanges);
                }
            }
            else
            {
                EIDM_TRACE_WARN(L"Warning: Failed to synchronize groups for %ls: 0x%08X",
                    wsUsername.c_str(), hrGroup);
                stats.dwWarnings++;
            }
        }

        hr = S_OK;
    }

    return hr;
}

HRESULT ReadImportFile(
    _In_ const std::wstring& wsInputPath,
    _In_ const SecureWString& wsPassword,
    _Out_ std::vector<CredentialInfo>& credentials,
    _Out_ std::vector<GroupInfo>& groups)
{
    EIDM_TRACE_VERBOSE(L"Reading import file: %ls", wsInputPath.c_str());

    ExportFileData data;
    HRESULT hr = ReadEncryptedFile(wsInputPath, wsPassword, data);

    if (SUCCEEDED(hr))
    {
        credentials = std::move(data.credentials);
        groups = std::move(data.groups);
    }

    return hr;
}

HRESULT ReadImportFileWithMetadata(
    _In_ const std::wstring& wsInputPath,
    _In_ const SecureWString& wsPassword,
    _Out_ std::vector<CredentialInfo>& credentials,
    _Out_ std::vector<GroupInfo>& groups,
    _Out_opt_ std::wstring* pwsSourceMachine,
    _Out_opt_ std::wstring* pwsExportDate,
    _Out_opt_ std::wstring* pwsExportedBy)
{
    EIDM_TRACE_VERBOSE(L"Reading import file with metadata: %ls", wsInputPath.c_str());

    ExportFileData data;
    HRESULT hr = ReadEncryptedFile(wsInputPath, wsPassword, data);

    if (SUCCEEDED(hr))
    {
        credentials = std::move(data.credentials);
        groups = std::move(data.groups);

        // Copy metadata if requested
        if (pwsSourceMachine)
            *pwsSourceMachine = data.wsSourceMachine;
        if (pwsExportDate)
            *pwsExportDate = Utf8ToWide(data.exportDate);
        if (pwsExportedBy)
            *pwsExportedBy = data.wsExportedBy;
    }

    return hr;
}

HRESULT ImportSingleCredential(
    _In_ const CredentialInfo& info,
    _In_ const IMPORT_OPTIONS& options,
    _Out_ BOOL& pfCreated)
{
    HRESULT hr = S_OK;
    pfCreated = FALSE;

    {
        EIDM_TRACE_VERBOSE(L"Importing credential for user: %ls (export RID: %u)",
            info.wsUsername.c_str(), info.dwRid);

        // Check if user exists
        BOOL fExists = FALSE;
        hr = UserExists(info.wsUsername, fExists);

        if (SUCCEEDED(hr) && !fExists)
        {
            if (options.fCreateUsers)
            {
                // Create user account with random password (EID auth works via smart card)
                std::wstring wsRandomPassword = GenerateRandomPassword(32);

                hr = CreateLocalUserAccount(info.wsUsername, L"", L"EID Smart Card User",
                    wsRandomPassword.c_str(), TRUE, TRUE);

                if (SUCCEEDED(hr))
                {
                    pfCreated = TRUE;
                    EIDM_TRACE_INFO(L"Created user account: %ls", info.wsUsername.c_str());
                }
                else
                {
                    EIDM_TRACE_ERROR(L"Failed to create user: %ls - 0x%08X",
                        info.wsUsername.c_str(), hr);
                    return hr;
                }
            }
            else
            {
                EIDM_TRACE_ERROR(L"User %ls does not exist (use -create-users)",
                    info.wsUsername.c_str());
                hr = HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER);
                return hr;
            }
        }

        // Get the ACTUAL RID for this user on the target machine
        // This is critical - the RID from export may not match the RID on this machine
        DWORD dwActualRid = 0;
        hr = GetUserRid(info.wsUsername, dwActualRid);

        if (FAILED(hr) || dwActualRid == 0)
        {
            EIDM_TRACE_ERROR(L"Failed to get RID for user %ls: 0x%08X",
                info.wsUsername.c_str(), hr);
            return hr;
        }

        if (dwActualRid != info.dwRid)
        {
            EIDM_TRACE_WARN(L"RID mismatch for %ls: export=%u, local=%u. Using local RID.",
                info.wsUsername.c_str(), info.dwRid, dwActualRid);
        }

        // Install the certificate to the user's MY store
        // This is REQUIRED for EID authentication to work!
        if (!info.Certificate.empty())
        {
            EIDM_TRACE_INFO(L"Installing certificate for user: %ls", info.wsUsername.c_str());

            // If running as admin, we can install to any user's store
            // For now, install to the target user's store
            HRESULT hrCert = InstallCertificateFromDER(info.Certificate, info.wsUsername);

            if (SUCCEEDED(hrCert))
            {
                EIDM_TRACE_INFO(L"Certificate installed successfully for: %ls", info.wsUsername.c_str());
            }
            else
            {
                EIDM_TRACE_ERROR(L"Failed to install certificate for %ls: 0x%08X",
                    info.wsUsername.c_str(), hrCert);

                // Certificate installation is critical for EID auth to work
                // Don't continue without it
                return hrCert;
            }
        }
        else
        {
            EIDM_TRACE_WARN(L"No certificate data for user: %ls", info.wsUsername.c_str());
        }

        // Create a copy of CredentialInfo with the correct RID
        CredentialInfo localInfo = info;
        localInfo.dwRid = dwActualRid;  // Use the actual RID from this machine

        // Check if a password was provided for this user
        std::wstring wsProvidedPassword;
        for (const auto& pair : options.userPasswords)
        {
            if (pair.first == info.wsUsername)
            {
                wsProvidedPassword = pair.second;
                break;
            }
        }

        // Set the user's password if one was provided
        if (!wsProvidedPassword.empty())
        {
            EIDM_TRACE_INFO(L"Setting password for user: %ls", info.wsUsername.c_str());

            HRESULT hrPwd = ::SetUserPassword(info.wsUsername, wsProvidedPassword.c_str());

            if (SUCCEEDED(hrPwd))
            {
                EIDM_TRACE_INFO(L"Password set successfully for: %ls", info.wsUsername.c_str());
            }
            else
            {
                EIDM_TRACE_WARN(L"Failed to set password for %ls: 0x%08X (continuing anyway)",
                    info.wsUsername.c_str(), hrPwd);
                // Don't fail the entire import if password setting fails
            }
        }

        // Import credential to LSA with the correct RID
        DWORD dwFlags = options.fCreateUsers ? 0x01 : 0x00; // CREATE_USER_IF_NOT_EXISTS
        hr = ImportLsaCredential(localInfo, dwFlags, pfCreated);

        if (SUCCEEDED(hr))
        {
            EIDM_TRACE_INFO(L"Successfully imported credential for: %ls (RID: %u)",
                info.wsUsername.c_str(), dwActualRid);
        }
    }

    return hr;
}

HRESULT ValidateCertificateForImport(
    _In_ PCCERT_CONTEXT pCertContext,
    _Out_ BOOL& pfTrusted,
    _Out_ std::vector<std::wstring>& warnings)
{
    if (!pCertContext)
    {
        pfTrusted = FALSE;
        warnings.push_back(L"No certificate provided");
        return E_INVALIDARG;
    }

    pfTrusted = TRUE;
    warnings.clear();

    // Get certificate validity period
    FILETIME ftNow;
    GetSystemTimeAsFileTime(&ftNow);

    // Check if certificate is expired
    if (CompareFileTime(&pCertContext->pCertInfo->NotAfter, &ftNow) < 0)
    {
        warnings.push_back(L"Certificate has expired");
        pfTrusted = FALSE;
        return S_OK;
    }

    // Check if certificate is not yet valid
    if (CompareFileTime(&pCertContext->pCertInfo->NotBefore, &ftNow) > 0)
    {
        warnings.push_back(L"Certificate is not yet valid");
        pfTrusted = FALSE;
        return S_OK;
    }

    // Check for expiration within 30 days
    FILETIME ftThirtyDays;
    ULARGE_INTEGER uli;
    uli.LowPart = ftNow.dwLowDateTime;
    uli.HighPart = ftNow.dwHighDateTime;
    uli.QuadPart += ULONGLONG(30) * 24 * 60 * 60 * 10000000; // 30 days in 100ns units
    ftThirtyDays.dwLowDateTime = uli.LowPart;
    ftThirtyDays.dwHighDateTime = uli.HighPart;

    if (CompareFileTime(&pCertContext->pCertInfo->NotAfter, &ftThirtyDays) < 0)
    {
        warnings.push_back(L"Certificate expires within 30 days");
        pfTrusted = TRUE; // Still trusted, just warning
    }

    return S_OK;
}

BOOL PromptForSmartCardPin(_In_ PCCERT_CONTEXT pCertContext, _Out_ SecureWString& wsPin)
{
    UNREFERENCED_PARAMETER(pCertContext);

    // Use the SecurePinPrompt dialog
    SecurePin pin;
    PIN_PROMPT_RESULT result = PromptForPIN(L"Enter smart card PIN:", pin);

    if (result == PIN_PROMPT_RESULT::SUCCESS)
    {
        // Convert SecurePin to SecureWString
        wsPin = SecureWString(pin.c_str());
        return TRUE;
    }

    return FALSE;
}

BOOL ValidateSmartCardPin(_In_ PCCERT_CONTEXT pCertContext, _In_ PCWSTR pwszPin)
{
    UNREFERENCED_PARAMETER(pCertContext);
    UNREFERENCED_PARAMETER(pwszPin);

    // Basic PIN validation
    if (!pwszPin || pwszPin[0] == L'\0')
        return FALSE;

    size_t cchLen = wcslen(pwszPin); // NOSONAR - pointer validated for NULL above (line 572)

    // PIN should be 4-8 digits for most smart cards
    if (cchLen < 4 || cchLen > 8)
        return FALSE;

    // All characters should be digits
    for (size_t i = 0; i < cchLen; i++)
    {
        if (!iswdigit(pwszPin[i]))
            return FALSE;
    }

    // Note: Actual smart card PIN validation requires communication with the card
    // via CryptoAPI or PKCS#11. This is a basic format check only.
    // The real validation happens when the credential is used for authentication.

    return TRUE;
}
