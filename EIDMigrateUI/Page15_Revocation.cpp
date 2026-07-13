// Page15_Revocation.cpp - Certificate Revocation Management Page Implementation
//
// Lets the operator control certificate revocation entirely from the GUI (no command line):
//   1. Install / update a signature-verified CRL for OFFLINE revocation checking
//      (delegates to InstallCrlFromFile, shared with EIDMigrate's import-crl command).
//   2. Toggle the "Require revocation checking" policy (RequireRevocationCheck) which makes
//      the auth stack fail-closed when a card's revocation status cannot be confirmed offline.
#include "Page15_Revocation.h"
#include "../EIDMigrate/CertificateInstall.h"
#include <commdlg.h>

// Same policy key/value the auth stack reads (EIDCardLibrary\GPO.cpp: szMainGPOKey).
static const wchar_t* const SC_POLICY_KEY =
    L"SOFTWARE\\Policies\\Microsoft\\Windows\\SmartCardCredentialProvider";
static const wchar_t* const REQUIRE_REVOCATION_VALUE = L"RequireRevocationCheck";

static void LoadRequireRevocation(HWND hwndDlg)
{
    DWORD dwVal = 0;
    DWORD cb = sizeof(dwVal);
    LSTATUS s = RegGetValueW(HKEY_LOCAL_MACHINE, SC_POLICY_KEY, REQUIRE_REVOCATION_VALUE,
        RRF_RT_REG_DWORD, nullptr, &dwVal, &cb);
    BOOL fChecked = (s == ERROR_SUCCESS && dwVal != 0);
    CheckDlgButton(hwndDlg, IDC_15_REQUIRE_REVOCATION, fChecked ? BST_CHECKED : BST_UNCHECKED);
}

static void SaveRequireRevocation(HWND hwndDlg)
{
    DWORD dwVal = (IsDlgButtonChecked(hwndDlg, IDC_15_REQUIRE_REVOCATION) == BST_CHECKED) ? 1u : 0u;
    LSTATUS s = RegSetKeyValueW(HKEY_LOCAL_MACHINE, SC_POLICY_KEY, REQUIRE_REVOCATION_VALUE,
        REG_DWORD, &dwVal, sizeof(dwVal));
    if (s != ERROR_SUCCESS)
    {
        MessageBoxW(hwndDlg,
            L"Could not update the revocation policy. Administrator rights are required to change machine policy.",
            L"Revocation", MB_ICONWARNING);
        // Restore the checkbox to the on-disk value so the UI reflects reality.
        LoadRequireRevocation(hwndDlg);
    }
    else if (dwVal != 0)
    {
        SetDlgItemText(hwndDlg, IDC_15_RESULTS,
            L"Revocation checking is now REQUIRED. A card will be refused if its revocation "
            L"status cannot be confirmed from a locally installed CRL.");
    }
    else
    {
        SetDlgItemText(hwndDlg, IDC_15_RESULTS,
            L"Revocation checking is now optional. Logon is allowed when revocation status is unknown.");
    }
}

INT_PTR CALLBACK WndProc_15_Revocation(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        return TRUE;

    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;  // NOSONAR (EXPLICIT-TYPE-04) - Explicit type preferred for code clarity
        switch (pnmh->code)
        {
        case PSN_SETACTIVE:
        {
            LoadRequireRevocation(hwndDlg);
            SetDlgItemText(hwndDlg, IDC_15_RESULTS, L"");

            // Change Cancel button to Close (this is a terminal branch off Welcome)
            HWND hwndPS = GetParent(hwndDlg);
            if (hwndPS) {
                SetWindowTextW(GetDlgItem(hwndPS, IDCANCEL), L"Close");
            }

            // Enable Back button to return to Welcome, hide Next
            PropSheet_SetWizButtons(hwndDlg, PSWIZB_BACK);
            return TRUE;
        }

        case PSN_WIZBACK:
        {
            // Jump back to Welcome page (index 0) instead of sequential back
            HWND hwndParent = GetParent(hwndDlg);
            if (hwndParent) {
                PropSheet_SetCurSel(hwndParent, nullptr, 0);
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);  // Prevent default back
            }
            return TRUE;
        }

        case PSN_WIZFINISH:
            return TRUE;

        default:
            break;
        }
        break;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_15_BROWSE:
        {
            WCHAR szFile[MAX_PATH] = L"";  // NOSONAR - LSASS-01: C-style buffer required by Win32 API
            OPENFILENAME ofn = {0};
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = hwndDlg;
            ofn.lpstrFilter = L"CRL Files (*.crl)\0*.crl\0All Files\0*.*\0";
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = ARRAYSIZE(szFile);
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

            if (GetOpenFileName(&ofn)) {
                SetDlgItemText(hwndDlg, IDC_15_CRL_PATH, szFile);
            }
            return TRUE;
        }

        case IDC_15_INSTALL:
        {
            WCHAR szFile[MAX_PATH];  // NOSONAR - LSASS-01: C-style buffer required by Win32 API
            GetDlgItemText(hwndDlg, IDC_15_CRL_PATH, szFile, ARRAYSIZE(szFile));

            if (wcslen(szFile) == 0) { // NOSONAR - szFile is stack-allocated buffer, never NULL
                MessageBoxW(hwndDlg, L"Please select a CRL file (.crl) to install.",
                    L"Revocation", MB_ICONEXCLAMATION);
                return TRUE;
            }

            HRESULT hr = InstallCrlFromFile(szFile);
            if (SUCCEEDED(hr)) {
                SetDlgItemText(hwndDlg, IDC_15_RESULTS,
                    L"CRL installed successfully into the machine CA store. Offline revocation "
                    L"checking will use it immediately.");
                MessageBoxW(hwndDlg, L"CRL installed successfully.",
                    L"Revocation", MB_ICONINFORMATION);
            } else {
                WCHAR szMsg[320];  // NOSONAR - LSASS-01: C-style buffer required by Win32 API
                swprintf_s(szMsg, ARRAYSIZE(szMsg),
                    L"Failed to install the CRL (0x%08X).\n\nThe file must be a valid CRL signed "
                    L"by a certification authority already trusted on this machine, and "
                    L"administrator rights are required.", (unsigned)hr);
                SetDlgItemText(hwndDlg, IDC_15_RESULTS, szMsg);
                MessageBoxW(hwndDlg, szMsg, L"Revocation", MB_ICONERROR);
            }
            return TRUE;
        }

        case IDC_15_REQUIRE_REVOCATION:
            SaveRequireRevocation(hwndDlg);
            return TRUE;

        default:
            break;
        }
        break;
    }

    default:
        break;
    }
    return FALSE;
}
