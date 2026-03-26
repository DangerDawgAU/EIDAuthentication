// Page07_ImportOptions.cpp - Import Options Page Implementation
#include "Page07_ImportOptions.h"

INT_PTR CALLBACK WndProc_07_ImportOptions(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        return TRUE;

    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        switch (pnmh->code)
        {
        case PSN_SETACTIVE:
        {
            // Set defaults based on wizard state
            HWND hDryRun = GetDlgItem(hwndDlg, IDC_07_DRY_RUN);
            HWND hForce = GetDlgItem(hwndDlg, IDC_07_FORCE_IMPORT);
            if (g_wizardData.fDryRun && hDryRun) Button_SetCheck(hDryRun, BST_CHECKED);
            else if (hForce) Button_SetCheck(hForce, BST_CHECKED);

            // Show warning
            std::wstring wsWarning = L"This will import ";
            WCHAR szCount[32];
            swprintf_s(szCount, ARRAYSIZE(szCount), L"%u", g_wizardData.dwFileCredentialCount);
            wsWarning += szCount;
            wsWarning += L" credentials from ";
            wsWarning += g_wizardData.wsSourceMachine;
            wsWarning += L".\r\n\r\n";
            wsWarning += L"Administrator privileges are required.";
            SetDlgItemText(hwndDlg, IDC_07_WARNING_TEXT, wsWarning.c_str());

            PropSheet_SetWizButtons(hwndDlg, PSWIZB_BACK | PSWIZB_NEXT);
            return TRUE;
        }

        case PSN_WIZNEXT:
        {
            // Store import options
            HWND hDryRun = GetDlgItem(hwndDlg, IDC_07_DRY_RUN);
            HWND hCreateUsers = GetDlgItem(hwndDlg, IDC_07_CREATE_USERS);
            HWND hContinue = GetDlgItem(hwndDlg, IDC_07_CONTINUE_ON_ERROR);
            HWND hOverwrite = GetDlgItem(hwndDlg, IDC_07_OVERWRITE);

            g_wizardData.fDryRun = (hDryRun && Button_GetCheck(hDryRun) == BST_CHECKED);
            g_wizardData.fCreateUsers = (hCreateUsers && Button_GetCheck(hCreateUsers) == BST_CHECKED);
            g_wizardData.fContinueOnError = (hContinue && Button_GetCheck(hContinue) == BST_CHECKED);
            g_wizardData.fOverwrite = (hOverwrite && Button_GetCheck(hOverwrite) == BST_CHECKED);

            // Clear previous password prompts
            g_wizardData.userPasswords.clear();

            // If not doing dry run and credentials have certificates, prompt for passwords
            if (!g_wizardData.fDryRun && !g_wizardData.credentials.empty())
            {
                HWND hwndParent = GetParent(hwndDlg);

                // Prompt for password for each credential with a certificate
                for (const auto& cred : g_wizardData.credentials)
                {
                    // Only prompt if there's a certificate (not just DPAPI-encrypted)
                    if (cred.EncryptionType == EID_PRIVATE_DATA_TYPE::eidpdtCrypted &&
                        !cred.Certificate.empty())
                    {
                        std::wstring wsPassword;
                        BOOL fSkipped = FALSE;

                        BOOL fResult = ShowPasswordPromptDialog(
                            hwndParent,
                            cred.wsUsername,
                            wsPassword,
                            fSkipped);

                        if (fResult && !fSkipped)
                        {
                            // User provided a password, store it for later
                            g_wizardData.userPasswords.push_back(
                                std::make_pair(cred.wsUsername, wsPassword));
                        }
                        // If skipped, no password will be set (smart card only)
                    }
                }
            }

            return TRUE;
        }
        default:
            break;
        }
        break;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == IDC_07_DRY_RUN || LOWORD(wParam) == IDC_07_FORCE_IMPORT) {
            // Update button text
            HWND hwndPS = GetParent(hwndDlg);
            if (hwndPS) {
                HWND hDryRun = GetDlgItem(hwndDlg, IDC_07_DRY_RUN);
                if (hDryRun && Button_GetCheck(hDryRun) == BST_CHECKED) {
                    SetWindowTextW(GetDlgItem(hwndPS, IDCANCEL), L"Cancel");
                } else {
                    SetWindowTextW(GetDlgItem(hwndPS, IDOK), L"Import");
                }
            }
            return TRUE;
        }
        break;
    }

    default:
        break;
    }
    return FALSE;
}
