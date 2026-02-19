#include <Windows.h>
#include <tchar.h>
#include <credentialprovider.h>
#include <string>

#include "EIDConfigurationWizard.h"
#include "global.h"

#include "../EIDCardLibrary/EIDCardLibrary.h"
#include "../EIDCardLibrary/Package.h"
#include "../EIDCardLibrary/Tracing.h"
#include "../EIDCardLibrary/Registration.h"
#include "../EIDCardLibrary/CertificateUtilities.h"
#include "../EIDCardLibrary/CertificateValidation.h"
#include "../EIDCardLibrary/GPO.h"
#include "../EIDCardLibrary/CommonManifest.h"
#include "../EIDCardLibrary/StringConversion.h"

INT_PTR CALLBACK	WndProc_01MAIN(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WndProc_02ENABLE(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WndProc_03NEW(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WndProc_04CHECKS(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WndProc_05PASSWORD(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WndProc_06TESTRESULTOK(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WndProc_07TESTRESULTNOTOK(HWND, UINT, WPARAM, LPARAM);

BOOL fShowNewCertificatePanel;
BOOL fGotoNewScreen = FALSE;
HINSTANCE g_hinst;
WCHAR szReader[256];  // NOSONAR - LSASS-01: C-style buffer for LSASS safety
const DWORD dwReaderSize = ARRAYSIZE(szReader);
WCHAR szCard[256];  // NOSONAR - LSASS-01: C-style buffer for LSASS safety
const DWORD dwCardSize = ARRAYSIZE(szCard);
WCHAR szUserName[256];  // NOSONAR - LSASS-01: C-style buffer for LSASS safety
const DWORD dwUserNameSize = ARRAYSIZE(szUserName);
WCHAR szPassword[256];  // NOSONAR - LSASS-01: C-style buffer for LSASS safety
const DWORD dwPasswordSize = ARRAYSIZE(szPassword);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// check that the authentication package is loaded
	if (!IsEIDPackageAvailable())
	{
		std::wstring szMessage = EID::LoadStringW(g_hinst, IDS_EIDNOTAVAILABLE);
		MessageBox(nullptr, szMessage.c_str(), L"Error", MB_ICONERROR);
		return -1;
	}
	// check that the user is not connected to a domain
	if (IsCurrentUserBelongToADomain())
	{
		std::wstring szMessage = EID::LoadStringW(g_hinst, IDS_NODOMAINACCOUNT);
		MessageBox(nullptr, szMessage.c_str(), L"Error", MB_ICONERROR);
		return -1;
	}
	g_hinst = hInstance;
	int iNumArgs;
	LPWSTR *pszCommandLine =  CommandLineToArgvW(lpCmdLine,&iNumArgs);

	DWORD dwSize = dwUserNameSize;
	GetUserName(szUserName,&dwSize);

	if (iNumArgs >= 1)
	{
		
		if (_tcscmp(pszCommandLine[0],L"NEW_USERNAME") == 0)
		{
			fGotoNewScreen = TRUE;
			if (iNumArgs > 1)
			{
				_tcscpy_s(szUserName,dwUserNameSize, pszCommandLine[1]);
			}
		}
		else if (_tcscmp(pszCommandLine[0],L"ENABLESIGNATUREONLY") == 0)
		{
			SetPolicyValue(GPOPolicy::AllowSignatureOnlyKeys, 1);
			return 0;
		}
		else if (_tcscmp(pszCommandLine[0],L"ENABLENOEKU") == 0)
		{
			SetPolicyValue(GPOPolicy::AllowCertificatesWithNoEKU, 1);
			return 0;
		}
		else if (_tcscmp(pszCommandLine[0],L"ENABLETIMEINVALID") == 0)
		{
			SetPolicyValue(GPOPolicy::AllowTimeInvalidCertificates, 1);
			return 0;
		}
		else if (_tcscmp(pszCommandLine[0],L"TRUST") == 0)
		{
			if (iNumArgs < 2)
			{
				return 0;
			}
			DWORD dwCertSize = 0;
			CryptStringToBinary(pszCommandLine[1],0,CRYPT_STRING_BASE64,nullptr,&dwCertSize,nullptr,nullptr);
			PBYTE pbCertificate = (PBYTE) EIDAlloc(dwCertSize);
			CryptStringToBinary(pszCommandLine[1],0,CRYPT_STRING_BASE64,pbCertificate,&dwCertSize,nullptr,nullptr);
			PCCERT_CONTEXT pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING,pbCertificate, dwCertSize);
			if (pCertContext)
			{
				MakeTrustedCertifcate(pCertContext);
				CertFreeCertificateContext(pCertContext);
			}
			EIDFree(pbCertificate);
			return 0;
		} 
		else if(_tcscmp(pszCommandLine[0],L"REPORT") == 0)
		{
			if (iNumArgs < 2)
			{
				return 0;
			}
			CreateReport(pszCommandLine[1]);
			return 0;
		}
	}

	HPROPSHEETPAGE ahpsp[6];
	
	PROPSHEETPAGE psp;
	ZeroMemory(&psp,sizeof(PROPSHEETPAGE));
	psp.dwSize = sizeof(PROPSHEETPAGE);
	psp.hInstance = hInstance;
	psp.dwFlags =  PSP_USEHEADERTITLE;
	psp.lParam = 0;//(LPARAM) &wizdata;

	psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE1);
	psp.pszTemplate = MAKEINTRESOURCE(IDD_02ENABLE);
	psp.pfnDlgProc = WndProc_02ENABLE;
	ahpsp[0] = CreatePropertySheetPage(&psp);

	psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE2);
	psp.pszTemplate = MAKEINTRESOURCE(IDD_03NEW);
	psp.pfnDlgProc = WndProc_03NEW;
	ahpsp[1] = CreatePropertySheetPage(&psp);

	psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE3);
	psp.pszTemplate = MAKEINTRESOURCE(IDD_04CHECKS);
	psp.pfnDlgProc = WndProc_04CHECKS;
	ahpsp[2] = CreatePropertySheetPage(&psp);

	psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE4);
	psp.pszTemplate = MAKEINTRESOURCE(IDD_05PASSWORD);
	psp.pfnDlgProc = WndProc_05PASSWORD;
	ahpsp[3] = CreatePropertySheetPage(&psp);

	psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE5);
	psp.pszTemplate = MAKEINTRESOURCE(IDD_06TESTRESULTOK);
	psp.pfnDlgProc = WndProc_06TESTRESULTOK;
	ahpsp[4] = CreatePropertySheetPage(&psp);

	psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE5);
	psp.pszTemplate = MAKEINTRESOURCE(IDD_07TESTRESULTNOTOK);
	psp.pfnDlgProc = WndProc_07TESTRESULTNOTOK;
	ahpsp[5] = CreatePropertySheetPage(&psp);

	PROPSHEETHEADER psh;
	ZeroMemory(&psh,sizeof(PROPSHEETHEADER));
	psh.dwSize = sizeof(psh);
	psh.hInstance = hInstance;
	psh.hwndParent = nullptr;
	psh.phpage = ahpsp;
	psh.dwFlags = PSH_WIZARD | PSH_AEROWIZARD | PSH_USEHICON ;
	psh.pszbmWatermark = nullptr;
	psh.pszbmHeader = nullptr;
	psh.nStartPage = 1;
	psh.nPages = ARRAYSIZE(ahpsp);
	psh.hIcon = nullptr;
	psh.pszCaption = MAKEINTRESOURCE(IDS_CAPTION);

	HMODULE hDll = EIDLoadSystemLibrary(L"imageres.dll");
	if (hDll)
	{
		psh.hIcon = LoadIcon(hDll, MAKEINTRESOURCE(58));
		FreeLibrary(hDll);
	}

	 if (fGotoNewScreen)
	{
		if (AskForCard(szReader, dwReaderSize, szCard, dwCardSize))
		{
			psh.nStartPage = 1;
		}
		else
		{
			psh.nStartPage = 0;
		}
	}
	else 
	{
		// 02ENABLE
		psh.nStartPage = 0;
	}
	INT_PTR rc = PropertySheet(&psh);
	if (rc == -1)
	{
		MessageBoxWin32(GetLastError());
	}
    return 0;

}

