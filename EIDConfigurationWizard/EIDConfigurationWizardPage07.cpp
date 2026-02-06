#include <windows.h>
#include <commctrl.h>

#include "../EIDCardLibrary/CContainer.h"
#include "../EIDCardLibrary/CContainerHolderFactory.h"
#include "../EIDCardLibrary/EIDCardLibrary.h"
// OnlineDatabase.h removed - internet reporting functionality disabled
#include "CContainerHolder.h"
#include "global.h"
#include "EIDConfigurationWizard.h"

// from previous step
// credentials
extern CContainerHolderFactory<CContainerHolderTest> *pCredentialList;
extern DWORD dwCurrentCredential;
extern DWORD dwWizardError;

void SetErrorMessage(HWND hWnd)
{
	LPTSTR Error;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr,dwWizardError,0,(LPTSTR)&Error,0,nullptr);
	SetWindowText(GetDlgItem(hWnd,IDC_WIZARDERROR),Error);
	LocalFree(Error);
}

INT_PTR CALLBACK	WndProc_07TESTRESULTNOTOK(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pnmh = (LPNMHDR)lParam;
	switch(message)
	{
		case WM_INITDIALOG:
			if (!IsElevated())
			{
				// Button_SetElevationRequiredState removed - internet reporting disabled
			}
			{
				HMODULE hDll = EIDLoadSystemLibrary(TEXT("imageres.dll"));
				if (hDll)
				{
					HICON hIcon = LoadIcon(hDll, MAKEINTRESOURCE(105));
					SendMessage(GetDlgItem(hWnd,IDC_07SHIELD),STM_SETIMAGE,IMAGE_ICON, (LPARAM) hIcon);
					DestroyIcon(hIcon);
					FreeLibrary(hDll);
				}
			}
			break;
		case WM_NOTIFY :
			switch(pnmh->code)
			{
				case PSN_SETACTIVE:
					EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Activate");
					PropSheet_SetWizButtons(hWnd, PSWIZB_BACK | PSWIZB_FINISH);
					SetErrorMessage(hWnd);
					break;
				case PSN_WIZBACK:
					// get to the test again (avoid test result page positive)
					PropSheet_PressButton(hWnd, PSBTN_BACK);
					break;
				case PSN_WIZFINISH:
					if (pCredentialList)
					{
						delete pCredentialList;
						pCredentialList = nullptr;
					}
					break;
				default:
					break;
			}
			break;
		// WM_COMMAND handler removed - internet reporting button (IDC_07SENDREPORT) disabled
		default:
			break;
	}
	return FALSE;
}
