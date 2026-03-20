#include <Windows.h>
#include <CommCtrl.h>

#include "../EIDCardLibrary/CContainer.h"
#include "../EIDCardLibrary/CContainerHolderFactory.h"
#include "../EIDCardLibrary/EIDCardLibrary.h"
// OnlineDatabase.h removed - internet reporting functionality disabled
#include "CContainerHolder.h"
#include "global.h"
#include "EIDConfigurationWizard.h"

// from previous step
// credentials
extern CContainerHolderFactory<CContainerHolderTest> *pCredentialList;  // NOSONAR - RUNTIME-01: Credential list
extern DWORD dwCurrentCredential;  // NOSONAR - RUNTIME-01: Selected index
extern DWORD dwWizardError;  // NOSONAR - RUNTIME-01: Error code

void SetErrorMessage(HWND hWnd)
{
	LPTSTR Error;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr,dwWizardError,0,(LPTSTR)&Error,0,nullptr);
	SetWindowText(GetDlgItem(hWnd,IDC_WIZARDERROR),Error);
	LocalFree(Error);
}

INT_PTR CALLBACK	WndProc_07TESTRESULTNOTOK(HWND hWnd, UINT message, [[maybe_unused]] WPARAM wParam, LPARAM lParam)
{
	auto pnmh = (LPNMHDR)lParam;  // NOSONAR - Cast required for Windows message handling
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
					// STM_SETIMAGE does NOT copy the icon - the control takes ownership.
					// Destroy the previous icon (if any) returned by STM_SETIMAGE, but NOT the new one.
					HICON hPrevIcon = (HICON)SendMessage(GetDlgItem(hWnd,IDC_07SHIELD),STM_SETIMAGE,IMAGE_ICON, (LPARAM) hIcon);
					if (hPrevIcon)
						DestroyIcon(hPrevIcon);
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
					// Clean up the icon handle before closing (control owns it after STM_SETIMAGE)
					{
						HICON hIcon = (HICON)SendDlgItemMessage(hWnd, IDC_07SHIELD, STM_SETIMAGE, IMAGE_ICON, 0);
						if (hIcon)
							DestroyIcon(hIcon);
					}
					if (pCredentialList)
					{
						delete pCredentialList;
						pCredentialList = nullptr;
					}
					break;

				case PSN_RESET:
					// Clean up the icon handle when wizard is cancelled
					{
						HICON hIcon = (HICON)SendDlgItemMessage(hWnd, IDC_07SHIELD, STM_SETIMAGE, IMAGE_ICON, 0);
						if (hIcon)
							DestroyIcon(hIcon);
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
