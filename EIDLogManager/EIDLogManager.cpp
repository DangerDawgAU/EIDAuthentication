// File: EIDLogManager/EIDLogManager.cpp
// EID Log Manager - Main dialog implementation for trace log management

#include "stdafx.h"
#include "EIDLogManager.h"
#include <tchar.h>
#include <ShObjIdl.h>
#include <ShlObj.h>
#include "../EIDCardLibrary/Registration.h"
#include "../EIDCardLibrary/Tracing.h"
#include "../EIDCardLibrary/TraceExport.h"
#include "../EIDCardLibrary/CSVConfig.h"

// Trace level constants
constexpr UCHAR TRACE_LEVEL_ERROR    = 2;
constexpr UCHAR TRACE_LEVEL_WARNING  = 3;
constexpr UCHAR TRACE_LEVEL_INFO     = 4;
constexpr UCHAR TRACE_LEVEL_VERBOSE  = 5;

// Forward declarations for new functions
void LoadSettings(HWND hDlg);
void SaveSettings(HWND hDlg);
void UpdateStatus(HWND hDlg);
void BrowseForLogPath(HWND hDlg);

// CSV configuration functions
void LoadCSVSettings(HWND hDlg);
void SaveCSVSettings(HWND hDlg);
void BrowseForCSVPath(HWND hDlg);
void ApplyColumnPreset(HWND hDlg, EID_CSV_COLUMN preset);
void EnableDisableCSVControls(HWND hDlg, BOOL fEnabled);

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

// Global variables
HINSTANCE hInst;  // NOSONAR - RUNTIME-01: HINSTANCE set by Windows at WinMain entry

INT_PTR CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	DialogBox(hInst, MAKEINTRESOURCE(IDD_EIDLOGMANAGER_DIALOG), nullptr, WndProc);
	return 0;
}

void SaveLog(HWND hDlg);
void DeleteLog(HWND hDlg);

void ShowHideLogButtons(HWND hDlg)
{
	if (IsLoggingEnabled())
	{
		EnableWindow(GetDlgItem(hDlg, IDC_ENABLELOG), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_DISABLELOG), TRUE);
	}
	else
	{
		EnableWindow(GetDlgItem(hDlg, IDC_ENABLELOG), TRUE);
		EnableWindow(GetDlgItem(hDlg, IDC_DISABLELOG), FALSE);
	}
}

// Dialog procedure for the main EID Log Manager window
INT_PTR CALLBACK WndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (message)
	{
		case WM_CLOSE:
			EndDialog(hDlg, 0);
			break;

		case WM_INITDIALOG:
			LoadSettings(hDlg);
			LoadCSVSettings(hDlg);
			UpdateStatus(hDlg);
			return (INT_PTR)TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_LOG_BROWSE_PATH:
					BrowseForLogPath(hDlg);
					return (INT_PTR)TRUE;

				case IDC_LOG_APPLY:
					SaveSettings(hDlg);
					SaveCSVSettings(hDlg);
					MessageBox(hDlg, L"Settings saved. Enable tracing to apply changes.", L"Settings Saved", MB_OK | MB_ICONINFORMATION);
					UpdateStatus(hDlg);
					return (INT_PTR)TRUE;

				case IDC_ENABLELOG:
					SaveSettings(hDlg);
					SaveCSVSettings(hDlg);
					if (!EnableLogging())
					{
						MessageBoxWin32Ex(GetLastError(), hDlg);
					}
					ShowHideLogButtons(hDlg);
					UpdateStatus(hDlg);
					break;

				case IDC_DISABLELOG:
					if (!DisableLogging())
					{
						MessageBoxWin32Ex(GetLastError(), hDlg);
					}
					ShowHideLogButtons(hDlg);
					UpdateStatus(hDlg);
					break;

				case IDC_SAVELOG:
					SaveLog(hDlg);
					break;

				case IDC_CLEARLOG:
					DeleteLog(hDlg);
					break;

				// CSV control handlers
				case IDC_CSV_ENABLE_CHECK:
					EnableDisableCSVControls(hDlg, IsDlgButtonChecked(hDlg, IDC_CSV_ENABLE_CHECK) == BST_CHECKED);
					return (INT_PTR)TRUE;

				case IDC_CSV_BROWSE_PATH:
					BrowseForCSVPath(hDlg);
					return (INT_PTR)TRUE;

				case IDC_CSV_COLUMN_PRESET_STANDARD:
					ApplyColumnPreset(hDlg, EID_CSV_PRESETS::STANDARD);
					return (INT_PTR)TRUE;

				case IDC_CSV_COLUMN_PRESET_EXTENDED:
					ApplyColumnPreset(hDlg, EID_CSV_PRESETS::EXTENDED);
					return (INT_PTR)TRUE;

				case IDC_CSV_COLUMN_PRESET_MINIMAL:
					ApplyColumnPreset(hDlg, EID_CSV_PRESETS::MINIMAL);
					return (INT_PTR)TRUE;
			}
			break;
	}
	return (INT_PTR)FALSE;
}

