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
extern CContainerHolderFactory<CContainerHolderTest> *pCredentialList;


INT_PTR CALLBACK	WndProc_06TESTRESULTOK(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_INITDIALOG:
		
		{
			HMODULE hDll = EIDLoadSystemLibrary(TEXT("imageres.dll"));
			if (hDll)
			{
				HICON hIcon = LoadIcon(hDll, MAKEINTRESOURCE(106));
				SendMessage(GetDlgItem(hWnd,IDC_06SHIELD),STM_SETIMAGE,IMAGE_ICON, (LPARAM) hIcon);
				DestroyIcon(hIcon);
				FreeLibrary(hDll);
			}
		}
		break;
	// WM_COMMAND handler removed - internet reporting button (IDC_06UPDATEDATABASE) disabled
	case WM_NOTIFY :
		{
			LPNMHDR pnmh = (LPNMHDR)lParam;
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
		}
	default:
		break;
	}
	return FALSE;
}
