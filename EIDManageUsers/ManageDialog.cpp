// ManageDialog.cpp - Manage selected users dialog implementation
#include "ManageDialog.h"
#include "EIDUserManagement.h"
#include "CredentialOps.h"
#include "CertificateOps.h"
#include "../EIDMigrate/Tracing.h"
#include <windowsx.h>
#include <commctrl.h>
#include <vector>

// Store selected users for the dialog
static const std::vector<UserInfo>* g_pSelectedUsers = nullptr;

// Operation flags
static BOOL g_fRemoveCred = FALSE;
static BOOL g_fRemoveCerts = FALSE;
static BOOL g_fRemoveGroups = FALSE;
static BOOL g_fDeleteAccount = FALSE;

// Initialize manage dialog
void InitializeManageDialog(HWND hwndDlg, _In_ const std::vector<UserInfo>* pUsers)
{
    g_pSelectedUsers = pUsers;

    // Set title with user count
    WCHAR szTitle[256];
    swprintf_s(szTitle, ARRAYSIZE(szTitle),
        L"Manage Selected Users (%zu user(s) selected)",
        pUsers->size());
    SetWindowTextW(hwndDlg, szTitle);

    // Reset operation checkboxes
    HWND hRemoveCred = GetDlgItem(hwndDlg, IDC_MANAGE_REMOVE_CRED);
    HWND hRemoveCerts = GetDlgItem(hwndDlg, IDC_MANAGE_REMOVE_CERTS);
    HWND hRemoveGroups = GetDlgItem(hwndDlg, IDC_MANAGE_REMOVE_GROUPS);
    HWND hDeleteAccount = GetDlgItem(hwndDlg, IDC_MANAGE_DELETE_ACCOUNT);

    if (hRemoveCred) Button_SetCheck(hRemoveCred, BST_UNCHECKED);
    if (hRemoveCerts) Button_SetCheck(hRemoveCerts, BST_UNCHECKED);
    if (hRemoveGroups) Button_SetCheck(hRemoveGroups, BST_UNCHECKED);
    if (hDeleteAccount) Button_SetCheck(hDeleteAccount, BST_UNCHECKED);

    g_fRemoveCred = FALSE;
    g_fRemoveCerts = FALSE;
    g_fRemoveGroups = FALSE;
    g_fDeleteAccount = FALSE;
}

// Update operation display
void UpdateOperationDisplay(HWND hwndDlg)
{
    HWND hRemoveCred = GetDlgItem(hwndDlg, IDC_MANAGE_REMOVE_CRED);
    HWND hRemoveCerts = GetDlgItem(hwndDlg, IDC_MANAGE_REMOVE_CERTS);
    HWND hRemoveGroups = GetDlgItem(hwndDlg, IDC_MANAGE_REMOVE_GROUPS);
    HWND hDeleteAccount = GetDlgItem(hwndDlg, IDC_MANAGE_DELETE_ACCOUNT);

    g_fRemoveCred = (hRemoveCred && Button_GetCheck(hRemoveCred) == BST_CHECKED);
    g_fRemoveCerts = (hRemoveCerts && Button_GetCheck(hRemoveCerts) == BST_CHECKED);
    g_fRemoveGroups = (hRemoveGroups && Button_GetCheck(hRemoveGroups) == BST_CHECKED);
    g_fDeleteAccount = (hDeleteAccount && Button_GetCheck(hDeleteAccount) == BST_CHECKED);

    // If delete account is selected, disable other options (they're redundant)
    BOOL fEnable = !g_fDeleteAccount;
    if (hRemoveCred) EnableWindow(hRemoveCred, fEnable);
    if (hRemoveCerts) EnableWindow(hRemoveCerts, fEnable);
    if (hRemoveGroups) EnableWindow(hRemoveGroups, fEnable);
}