HANDLE hFile = nullptr;  // NOSONAR - RUNTIME-01: File handle, opened at runtime

void SaveLog(HWND hDlg)
{
	IFileSaveDialog *pSaveDialog = nullptr;

	LPOLESTR pszPath = nullptr;
	__try
	{
		HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog,
			nullptr,
			CLSCTX_INPROC_SERVER_LOCAL,
			IID_PPV_ARGS(&pSaveDialog));  // NOSONAR - EXPLICIT-TYPE-03: HRESULT visible for security audit

		if (!SUCCEEDED(hr))
		{
			return;
		}

		pSaveDialog->SetDefaultExtension(TEXT("txt"));
		pSaveDialog->SetFileName(TEXT("Report.txt"));
		pSaveDialog->SetOptions(FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT | FOS_DONTADDTORECENT);

		// Show the dialog
		hr = pSaveDialog->Show(hDlg);

		if (SUCCEEDED(hr))
		{
			IShellItem *ppsi = nullptr;
			// This will fail if Cancel has been clicked:
			hr = pSaveDialog->GetResult(&ppsi);

			if (SUCCEEDED(hr))
			{
				// Extract the path:
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
		{
			CoTaskMemFree(pszPath);
		}
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
		MessageBox(hDlg, TEXT("Tracing must be disabled"), TEXT("Error"), 0);
	}
	else
	{
		// Get the configured log file path and base name
		WCHAR szLogPath[MAX_PATH];
		DWORD dwLevel, dwMaxSize, dwFileCounter;
		BOOL fAutoStart;

		GetTraceConfig(&dwLevel, szLogPath, MAX_PATH, &dwMaxSize, &dwFileCounter, &fAutoStart);

		// Delete the main log file and rotated files
		DeleteFile(szLogPath);

		// Delete rotated files
		for (int i = 1; i <= 100; i++)
		{
			WCHAR szRotatedPath[MAX_PATH];
			swprintf_s(szRotatedPath, MAX_PATH, L"%s.%03d", szLogPath, i);
			DeleteFile(szRotatedPath);
		}
	}
}

void LoadSettings(HWND hDlg)
{
	DWORD dwLevel, dwMaxSize, dwFileCount;
	WCHAR szPath[MAX_PATH];
	BOOL fAutoStart;

	// Read from registry
	GetTraceConfig(&dwLevel, szPath, MAX_PATH, &dwMaxSize, &dwFileCount, &fAutoStart);

	// Populate UI controls
	SetDlgItemText(hDlg, IDC_LOG_FILE_PATH, szPath);
	SetDlgItemInt(hDlg, IDC_LOG_MAX_SIZE, dwMaxSize, FALSE);
	SetDlgItemInt(hDlg, IDC_LOG_FILE_COUNT, dwFileCount, FALSE);
	CheckDlgButton(hDlg, IDC_LOG_AUTO_START, fAutoStart ? BST_CHECKED : BST_UNCHECKED);

	// Set radio button based on level
	// Map level to radio button: Error=206, Warning=207, Info=208, Verbose=209
	int nRadioButtonId = IDC_LOG_LEVEL_INFO;  // Default to Info
	switch (dwLevel)
	{
		case TRACE_LEVEL_ERROR:
			nRadioButtonId = IDC_LOG_LEVEL_ERROR;
			break;
		case TRACE_LEVEL_WARNING:
			nRadioButtonId = IDC_LOG_LEVEL_WARNING;
			break;
		case TRACE_LEVEL_VERBOSE:
			nRadioButtonId = IDC_LOG_LEVEL_VERBOSE;
			break;
		case TRACE_LEVEL_INFO:
		default:
			nRadioButtonId = IDC_LOG_LEVEL_INFO;
			break;
	}
	CheckDlgButton(hDlg, nRadioButtonId, BST_CHECKED);
}

void SaveSettings(HWND hDlg)
{
	WCHAR szPath[MAX_PATH];
	GetDlgItemText(hDlg, IDC_LOG_FILE_PATH, szPath, MAX_PATH);

	BOOL bTranslated = FALSE;
	UINT nMaxSize = GetDlgItemInt(hDlg, IDC_LOG_MAX_SIZE, &bTranslated, FALSE);
	if (!bTranslated || nMaxSize < 1) nMaxSize = 64;

	UINT nFileCount = GetDlgItemInt(hDlg, IDC_LOG_FILE_COUNT, &bTranslated, FALSE);
	if (!bTranslated || nFileCount < 1) nFileCount = 5;

	BOOL fAutoStart = (IsDlgButtonChecked(hDlg, IDC_LOG_AUTO_START) == BST_CHECKED);

	DWORD dwLevel = TRACE_LEVEL_INFO;
	if (IsDlgButtonChecked(hDlg, IDC_LOG_LEVEL_ERROR) == BST_CHECKED)
		dwLevel = TRACE_LEVEL_ERROR;
	else if (IsDlgButtonChecked(hDlg, IDC_LOG_LEVEL_WARNING) == BST_CHECKED)
		dwLevel = TRACE_LEVEL_WARNING;
	else if (IsDlgButtonChecked(hDlg, IDC_LOG_LEVEL_VERBOSE) == BST_CHECKED)
		dwLevel = TRACE_LEVEL_VERBOSE;
	else
		dwLevel = TRACE_LEVEL_INFO;

	SetTraceConfig(dwLevel, szPath, (DWORD)nMaxSize, (DWORD)nFileCount, fAutoStart);
}

void UpdateStatus(HWND hDlg)
{
	if (IsLoggingEnabled())
	{
		SetDlgItemText(hDlg, IDC_LOG_STATUS_TEXT, L"Logging is ACTIVE");
	}
	else
	{
		SetDlgItemText(hDlg, IDC_LOG_STATUS_TEXT, L"Logging is STOPPED");
	}
}

void BrowseForLogPath(HWND hDlg)
{
	IFileSaveDialog *pSaveDialog = nullptr;

	__try
	{
		HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog,
			nullptr,
			CLSCTX_INPROC_SERVER_LOCAL,
			IID_PPV_ARGS(&pSaveDialog));

		if (!SUCCEEDED(hr))
		{
			__leave;
		}

		pSaveDialog->SetDefaultExtension(TEXT("etl"));
		pSaveDialog->SetFileName(TEXT("EIDCredentialProvider.etl"));
		pSaveDialog->SetOptions(FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT | FOS_DONTADDTORECENT);

		// Set the title for the dialog
		pSaveDialog->SetTitle(L"Select Log File Location");

		hr = pSaveDialog->Show(hDlg);

		if (SUCCEEDED(hr))
		{
			IShellItem *ppsi = nullptr;
			hr = pSaveDialog->GetResult(&ppsi);

			if (SUCCEEDED(hr))
			{
				LPOLESTR pszPath = nullptr;
				hr = ppsi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);

				if (SUCCEEDED(hr))
				{
					SetDlgItemText(hDlg, IDC_LOG_FILE_PATH, pszPath);
					CoTaskMemFree(pszPath);
				}
				ppsi->Release();
			}
		}

		pSaveDialog->Release();
	}
	__finally
	{
		// Cleanup handled by __finally block
	}
}

