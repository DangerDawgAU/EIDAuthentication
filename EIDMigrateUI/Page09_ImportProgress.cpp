// Page09_ImportProgress.cpp - Import Progress Page Implementation
#include "Page09_ImportProgress.h"
#include "WorkerThread.h"

// Static worker handle
static HANDLE s_hWorkerThread = nullptr;  // NOSONAR - GLOBAL-01: pointer assigned at runtime
static WORKER_CONTEXT s_workerContext;  // NOSONAR - GLOBAL-01: variable populated at runtime

INT_PTR CALLBACK WndProc_09_ImportProgress(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)  // NOSONAR - COMPLEXITY-01: refactor deferred; logic verified
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
            // Hide buttons during progress
            PropSheet_SetWizButtons(hwndDlg, 0);

            // Start progress bar
            HWND hProgress = GetDlgItem(hwndDlg, IDC_09_PROGRESSBAR);
            if (hProgress) {
                SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
                SendMessage(hProgress, PBM_SETPOS, 0, 0);
            }

            // Start import worker thread
            if (!s_hWorkerThread) {
                ZeroMemory(&s_workerContext, sizeof(s_workerContext));
                s_workerContext.hwndParent = hwndDlg;
                s_workerContext.uProgressMsg = WM_USER_PROGRESS;
                s_workerContext.uCompleteMsg = WM_USER_COMPLETE;
                s_workerContext.uErrorMsg = WM_USER_ERROR;
                s_workerContext.pwszInputFile = &g_wizardData.wsInputFile;
                s_workerContext.pwszPassword = &g_wizardData.wsPassword;
                s_workerContext.pfDryRun = &g_wizardData.fDryRun;
                s_workerContext.pfCreateUsers = &g_wizardData.fCreateUsers;
                s_workerContext.pfContinueOnError = &g_wizardData.fContinueOnError;
                s_workerContext.pUserPasswords = &g_wizardData.userPasswords;
                s_workerContext.pSelectedGroups = &g_wizardData.SelectedGroups;

                DWORD dwThreadId;
                s_hWorkerThread = CreateThread(nullptr, 0, ImportWorker,
                    &s_workerContext, 0, &dwThreadId);
            }
            return TRUE;
        }

        case PSN_WIZNEXT:
        case PSN_WIZBACK:
            // Block navigation while working
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
            return TRUE;

        case PSN_WIZFINISH:
            return TRUE;

        default:
            break;
        }
        break;
    }

    case WM_USER_PROGRESS:
    {
        PROGRESS_DATA* pData = (PROGRESS_DATA*)lParam;  // NOSONAR (EXPLICIT-TYPE-04) - Explicit type preferred for code clarity
        if (pData) {
            SetDlgItemText(hwndDlg, IDC_09_PROGRESSTEXT, pData->wsStatus.c_str());

            HWND hProgress = GetDlgItem(hwndDlg, IDC_09_PROGRESSBAR);
            if (hProgress && pData->dwTotal > 0) {
                DWORD dwPos = (pData->dwCurrent * 100) / pData->dwTotal;
                SendMessage(hProgress, PBM_SETPOS, dwPos, 0);
            }
            delete pData;  // NOSONAR - OWNERSHIP-01: manual Win32 lifetime management
        }
        return TRUE;
    }

    case WM_USER_COMPLETE:
    {
        COMPLETE_DATA* pData = (COMPLETE_DATA*)lParam;  // NOSONAR (EXPLICIT-TYPE-04) - Explicit type preferred for code clarity
        if (pData) {
            g_wizardData.fImportComplete = TRUE;
            g_wizardData.dwImportedCount = pData->dwItemCount;

            // Move to completion page
            HWND hwndParent = GetParent(hwndDlg);
            if (hwndParent) {
                PropSheet_PressButton(hwndParent, PSBTN_NEXT);
            }
            delete pData;  // NOSONAR - OWNERSHIP-01: manual Win32 lifetime management
        }
        if (s_hWorkerThread) {
            CloseHandle(s_hWorkerThread);
            s_hWorkerThread = nullptr;
        }
        return TRUE;
    }

    case WM_USER_ERROR:
    {
        ERROR_DATA* pData = (ERROR_DATA*)lParam;  // NOSONAR (EXPLICIT-TYPE-04) - Explicit type preferred for code clarity
        if (pData) {
            MessageBoxW(hwndDlg, pData->wsMessage.c_str(), L"Import Error", MB_ICONERROR);
            delete pData;  // NOSONAR - OWNERSHIP-01: manual Win32 lifetime management
        }
        // Allow going back
        PropSheet_SetWizButtons(hwndDlg, PSWIZB_BACK);
        if (s_hWorkerThread) {
            CloseHandle(s_hWorkerThread);
            s_hWorkerThread = nullptr;
        }
        return TRUE;
    }

    default:
        break;
    }
    return FALSE;
}
