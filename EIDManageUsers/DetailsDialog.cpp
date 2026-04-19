// DetailsDialog.cpp - User details dialog implementation
#include "DetailsDialog.h"
#include "../EIDMigrate/LsaClient.h"
#include <windowsx.h>

// Helper: Convert bytes to hex string (local implementation to avoid include issues)
static std::string BytesToHex(_In_reads_bytes_(cbBytes) const BYTE* pbBytes, _In_ DWORD cbBytes)
{
    std::string result;
    result.reserve(cbBytes * 2);
    for (DWORD i = 0; i < cbBytes; i++)
    {
        char szHex[3];
        sprintf_s(szHex, sizeof(szHex), "%02X", pbBytes[i]);
        result += szHex;
    }
    return result;
}

// Store user pointer for dialog
static const UserInfo* g_pUser = nullptr;

// Format user details
std::wstring FormatUserDetails(_In_ const UserInfo& user)
{
    WCHAR szDetails[4096];
    DWORD dwPos = 0;

    dwPos += swprintf_s(szDetails + dwPos, ARRAYSIZE(szDetails) - dwPos,
        L"User Details for: %s\r\n\r\n", user.wsUsername.c_str());

    dwPos += swprintf_s(szDetails + dwPos, ARRAYSIZE(szDetails) - dwPos,
        L"RID: %u\r\n", user.dwRid);

    dwPos += swprintf_s(szDetails + dwPos, ARRAYSIZE(szDetails) - dwPos,
        L"SID: %s\r\n", user.wsSid.c_str());

    dwPos += swprintf_s(szDetails + dwPos, ARRAYSIZE(szDetails) - dwPos,
        L"Has EID Credential: %s\r\n",
        user.fHasEIDCredential ? L"Yes" : L"No");

    PCWSTR pwszEnc = (user.EncryptionType == EID_PRIVATE_DATA_TYPE::eidpdtCrypted) ? L"Certificate-based" :
                     (user.EncryptionType == EID_PRIVATE_DATA_TYPE::eidpdtDPAPI) ? L"DPAPI" : L"None";
    dwPos += swprintf_s(szDetails + dwPos, ARRAYSIZE(szDetails) - dwPos,
        L"Encryption Type: %s\r\n", pwszEnc);

    dwPos += swprintf_s(szDetails + dwPos, ARRAYSIZE(szDetails) - dwPos,
        L"Last Login: %s\r\n\r\n", user.wsLastLogin.c_str());

    // Certificate hash
    if (user.fHasEIDCredential)
    {
        dwPos += swprintf_s(szDetails + dwPos, ARRAYSIZE(szDetails) - dwPos,
            L"--- Certificate Information ---\r\n");

        std::string sHash = BytesToHex(user.CertificateHash, CERT_HASH_LENGTH);
        dwPos += swprintf_s(szDetails + dwPos, ARRAYSIZE(szDetails) - dwPos,
            L"Certificate Hash: %S\r\n\r\n", sHash.c_str());
    }

    // Groups
    dwPos += swprintf_s(szDetails + dwPos, ARRAYSIZE(szDetails) - dwPos,
        L"--- Group Membership ---\r\n");
    if (user.wsGroups.empty())
    {
        dwPos += swprintf_s(szDetails + dwPos, ARRAYSIZE(szDetails) - dwPos,
            L"No group memberships found.\r\n");
    }
    else
    {
        for (const auto& group : user.wsGroups)
        {
            dwPos += swprintf_s(szDetails + dwPos, ARRAYSIZE(szDetails) - dwPos,
                L"• %s\r\n", group.c_str());
        }
    }

    return std::wstring(szDetails);
}

// Initialize details dialog
void InitializeDetailsDialog(HWND hwndDlg, _In_ const UserInfo* pUser)
{
    g_pUser = pUser;

    // Set window title
    WCHAR szTitle[256];
    swprintf_s(szTitle, ARRAYSIZE(szTitle),
        L"User Details - %s", pUser->wsUsername.c_str());
    SetWindowTextW(hwndDlg, szTitle);

    // Get details text control
    HWND hDetails = GetDlgItem(hwndDlg, IDC_DETAILS_TEXT);
    if (hDetails)
    {
        std::wstring wsDetails = FormatUserDetails(*pUser);
        SetWindowTextW(hDetails, wsDetails.c_str());
    }
}

// Details dialog procedure
INT_PTR CALLBACK WndProc_Details(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        SetWindowIcon(hwndDlg);
        InitializeDetailsDialog(hwndDlg,
            reinterpret_cast<const UserInfo*>(lParam));
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDCLOSE:
        case IDOK:
        case IDCANCEL:
            EndDialog(hwndDlg, IDOK);
            return TRUE;
        }
        break;

    default:
        break;
    }

    return FALSE;
}