// ================================================================
// CSV Configuration Functions
// ================================================================

void LoadCSVSettings(HWND hDlg)
{
	EID_CSV_CONFIG config;
	HRESULT hr = EID_CSV_LoadConfig(config);

	if (FAILED(hr))
	{
		// Use defaults
		config = EID_CSV_CONFIG();
		wcscpy_s(config.szLogPath, EID_CSV_DEFAULT_LOG_PATH);
	}

	// Populate controls
	CheckDlgButton(hDlg, IDC_CSV_ENABLE_CHECK,
		config.fEnabled ? BST_CHECKED : BST_UNCHECKED);

	SetDlgItemText(hDlg, IDC_CSV_LOG_PATH, config.szLogPath);
	SetDlgItemInt(hDlg, IDC_CSV_MAX_SIZE, config.dwMaxFileSizeMB, FALSE);
	SetDlgItemInt(hDlg, IDC_CSV_FILE_COUNT, config.dwFileCount, FALSE);

	// Set column checkboxes based on preset
	EID_CSV_COLUMN cols = config.dwColumns;
	CheckDlgButton(hDlg, IDC_CSV_COLUMN_CHECK_TIMESTAMP,
		(cols & EID_CSV_COLUMN::TIMESTAMP) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CSV_COLUMN_CHECK_EVENT_ID,
		(cols & EID_CSV_COLUMN::EVENT_ID) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CSV_COLUMN_CHECK_CATEGORY,
		(cols & EID_CSV_COLUMN::CATEGORY) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CSV_COLUMN_CHECK_SEVERITY,
		(cols & EID_CSV_COLUMN::SEVERITY) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CSV_COLUMN_CHECK_OUTCOME,
		(cols & EID_CSV_COLUMN::OUTCOME) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CSV_COLUMN_CHECK_USERNAME,
		(cols & EID_CSV_COLUMN::USERNAME) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CSV_COLUMN_CHECK_ACTION,
		(cols & EID_CSV_COLUMN::ACTION) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CSV_COLUMN_CHECK_MESSAGE,
		(cols & EID_CSV_COLUMN::MESSAGE) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CSV_COLUMN_CHECK_DOMAIN,
		(cols & EID_CSV_COLUMN::DOMAIN) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CSV_COLUMN_CHECK_SOURCE_IP,
		(cols & EID_CSV_COLUMN::CLIENT_IP) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CSV_COLUMN_CHECK_PROCESS_ID,
		(cols & EID_CSV_COLUMN::PROCESS_ID) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CSV_COLUMN_CHECK_THREAD_ID,
		(cols & EID_CSV_COLUMN::THREAD_ID) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CSV_COLUMN_CHECK_SESSION_ID,
		(cols & EID_CSV_COLUMN::LOGIN_SESSION) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CSV_COLUMN_CHECK_RESOURCE,
		(cols & EID_CSV_COLUMN::TARGET) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CSV_COLUMN_CHECK_REASON,
		(cols & EID_CSV_COLUMN::REASON) ? BST_CHECKED : BST_UNCHECKED);

	// Set category filter checkboxes
	CheckDlgButton(hDlg, IDC_CSV_FILTER_AUTH,
		IsCategoryEnabled(config.dwCategoryFilter, EID_EVENT_CATEGORY::AUTHENTICATION) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CSV_FILTER_AUTHZ,
		IsCategoryEnabled(config.dwCategoryFilter, EID_EVENT_CATEGORY::AUTHORIZATION) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CSV_FILTER_SESSION,
		IsCategoryEnabled(config.dwCategoryFilter, EID_EVENT_CATEGORY::SESSION) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CSV_FILTER_CERT,
		IsCategoryEnabled(config.dwCategoryFilter, EID_EVENT_CATEGORY::CERTIFICATE) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CSV_FILTER_SMARTCARD,
		IsCategoryEnabled(config.dwCategoryFilter, EID_EVENT_CATEGORY::SMARTCARD) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CSV_FILTER_CONFIG,
		IsCategoryEnabled(config.dwCategoryFilter, EID_EVENT_CATEGORY::CONFIG) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hDlg, IDC_CSV_FILTER_AUDIT,
		IsCategoryEnabled(config.dwCategoryFilter, EID_EVENT_CATEGORY::AUDIT) ? BST_CHECKED : BST_UNCHECKED);

	CheckDlgButton(hDlg, IDC_CSV_VERBOSE_CHECK,
		config.fVerboseEvents ? BST_CHECKED : BST_UNCHECKED);

	// Enable/disable controls based on enabled state
	EnableDisableCSVControls(hDlg, config.fEnabled);
}

