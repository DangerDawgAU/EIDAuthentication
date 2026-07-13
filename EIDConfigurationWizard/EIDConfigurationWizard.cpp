#include <Windows.h>
#include <tchar.h>
#include <credentialprovider.h>
#include <string>

#include "EIDConfigurationWizard.h"
#include "global.h"

// Securely clear the global password buffer
// This should be called when the wizard completes or is cancelled
VOID SecurelyClearPassword()
{
	if (szPassword[0] != L'\0')
	{
		SecureZeroMemory(szPassword, dwPasswordSize * sizeof(WCHAR));
	}
}

#include "../EIDCardLibrary/EIDCardLibrary.h"
#include "../EIDCardLibrary/Package.h"
#include "../EIDCardLibrary/Tracing.h"
#include "../EIDCardLibrary/Registration.h"
#include "../EIDCardLibrary/CertificateUtilities.h"
#include "../EIDCardLibrary/CertificateValidation.h"
#include "../EIDCardLibrary/GPO.h" // NOSONAR - INCLUDE-01: include placement after SecurelyClearPassword is intentional for build ordering
#include "../EIDCardLibrary/CommonManifest.h"
#include "../EIDCardLibrary/StringConversion.h"

// M2 (security uplift): the elevated instance makes machine-wide trust changes straight from
// its command-line arguments (install an argv[1] root/CA certificate via TRUST, or flip the
// AllowSignatureOnly / AllowNoEKU / AllowTimeInvalid GPOs). A single, under-described UAC prompt
// otherwise silently installs a machine-wide trust anchor -> every cert the attacker issues then
// chains to trusted -> smart-card logon bypass. Require an explicit, SPECIFIC confirmation inside
// the elevated process, showing exactly which certificate / weakening is about to be applied,
// defaulting to "No". The interactive wizard's own "solve trust" path re-derives the certificate
// from the inserted card (CContainerHolder::Solve) and is unaffected.
static void FormatSha1Thumbprint(PCCERT_CONTEXT pCertContext, PWSTR szOut, DWORD cchOut)  // NOSONAR - LSASS-01: C-style buffer required by Win32 API
{
	BYTE rgbHash[20] = {0};  // NOSONAR - LSASS-01: C-style buffer required by Win32 API
	DWORD cbHash = sizeof(rgbHash);
	if (cchOut == 0) return;
	szOut[0] = L'\0';
	if (CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, rgbHash, &cbHash))
	{
		DWORD pos = 0;
		for (DWORD i = 0; i < cbHash; i++)
		{
			int n = swprintf_s(szOut + pos, cchOut - pos, (i ? L" %02X" : L"%02X"), rgbHash[i]);
			if (n < 0) break;
			pos += (DWORD) n;
		}
	}
}

static BOOL ConfirmTrustInstall(PCCERT_CONTEXT pCertContext)
{
	if (!pCertContext) return FALSE;
	WCHAR szSubject[512] = L"";  // NOSONAR - LSASS-01: C-style buffer required by Win32 API
	WCHAR szIssuer[512] = L"";  // NOSONAR - LSASS-01: C-style buffer required by Win32 API
	WCHAR szThumb[128] = L"";  // NOSONAR - LSASS-01: C-style buffer required by Win32 API
	CertGetNameStringW(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, szSubject, ARRAYSIZE(szSubject));
	CertGetNameStringW(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, nullptr, szIssuer, ARRAYSIZE(szIssuer));
	FormatSha1Thumbprint(pCertContext, szThumb, ARRAYSIZE(szThumb));

	WCHAR szMsg[2048];  // NOSONAR - LSASS-01: C-style buffer required by Win32 API
	swprintf_s(szMsg, ARRAYSIZE(szMsg),
		L"You are about to install a certificate as a TRUSTED AUTHORITY for ALL users on this "
		L"computer. Any smart-card certificate that chains to it will then be accepted for logon.\n\n"
		L"Subject:\t%ls\n"
		L"Issuer:\t%ls\n"
		L"SHA-1:\t%ls\n\n"
		L"Only continue if you obtained this certificate from a trusted source and were expecting "
		L"this prompt. Install it machine-wide?",
		szSubject[0] ? szSubject : L"(unknown)",
		szIssuer[0] ? szIssuer : L"(unknown)",
		szThumb[0] ? szThumb : L"(unavailable)");

	return MessageBoxW(nullptr, szMsg, L"Confirm trusted certificate install",
		MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2 | MB_SETFOREGROUND) == IDYES;
}

