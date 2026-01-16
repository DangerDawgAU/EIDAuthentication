#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include "EIDConfigurationWizardElevated.h"

HINSTANCE g_hinst;

// Secure DLL loading function - prevents DLL hijacking attacks
static HMODULE LoadSystemLibrarySafe(LPCTSTR szDllName)
{
	TCHAR szFullPath[MAX_PATH];
	UINT uLen;

	if (szDllName == NULL || _tcschr(szDllName, TEXT('\\')) != NULL || _tcschr(szDllName, TEXT('/')) != NULL)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
	}

	uLen = GetSystemDirectory(szFullPath, ARRAYSIZE(szFullPath));
	if (uLen == 0 || uLen >= ARRAYSIZE(szFullPath))
	{
		SetLastError(ERROR_BUFFER_OVERFLOW);
		return NULL;
	}

	if (FAILED(StringCchCat(szFullPath, ARRAYSIZE(szFullPath), TEXT("\\"))) ||
		FAILED(StringCchCat(szFullPath, ARRAYSIZE(szFullPath), szDllName)))
	{
		SetLastError(ERROR_BUFFER_OVERFLOW);
		return NULL;
	}

	return LoadLibrary(szFullPath);
}
INT_PTR CALLBACK	WndProc_RemovePolicy(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK	WndProc_ForcePolicy(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	g_hinst = hInstance;
	int iNumArgs;
	LPWSTR *pszCommandLine =  CommandLineToArgvW(lpCmdLine,&iNumArgs);
	if (_tcscmp(pszCommandLine[0],TEXT("DIALOGREMOVEPOLICY")) == 0)
	{
		DialogBox(g_hinst, MAKEINTRESOURCE(IDD_DIALOGREMOVEPOLICY), NULL, WndProc_RemovePolicy);
		return 0;
	} 
	else //if (_tcscmp(pszCommandLine[0],TEXT("DIALOGFORCEPOLICY")) == 0)
	{
		DialogBox(g_hinst, MAKEINTRESOURCE(IDD_DIALOGFORCEPOLICY), NULL, WndProc_ForcePolicy);
		return 0;
	}
	return 0;
}

VOID CenterWindow(HWND hWnd)
{
	RECT rc;
    if (!GetWindowRect(hWnd, &rc)) return;

    const int width  = rc.right  - rc.left;
    const int height = rc.bottom - rc.top;

    MoveWindow(hWnd,
        (GetSystemMetrics(SM_CXSCREEN) - width)  / 2,
        (GetSystemMetrics(SM_CYSCREEN) - height) / 2,
        width, height, true);
}


VOID SetIcon(HWND hWnd)
{
	HMODULE hDll = LoadSystemLibrarySafe(TEXT("imageres.dll"));
	if (hDll)
	{
		HANDLE hbicon = LoadImage(hDll, MAKEINTRESOURCE(58),IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
		if (hbicon)
			SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM) hbicon);
		hbicon = LoadImage(hDll, MAKEINTRESOURCE(58),IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
		if (hbicon)
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) hbicon);
		FreeLibrary(hDll);
	}
}