void SaveCSVSettings(HWND hDlg)
{
	EID_CSV_CONFIG config;

	config.fEnabled = IsDlgButtonChecked(hDlg, IDC_CSV_ENABLE_CHECK) == BST_CHECKED;
	GetDlgItemText(hDlg, IDC_CSV_LOG_PATH, config.szLogPath, MAX_PATH);

	BOOL bTranslated = FALSE;
	UINT nMaxSize = GetDlgItemInt(hDlg, IDC_CSV_MAX_SIZE, &bTranslated, FALSE);
	if (!bTranslated || nMaxSize < 1) nMaxSize = 64;
	config.dwMaxFileSizeMB = static_cast<DWORD>(nMaxSize);

	UINT nFileCount = GetDlgItemInt(hDlg, IDC_CSV_FILE_COUNT, &bTranslated, FALSE);
	if (!bTranslated || nFileCount < 1) nFileCount = 5;
	config.dwFileCount = static_cast<DWORD>(nFileCount);

	// Build column bitmask
	config.dwColumns = EID_CSV_COLUMN::NONE;
	if (IsDlgButtonChecked(hDlg, IDC_CSV_COLUMN_CHECK_TIMESTAMP) == BST_CHECKED)
		config.dwColumns = static_cast<EID_CSV_COLUMN>(config.dwColumns | EID_CSV_COLUMN::TIMESTAMP);
	if (IsDlgButtonChecked(hDlg, IDC_CSV_COLUMN_CHECK_EVENT_ID) == BST_CHECKED)
		config.dwColumns = static_cast<EID_CSV_COLUMN>(config.dwColumns | EID_CSV_COLUMN::EVENT_ID);
	if (IsDlgButtonChecked(hDlg, IDC_CSV_COLUMN_CHECK_CATEGORY) == BST_CHECKED)
		config.dwColumns = static_cast<EID_CSV_COLUMN>(config.dwColumns | EID_CSV_COLUMN::CATEGORY);
	if (IsDlgButtonChecked(hDlg, IDC_CSV_COLUMN_CHECK_SEVERITY) == BST_CHECKED)
		config.dwColumns = static_cast<EID_CSV_COLUMN>(config.dwColumns | EID_CSV_COLUMN::SEVERITY);
	if (IsDlgButtonChecked(hDlg, IDC_CSV_COLUMN_CHECK_OUTCOME) == BST_CHECKED)
		config.dwColumns = static_cast<EID_CSV_COLUMN>(config.dwColumns | EID_CSV_COLUMN::OUTCOME);
	if (IsDlgButtonChecked(hDlg, IDC_CSV_COLUMN_CHECK_USERNAME) == BST_CHECKED)
		config.dwColumns = static_cast<EID_CSV_COLUMN>(config.dwColumns | EID_CSV_COLUMN::USERNAME);
	if (IsDlgButtonChecked(hDlg, IDC_CSV_COLUMN_CHECK_ACTION) == BST_CHECKED)
		config.dwColumns = static_cast<EID_CSV_COLUMN>(config.dwColumns | EID_CSV_COLUMN::ACTION);
	if (IsDlgButtonChecked(hDlg, IDC_CSV_COLUMN_CHECK_MESSAGE) == BST_CHECKED)
		config.dwColumns = static_cast<EID_CSV_COLUMN>(config.dwColumns | EID_CSV_COLUMN::MESSAGE);
	if (IsDlgButtonChecked(hDlg, IDC_CSV_COLUMN_CHECK_DOMAIN) == BST_CHECKED)
		config.dwColumns = static_cast<EID_CSV_COLUMN>(config.dwColumns | EID_CSV_COLUMN::DOMAIN);
	if (IsDlgButtonChecked(hDlg, IDC_CSV_COLUMN_CHECK_SOURCE_IP) == BST_CHECKED)
		config.dwColumns = static_cast<EID_CSV_COLUMN>(config.dwColumns | EID_CSV_COLUMN::SOURCE_IP);
	if (IsDlgButtonChecked(hDlg, IDC_CSV_COLUMN_CHECK_PROCESS_ID) == BST_CHECKED)
		config.dwColumns = static_cast<EID_CSV_COLUMN>(config.dwColumns | EID_CSV_COLUMN::PROCESS_ID);
	if (IsDlgButtonChecked(hDlg, IDC_CSV_COLUMN_CHECK_THREAD_ID) == BST_CHECKED)
		config.dwColumns = static_cast<EID_CSV_COLUMN>(config.dwColumns | EID_CSV_COLUMN::THREAD_ID);
	if (IsDlgButtonChecked(hDlg, IDC_CSV_COLUMN_CHECK_SESSION_ID) == BST_CHECKED)
		config.dwColumns = static_cast<EID_CSV_COLUMN>(config.dwColumns | EID_CSV_COLUMN::SESSION_ID);
	if (IsDlgButtonChecked(hDlg, IDC_CSV_COLUMN_CHECK_RESOURCE) == BST_CHECKED)
		config.dwColumns = static_cast<EID_CSV_COLUMN>(config.dwColumns | EID_CSV_COLUMN::RESOURCE);
	if (IsDlgButtonChecked(hDlg, IDC_CSV_COLUMN_CHECK_REASON) == BST_CHECKED)
		config.dwColumns = static_cast<EID_CSV_COLUMN>(config.dwColumns | EID_CSV_COLUMN::REASON);

	// Build category filter bitmask
	config.dwCategoryFilter = 0;
	if (IsDlgButtonChecked(hDlg, IDC_CSV_FILTER_AUTH) == BST_CHECKED)
		config.dwCategoryFilter = EnableCategory(config.dwCategoryFilter, EID_EVENT_CATEGORY::AUTHENTICATION);
	if (IsDlgButtonChecked(hDlg, IDC_CSV_FILTER_AUTHZ) == BST_CHECKED)
		config.dwCategoryFilter = EnableCategory(config.dwCategoryFilter, EID_EVENT_CATEGORY::AUTHORIZATION);
	if (IsDlgButtonChecked(hDlg, IDC_CSV_FILTER_SESSION) == BST_CHECKED)
		config.dwCategoryFilter = EnableCategory(config.dwCategoryFilter, EID_EVENT_CATEGORY::SESSION);
	if (IsDlgButtonChecked(hDlg, IDC_CSV_FILTER_CERT) == BST_CHECKED)
		config.dwCategoryFilter = EnableCategory(config.dwCategoryFilter, EID_EVENT_CATEGORY::CERTIFICATE);
	if (IsDlgButtonChecked(hDlg, IDC_CSV_FILTER_SMARTCARD) == BST_CHECKED)
		config.dwCategoryFilter = EnableCategory(config.dwCategoryFilter, EID_EVENT_CATEGORY::SMARTCARD);
	if (IsDlgButtonChecked(hDlg, IDC_CSV_FILTER_CONFIG) == BST_CHECKED)
		config.dwCategoryFilter = EnableCategory(config.dwCategoryFilter, EID_EVENT_CATEGORY::CONFIG);
	if (IsDlgButtonChecked(hDlg, IDC_CSV_FILTER_AUDIT) == BST_CHECKED)
		config.dwCategoryFilter = EnableCategory(config.dwCategoryFilter, EID_EVENT_CATEGORY::AUDIT);

	config.fVerboseEvents = IsDlgButtonChecked(hDlg, IDC_CSV_VERBOSE_CHECK) == BST_CHECKED;

	// Save configuration
	HRESULT hr = EID_CSV_SaveConfig(config);
	if (FAILED(hr))
	{
		MessageBoxWin32Ex(hr, hDlg);
	}
}