static BOOL ConfirmPolicyWeaken(LPCWSTR szWhat)
{
	WCHAR szMsg[1024];  // NOSONAR - LSASS-01: C-style buffer required by Win32 API
	swprintf_s(szMsg, ARRAYSIZE(szMsg),
		L"You are about to WEAKEN smart-card logon security for ALL users on this computer:\n\n"
		L"    %ls\n\n"
		L"Only continue if you were expecting this prompt. Apply this change machine-wide?", szWhat);
	return MessageBoxW(nullptr, szMsg, L"Confirm security policy change",
		MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2 | MB_SETFOREGROUND) == IDYES;
}

INT_PTR CALLBACK	WndProc_01MAIN(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WndProc_02ENABLE(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WndProc_03NEW(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WndProc_04CHECKS(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WndProc_05PASSWORD(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WndProc_06TESTRESULTOK(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	WndProc_07TESTRESULTNOTOK(HWND, UINT, WPARAM, LPARAM);

BOOL fShowNewCertificatePanel;  // NOSONAR - RUNTIME-01: UI state flag, modified at runtime
BOOL fGotoNewScreen = FALSE;  // NOSONAR - RUNTIME-01: UI navigation flag, modified at runtime
HINSTANCE g_hinst;  // NOSONAR - RUNTIME-01: HINSTANCE, set by Windows at DLL load
WCHAR szReader[256];  // NOSONAR - LSASS-01: C-style buffer for LSASS safety
const DWORD dwReaderSize = ARRAYSIZE(szReader);
WCHAR szCard[256];  // NOSONAR - LSASS-01: C-style buffer for LSASS safety
const DWORD dwCardSize = ARRAYSIZE(szCard);
WCHAR szUserName[256];  // NOSONAR - LSASS-01: C-style buffer for LSASS safety
const DWORD dwUserNameSize = ARRAYSIZE(szUserName);
WCHAR szPassword[256];  // NOSONAR - LSASS-01: C-style buffer for LSASS safety
const DWORD dwPasswordSize = ARRAYSIZE(szPassword);
WCHAR szCAName[256];  // NOSONAR - LSASS-01: C-style buffer for LSASS safety
const DWORD dwCANameSize = ARRAYSIZE(szCAName);

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
			if (ConfirmPolicyWeaken(L"Allow smart cards whose certificate has signature-only keys."))
			{
				SetPolicyValue(GPOPolicy::AllowSignatureOnlyKeys, 1);
			}
			return 0;
		}
		else if (_tcscmp(pszCommandLine[0],L"ENABLENOEKU") == 0)
		{
			if (ConfirmPolicyWeaken(L"Allow certificates that do NOT have the Smart Card Logon EKU."))
			{
				SetPolicyValue(GPOPolicy::AllowCertificatesWithNoEKU, 1);
			}
			return 0;
		}
		else if (_tcscmp(pszCommandLine[0],L"ENABLETIMEINVALID") == 0)
		{
			if (ConfirmPolicyWeaken(L"Allow expired or not-yet-valid certificates for logon."))
			{
				SetPolicyValue(GPOPolicy::AllowTimeInvalidCertificates, 1);
			}
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
			PBYTE pbCertificate = (PBYTE) EIDAlloc(dwCertSize);  // NOSONAR (EXPLICIT-TYPE-04) - Explicit type preferred for code clarity
			CryptStringToBinary(pszCommandLine[1],0,CRYPT_STRING_BASE64,pbCertificate,&dwCertSize,nullptr,nullptr);
			PCCERT_CONTEXT pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING,pbCertificate, dwCertSize);
			if (pCertContext)
			{
				// M2: require explicit confirmation of the specific certificate before it becomes a
				// machine-wide trust anchor. Defaults to No; a silent/forced elevation cannot proceed.
				if (ConfirmTrustInstall(pCertContext))
				{
					MakeTrustedCertifcate(pCertContext);
				}
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

