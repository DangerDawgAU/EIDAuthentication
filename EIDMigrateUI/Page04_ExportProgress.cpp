// Page04_ExportProgress.cpp - Export Progress Page Implementation
#include "Page04_ExportProgress.h"
#include "WorkerThread.h"

// Static worker handle
static HANDLE s_hWorkerThread = nullptr;
static WORKER_CONTEXT s_workerContext;

INT_PTR CALLBACK WndProc_04_ExportProgress(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
            // Hide buttons during progress
            PropSheet_SetWizButtons(hwndDlg, 0);

            // Start marquee progress
            HWND hProgress = GetDlgItem(hwndDlg, IDC_04_PROGRESSBAR);
            if (hProgress) {
                SendMessage(hProgress, PBM_SETMARQUEE, TRUE, 0);
            }

            // Start export worker thread
            if (!s_hWorkerThread) {
                ZeroMemory(&s_workerContext, sizeof(s_workerContext));
                s_workerContext.hwndParent = hwndDlg;
                s_workerContext.uProgressMsg = WM_USER_PROGRESS;
                s_workerContext.uCompleteMsg = WM_USER_COMPLETE;
                s_workerContext.uErrorMsg = WM_USER_ERROR;
                s_workerContext.pwszOutputFile = &g_wizardData.wsOutputFile;
                s_workerContext.pwszPassword = &g_wizardData.wsPassword;
                s_workerContext.pfValidateCerts = &g_wizardData.fValidateCerts;
                s_workerContext.pfIncludeGroups = &g_wizardData.fIncludeGroups;

                DWORD dwThreadId;
                s_hWorkerThread = CreateThread(nullptr, 0, ExportWorker,
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
        PROGRESS_DATA* pData = (PROGRESS_DATA*)lParam;
        if (pData) {
            SetDlgItemText(hwndDlg, IDC_04_PROGRESSTEXT, pData->wsStatus.c_str());
            delete pData;
        }
        return TRUE;
    }

    case WM_USER_COMPLETE:
    {
        COMPLETE_DATA* pData = (COMPLETE_DATA*)lParam;
        if (pData) {
            g_wizardData.fExportComplete = TRUE;
            g_wizardData.dwExportedCount = pData->dwItemCount;

            // Move to completion page
            HWND hwndParent = GetParent(hwndDlg);
            if (hwndParent) {
                PropSheet_PressButton(hwndParent, PSBTN_NEXT);
            }
            delete pData;
        }
        if (s_hWorkerThread) {
            CloseHandle(s_hWorkerThread);
            s_hWorkerThread = nullptr;
        }
        return TRUE;
    }

    case WM_USER_ERROR:
    {
        ERROR_DATA* pData = (ERROR_DATA*)lParam;
        if (pData) {
            MessageBoxW(hwndDlg, pData->wsMessage.c_str(), L"Export Error", MB_ICONERROR);
            delete pData;
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