void BrowseForCSVPath(HWND hDlg)
{
	IFileSaveDialog *pSaveDialog = nullptr;

	__try
	{
		HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog,
			nullptr,
			CLSCTX_INPROC_SERVER_LOCAL,
			IID_PPV_ARGS(&pSaveDialog));

		if (!SUCCEEDED(hr))
		{
			__leave;
		}

		pSaveDialog->SetDefaultExtension(TEXT("csv"));
		pSaveDialog->SetFileName(TEXT("events.csv"));
		pSaveDialog->SetOptions(FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_OVERWRITEPROMPT | FOS_DONTADDTORECENT);

		// Set the title for the dialog
		pSaveDialog->SetTitle(L"Select CSV Log File Location");

		hr = pSaveDialog->Show(hDlg);

		if (SUCCEEDED(hr))
		{
			IShellItem *ppsi = nullptr;
			hr = pSaveDialog->GetResult(&ppsi);

			if (SUCCEEDED(hr))
			{
				LPOLESTR pszPath = nullptr;
				hr = ppsi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);

				if (SUCCEEDED(hr))
				{
					SetDlgItemText(hDlg, IDC_CSV_LOG_PATH, pszPath);
					CoTaskMemFree(pszPath);
				}
				ppsi->Release();
			}
		}

		pSaveDialog->Release();
	}
	__finally
	{
		// Cleanup handled by __finally block
	}
}

