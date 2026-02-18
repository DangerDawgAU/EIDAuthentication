// EIDLogManager.cpp�: d�finit le point d'entr�e pour l'application.
//

#include "stdafx.h"
#include "EIDLogManager.h"
#include <tchar.h>
#include <ShObjIdl.h>
#include <shlobj.h>
#include "../EIDCardLibrary/Registration.h"
#include "../EIDCardLibrary/Tracing.h"
#include "../EIDCardLibrary/TraceExport.h"

#pragma comment(lib,"comctl32")
#pragma comment(lib,"Shell32")
#pragma comment(lib,"crypt32")

#ifdef UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

// Local override of Windows SDK CLSCTX_INPROC_SERVER (typically CLSCTX_INPROC_SERVER = 0x1 from objbase.h)
// Using constexpr for type safety while maintaining the same value
constexpr LONG CLSCTX_INPROC_SERVER_LOCAL = 1;

// Variables globales�:
HINSTANCE hInst;								// instance actuelle

INT_PTR CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	DialogBox(hInst, MAKEINTRESOURCE(IDD_EIDLOGMANAGER_DIALOG), nullptr, WndProc);
}

void SaveLog(HWND hDlg);
void DeleteLog(HWND hDlg);

void ShowHideLogButtons(HWND hDlg)
{
	if (IsLoggingEnabled())
	{
		EnableWindow(GetDlgItem(hDlg,IDC_ENABLELOG), FALSE);
		EnableWindow(GetDlgItem(hDlg,IDC_DISABLELOG), TRUE);
	}
	else
	{
		EnableWindow(GetDlgItem(hDlg,IDC_ENABLELOG), TRUE);
		EnableWindow(GetDlgItem(hDlg,IDC_DISABLELOG), FALSE);
	}
}

void ShowHideCrashDumpButtons(HWND hDlg)
{
	if (IsCrashDumpEnabled())
	{
		EnableWindow(GetDlgItem(hDlg,IDC_ENABLECRASHDUMP), FALSE);
		EnableWindow(GetDlgItem(hDlg,IDC_DISABLECRASHDUMP), TRUE);
	}
	else
	{
		EnableWindow(GetDlgItem(hDlg,IDC_ENABLECRASHDUMP), TRUE);
		EnableWindow(GetDlgItem(hDlg,IDC_DISABLECRASHDUMP), FALSE);
	}
}
// Gestionnaire de messages pour la bo�te de dialogue � propos de.
INT_PTR CALLBACK WndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
		case WM_CLOSE:
          EndDialog(hDlg, 0);
          break;

		case WM_INITDIALOG:
			ShowHideLogButtons(hDlg);
			ShowHideCrashDumpButtons(hDlg);
			return (INT_PTR)TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_ENABLELOG:
					if (!EnableLogging())
					{
						MessageBoxWin32Ex(GetLastError(), hDlg);
					}
					ShowHideLogButtons(hDlg);
					break;	
				case IDC_DISABLELOG:
					if (!DisableLogging())
					{
						MessageBoxWin32Ex(GetLastError(), hDlg);
					}
					ShowHideLogButtons(hDlg);
					break;
				case IDC_SAVELOG:
					SaveLog(hDlg);
					break;
				case IDC_CLEARLOG:
					DeleteLog(hDlg);
					break;
				case IDC_ENABLECRASHDUMP:
					{
						TCHAR strPath[ MAX_PATH ];
						SHGetSpecialFolderPath(hDlg,  strPath, CSIDL_DESKTOPDIRECTORY, FALSE );
						EnableCrashDump(strPath);
						ShowHideCrashDumpButtons(hDlg);
					}
					break;
				case IDC_DISABLECRASHDUMP:
					DisableCrashDump();
					ShowHideCrashDumpButtons(hDlg);
					break;
			}
			break;
	}
	return (INT_PTR)FALSE;
}

HANDLE hFile = nullptr;

void SaveLog(HWND hDlg)
{
	IFileSaveDialog *pSaveDialog;
	
	LPOLESTR pszPath = nullptr;
    __try
	{
		HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog,
									  nullptr,
									  CLSCTX_INPROC_SERVER_LOCAL,
									  IID_PPV_ARGS(&pSaveDialog));

		if (!SUCCEEDED(hr))
		{
			return;
		}
		pSaveDialog->SetDefaultExtension(TEXT("txt"));
		pSaveDialog->SetFileName(TEXT("Report.txt"));
        pSaveDialog->SetOptions(FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT | FOS_DONTADDTORECENT);
		// show the dialog:
        hr = pSaveDialog->Show(hDlg);

        if(SUCCEEDED(hr))
        {
            IShellItem *ppsi;
            // this will fail if Cancel has been clicked:
            hr = pSaveDialog->GetResult(&ppsi);

            if(SUCCEEDED(hr))
            {
                    // extract the path:
                    hr = ppsi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
                    ppsi->Release();
            }
        }

        pSaveDialog->Release();

		if (!SUCCEEDED(hr))
		{
			__leave;
		}
	
		

		hFile = CreateFile(pszPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			__leave;
		}
		// Static buffers for ExportOneTraceFile (requires non-const PTSTR)
		static TCHAR s_szTraceFile0[] = TEXT("c:\\Windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl");
		static TCHAR s_szTraceFile1[] = TEXT("c:\\Windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl.001");
		static TCHAR s_szTraceFile2[] = TEXT("c:\\Windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl.002");
		static TCHAR s_szTraceFile3[] = TEXT("c:\\Windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl.003");
		static TCHAR s_szTraceFile4[] = TEXT("c:\\Windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl.004");
		static TCHAR s_szTraceFile5[] = TEXT("c:\\Windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl.005");
		static TCHAR s_szTraceFile6[] = TEXT("c:\\Windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl.006");
		static TCHAR s_szTraceFile7[] = TEXT("c:\\Windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl.007");
		static TCHAR s_szTraceFile8[] = TEXT("c:\\Windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl.008");
		ExportOneTraceFile(hFile, s_szTraceFile0);
		ExportOneTraceFile(hFile, s_szTraceFile1);
		ExportOneTraceFile(hFile, s_szTraceFile2);
		ExportOneTraceFile(hFile, s_szTraceFile3);
		ExportOneTraceFile(hFile, s_szTraceFile4);
		ExportOneTraceFile(hFile, s_szTraceFile5);
		ExportOneTraceFile(hFile, s_szTraceFile6);
		ExportOneTraceFile(hFile, s_szTraceFile7);
		ExportOneTraceFile(hFile, s_szTraceFile8);
	}
	__finally
	{
		if (pszPath)
			CoTaskMemFree(pszPath);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
		}
	}
}

void DeleteLog(HWND hDlg)
{
	if (IsLoggingEnabled())
	{
		MessageBox(hDlg, TEXT("Tracing must be disabled"),TEXT("Error"),0);
	}
	else
	{
		DeleteFile(TEXT("c:\\Windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl"));
		DeleteFile(TEXT("c:\\Windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl.001"));
		DeleteFile(TEXT("c:\\Windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl.002"));
		DeleteFile(TEXT("c:\\Windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl.003"));
		DeleteFile(TEXT("c:\\Windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl.004"));
		DeleteFile(TEXT("c:\\Windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl.005"));
		DeleteFile(TEXT("c:\\Windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl.006"));
		DeleteFile(TEXT("c:\\Windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl.007"));
		DeleteFile(TEXT("c:\\Windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl.008"));
	}
}