// Show confirmation dialog
BOOL ShowManagementConfirmation(HWND hwndDlg,
    _In_ const std::vector<UserInfo>& users,
    DWORD dwOperations)
{
    std::wstring wsMessage = L"You are about to perform the following operations:\n\n";

    if (dwOperations & OP_REMOVE_CREDENTIAL)
        wsMessage += L"✓ Remove EID credentials (LSA secret)\n";
    if (dwOperations & OP_REMOVE_CERTIFICATES)
        wsMessage += L"✓ Remove certificates from user store\n";
    if (dwOperations & OP_REMOVE_FROM_GROUPS)
        wsMessage += L"✓ Remove from EID-related groups\n";
    if (dwOperations & OP_DELETE_ACCOUNT)
        wsMessage += L"✓ DELETE USER ACCOUNT AND PROFILE\n";

    wsMessage += L"\nOn the following user(s):\n\n";

    for (const auto& user : users)
    {
        wsMessage += L"• " + user.wsUsername + L"\n";
    }

    wsMessage += L"\nThese actions CANNOT be undone!\n\nAre you sure?";

    return MessageBoxW(hwndDlg, wsMessage.c_str(),
        L"Confirm Management Operations",
        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES;
}

// Execute management operations
HRESULT ExecuteManagementOperations(HWND hwndDlg, _In_ const std::vector<UserInfo>& users)
{
    DWORD dwSuccess = 0;
    DWORD dwFailed = 0;
    std::vector<std::wstring> failedUsers;

    for (const auto& user : users)
    {
        HRESULT hrUser = S_OK;
        BOOL fAnyOp = FALSE;

        // Delete account (includes profile)
        if (g_fDeleteAccount)
        {
            fAnyOp = TRUE;

            // Delete profile first
            DeleteUserProfile(user.wsUsername);

            // Then delete account
            hrUser = DeleteUserAccount(user.wsUsername);

            if (SUCCEEDED(hrUser))
            {
                EIDM_TRACE_INFO(L"Deleted account '%ls'", user.wsUsername.c_str());
            }
            else
            {
                EIDM_TRACE_ERROR(L"Failed to delete account '%ls': 0x%08X",
                    user.wsUsername.c_str(), hrUser);
                dwFailed++;
                failedUsers.push_back(user.wsUsername);
                continue;
            }
        }
        else
        {
            // Individual operations

            // Remove EID credential
            if (g_fRemoveCred)
            {
                fAnyOp = TRUE;
                HRESULT hr = RemoveEIDCredential(user.dwRid);
                if (FAILED(hr))
                {
                    EIDM_TRACE_WARN(L"Failed to remove credential for '%ls'", user.wsUsername.c_str());
                    hrUser = hr;
                }
            }

            // Remove certificates
            if (g_fRemoveCerts)
            {
                fAnyOp = TRUE;
                HRESULT hr = RemoveUserCertificates(user.wsUsername);
                if (FAILED(hr))
                {
                    EIDM_TRACE_WARN(L"Failed to remove certificates for '%ls'", user.wsUsername.c_str());
                    hrUser = hr;
                }
            }

            // Remove from groups
            if (g_fRemoveGroups)
            {
                fAnyOp = TRUE;
                HRESULT hr = RemoveUserFromGroups(user.wsUsername, g_appState.EIDGroups);
                if (FAILED(hr))
                {
                    EIDM_TRACE_WARN(L"Failed to remove '%ls' from groups", user.wsUsername.c_str());
                    hrUser = hr;
                }
            }
        }

        if (SUCCEEDED(hrUser))
            dwSuccess++;
        else
        {
            dwFailed++;
            failedUsers.push_back(user.wsUsername);
        }
    }

    // Show results
    WCHAR szResult[512];
    swprintf_s(szResult, ARRAYSIZE(szResult),
        L"Operations completed:\n\n"
        L"Success: %u\n"
        L"Failed: %u",
        dwSuccess, dwFailed);

    if (!failedUsers.empty())
    {
        wcscat_s(szResult, L"\n\nFailed users:");
        for (const auto& wsUser : failedUsers)
        {
            if (wcslen(szResult) < ARRAYSIZE(szResult) - wsUser.length() - 5)
            {
                wcscat_s(szResult, L"\n• ");
                wcscat_s(szResult, wsUser.c_str());
            }
        }
    }

    MessageBoxW(hwndDlg, szResult,
        L"Management Operations Complete",
        MB_ICONINFORMATION);

    return dwFailed > 0 ? S_FALSE : S_OK;
}

// Manage dialog procedure
INT_PTR CALLBACK WndProc_Manage(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        SetWindowIcon(hwndDlg);
        InitializeManageDialog(hwndDlg,
            reinterpret_cast<const std::vector<UserInfo>*>(lParam));
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_MANAGE_REMOVE_CRED:
        case IDC_MANAGE_REMOVE_CERTS:
        case IDC_MANAGE_REMOVE_GROUPS:
        case IDC_MANAGE_DELETE_ACCOUNT:
            UpdateOperationDisplay(hwndDlg);
            return TRUE;

        case IDOK:
        {
            if (!g_pSelectedUsers || g_pSelectedUsers->empty())
            {
                EndDialog(hwndDlg, IDCANCEL);
                return TRUE;
            }

            // Build operation flags
            DWORD dwOps = 0;
            if (g_fRemoveCred) dwOps |= OP_REMOVE_CREDENTIAL;
            if (g_fRemoveCerts) dwOps |= OP_REMOVE_CERTIFICATES;
            if (g_fRemoveGroups) dwOps |= OP_REMOVE_FROM_GROUPS;
            if (g_fDeleteAccount) dwOps |= OP_DELETE_ACCOUNT;

            if (dwOps == 0)
            {
                MessageBoxW(hwndDlg,
                    L"Please select at least one operation to perform.",
                    L"No Operations Selected",
                    MB_ICONINFORMATION);
                return TRUE;
            }

            // Show confirmation if enabled
            if (g_appState.fConfirmActions)
            {
                if (!ShowManagementConfirmation(hwndDlg, *g_pSelectedUsers, dwOps))
                {
                    return TRUE;  // User cancelled
                }
            }

            // Execute operations
            ExecuteManagementOperations(hwndDlg, *g_pSelectedUsers);
            EndDialog(hwndDlg, IDOK);
            return TRUE;
        }

        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
        }
        break;

    default:
        break;
    }

    return FALSE;
}
