#include <windows.h>
#include <tchar.h>
#include "../EIDCardLibrary/GPO.h"
#include "../EIDCardLibrary/EIDCardLibrary.h"
#include "EIDConfigurationWizardElevated.h"

INT_PTR CALLBACK WndProc_ForcePolicy(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId;
	int wmEvent;
	switch(message)
	{
	case WM_INITDIALOG:
		CenterWindow(hWnd);
		if (GetPolicyValue(scforceoption) > 0)
		{
			CheckRadioButton(hWnd, IDC_FORCEDISABLE, IDC_FORCEENABLE, IDC_FORCEENABLE);
		}
		else
		{
			CheckRadioButton(hWnd, IDC_FORCEDISABLE, IDC_FORCEENABLE, IDC_FORCEDISABLE);
		}
		SetIcon(hWnd);
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch(wmId)
		{
		case IDOK:
			SetPolicyValue(scforceoption,IsDlgButtonChecked(hWnd, IDC_FORCEENABLE));
			EndDialog(hWnd, 0);
			return TRUE;
		case IDCANCEL:
			EndDialog(hWnd, 0);
			return TRUE;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return FALSE;
}