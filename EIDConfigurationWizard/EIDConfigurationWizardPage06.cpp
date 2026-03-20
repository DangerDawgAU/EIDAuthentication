#include <Windows.h>
#include <tchar.h>
#include "../EIDCardLibrary/CContainer.h"
#include "../EIDCardLibrary/CContainerHolderFactory.h"
#include "../EIDCardLibrary/EIDCardLibrary.h"
#include "CContainerHolder.h"
#include "global.h"
#include "EIDConfigurationWizard.h"
#include "../EIDCardLibrary/GPO.h"
// OnlineDatabase.h removed - internet reporting functionality disabled
// from previous step
// credentials
extern CContainerHolderFactory<CContainerHolderTest> *pCredentialList;  // NOSONAR - RUNTIME-01: Extern pointer to credential list


INT_PTR CALLBACK	WndProc_06TESTRESULTOK(HWND hWnd, UINT message, [[maybe_unused]] WPARAM wParam, [[maybe_unused]] LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:

		{
			HMODULE hDll = EIDLoadSystemLibrary(TEXT("imageres.dll"));
			if (hDll)
			{
				HICON hIcon = LoadIcon(hDll, MAKEINTRESOURCE(106));
				// STM_SETIMAGE does NOT copy the icon - the control takes ownership.
				// Destroy the previous icon (if any) returned by STM_SETIMAGE, but NOT the new one.
				HICON hPrevIcon = (HICON)SendMessage(GetDlgItem(hWnd,IDC_06SHIELD),STM_SETIMAGE,IMAGE_ICON, (LPARAM) hIcon);
				if (hPrevIcon)
					DestroyIcon(hPrevIcon);
				FreeLibrary(hDll);
			}
		}
		break;
	// WM_COMMAND handler removed - internet reporting button (IDC_06UPDATEDATABASE) disabled
	case WM_NOTIFY :
		{
			auto pnmh = (LPNMHDR)lParam;  // NOSONAR - Cast required for Windows message handling
			switch(pnmh->code)
			{
			case NM_CLICK:
			case NM_RETURN:
				{
					// enable / disable policy
				}
				[[fallthrough]];
			case PSN_SETACTIVE:
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Activate");
				PropSheet_SetWizButtons(hWnd, PSWIZB_BACK | PSWIZB_FINISH);
				break;

			case PSN_WIZFINISH:
				// Clean up the icon handle before closing (control owns it after STM_SETIMAGE)
				{
					HICON hIcon = (HICON)SendDlgItemMessage(hWnd, IDC_06SHIELD, STM_SETIMAGE, IMAGE_ICON, 0);
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
					HICON hIcon = (HICON)SendDlgItemMessage(hWnd, IDC_06SHIELD, STM_SETIMAGE, IMAGE_ICON, 0);
					if (hIcon)
						DestroyIcon(hIcon);
				}
				break;
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