void ApplyColumnPreset(HWND hDlg, EID_CSV_COLUMN preset)
{
	// Helper lambda to set checkbox state
	auto SetCheck = [hDlg](int id, bool checked)
	{
		CheckDlgButton(hDlg, id, checked ? BST_CHECKED : BST_UNCHECKED);
	};

	SetCheck(IDC_CSV_COLUMN_CHECK_TIMESTAMP, (preset & EID_CSV_COLUMN::TIMESTAMP) != 0);
	SetCheck(IDC_CSV_COLUMN_CHECK_EVENT_ID, (preset & EID_CSV_COLUMN::EVENT_ID) != 0);
	SetCheck(IDC_CSV_COLUMN_CHECK_CATEGORY, (preset & EID_CSV_COLUMN::CATEGORY) != 0);
	SetCheck(IDC_CSV_COLUMN_CHECK_SEVERITY, (preset & EID_CSV_COLUMN::SEVERITY) != 0);
	SetCheck(IDC_CSV_COLUMN_CHECK_OUTCOME, (preset & EID_CSV_COLUMN::OUTCOME) != 0);
	SetCheck(IDC_CSV_COLUMN_CHECK_USERNAME, (preset & EID_CSV_COLUMN::USERNAME) != 0);
	SetCheck(IDC_CSV_COLUMN_CHECK_ACTION, (preset & EID_CSV_COLUMN::ACTION) != 0);
	SetCheck(IDC_CSV_COLUMN_CHECK_MESSAGE, (preset & EID_CSV_COLUMN::MESSAGE) != 0);
	SetCheck(IDC_CSV_COLUMN_CHECK_DOMAIN, (preset & EID_CSV_COLUMN::DOMAIN) != 0);
	SetCheck(IDC_CSV_COLUMN_CHECK_SOURCE_IP, (preset & EID_CSV_COLUMN::SOURCE_IP) != 0);
	SetCheck(IDC_CSV_COLUMN_CHECK_PROCESS_ID, (preset & EID_CSV_COLUMN::PROCESS_ID) != 0);
	SetCheck(IDC_CSV_COLUMN_CHECK_THREAD_ID, (preset & EID_CSV_COLUMN::THREAD_ID) != 0);
	SetCheck(IDC_CSV_COLUMN_CHECK_SESSION_ID, (preset & EID_CSV_COLUMN::SESSION_ID) != 0);
	SetCheck(IDC_CSV_COLUMN_CHECK_RESOURCE, (preset & EID_CSV_COLUMN::RESOURCE) != 0);
	SetCheck(IDC_CSV_COLUMN_CHECK_REASON, (preset & EID_CSV_COLUMN::REASON) != 0);
}

void EnableDisableCSVControls(HWND hDlg, BOOL fEnabled)
{
	// Enable/disable CSV controls based on whether CSV logging is enabled
	EnableWindow(GetDlgItem(hDlg, IDC_CSV_LOG_PATH), fEnabled);
	EnableWindow(GetDlgItem(hDlg, IDC_CSV_BROWSE_PATH), fEnabled);
	EnableWindow(GetDlgItem(hDlg, IDC_CSV_MAX_SIZE), fEnabled);
	EnableWindow(GetDlgItem(hDlg, IDC_CSV_FILE_COUNT), fEnabled);
	EnableWindow(GetDlgItem(hDlg, IDC_CSV_COLUMNS_GROUP), fEnabled);
	EnableWindow(GetDlgItem(hDlg, IDC_CSV_COLUMN_PRESET_STANDARD), fEnabled);
	EnableWindow(GetDlgItem(hDlg, IDC_CSV_COLUMN_PRESET_EXTENDED), fEnabled);
	EnableWindow(GetDlgItem(hDlg, IDC_CSV_COLUMN_PRESET_MINIMAL), fEnabled);
	EnableWindow(GetDlgItem(hDlg, IDC_CSV_FILTERS_GROUP), fEnabled);
}
