#include <Windows.h>
#include <tchar.h>
#include <CryptUIApi.h>
#include <ShObjIdl.h>
#include "global.h"
#include "EIDConfigurationWizard.h"
#include "../EIDCardLibrary/CertificateUtilities.h"
#include "../EIDCardLibrary/Tracing.h"
#include "../EIDCardLibrary/StringConversion.h"
#include <string>

// used to know what root certicate we are refering
// null = unknown
PCCERT_CONTEXT pRootCertificate = nullptr;

// Default certificate validity in years
constexpr WORD DEFAULT_CERT_VALIDITY_YEARS = 3;
constexpr WORD MAX_CERT_VALIDITY_YEARS = 30;
constexpr WORD MIN_CERT_VALIDITY_YEARS = 1;

// Helper function to get root CA expiry year
// Returns 0 if unable to determine
WORD GetRootCAExpiryYear(PCCERT_CONTEXT pRootCert)
{
	if (!pRootCert) return 0;
	SYSTEMTIME st;
	if (FileTimeToSystemTime(&(pRootCert->pCertInfo->NotAfter), &st))
	{
		return st.wYear;
	}
	return 0;
}

// Validate certificate validity against root CA and update warning
VOID ValidateCertificateValidity(HWND hWnd, PCCERT_CONTEXT pRootCert)
{
	WORD wValidityYears = (WORD)GetDlgItemInt(hWnd, IDC_03VALIDITYYEARS, nullptr, FALSE);
	SYSTEMTIME stNow;
	GetSystemTime(&stNow);
	WORD wCertExpiryYear = stNow.wYear + wValidityYears;
	WORD wCAExpiryYear = GetRootCAExpiryYear(pRootCert);

	if (wCAExpiryYear > 0 && wCertExpiryYear > wCAExpiryYear)
	{
		// Show warning - certificate will expire after root CA
		std::wstring szWarning = EID::LoadStringW(g_hinst, IDS_03VALIDITYWARNING);
		EID::SetWindowTextW(GetDlgItem(hWnd, IDC_03VALIDITYWARNING), szWarning);
		// Set text color to red (we'll use a simple approach - just show the text)
	}
	else
	{
		EID::SetWindowTextW(GetDlgItem(hWnd, IDC_03VALIDITYWARNING), L"");
	}
}

BOOL SelectFile(HWND hWnd)
{
	// select file to open
	PWSTR szFileName = nullptr;
	std::wstring szSpecContainer = EID::LoadStringW(g_hinst, IDS_03CONTAINERFILES);
	std::wstring szSpecAll = EID::LoadStringW(g_hinst, IDS_03ALLFILES);
	OPENFILENAME ofn;
	wchar_t szFile[MAX_PATH] = { 0 };
	std::wstring szFilter = EID::Format(L"%s%c*.pfx;*.p12%c%s%c*.*%c",
	                                     szSpecContainer.c_str(), 0, 0,
	                                     szSpecAll.c_str(), 0, 0);
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = ARRAYSIZE(szFile);
	ofn.lpstrFilter = szFilter.c_str();
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	if (GetOpenFileName(&ofn)==TRUE)
	{
		EID::SetWindowTextW(GetDlgItem(hWnd,IDC_03FILENAME), szFile);
		CheckDlgButton(hWnd,IDC_03IMPORT,BST_CHECKED);
		CheckDlgButton(hWnd,IDC_03USETHIS,BST_UNCHECKED);
		CheckDlgButton(hWnd,IDC_03_CREATE,BST_UNCHECKED);
		return TRUE;
	}
	return FALSE;
}

BOOL CreateRootCertificate()
{
	BOOL fReturn;
	wchar_t szComputerNameBuf[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD dwSize = ARRAYSIZE(szComputerNameBuf);
	GetComputerName(szComputerNameBuf, &dwSize);
	std::wstring szSubject = EID::Format(L"CN=%s", szComputerNameBuf);
	UI_CERTIFICATE_INFO CertificateInfo;
	memset(&CertificateInfo, 0, sizeof(CertificateInfo));
	CertificateInfo.dwSaveon = UI_CERTIFICATE_INFO_SAVEON_SYSTEMSTORE;
	CertificateInfo.dwKeyType = AT_SIGNATURE;
	CertificateInfo.bIsSelfSigned = TRUE;
	CertificateInfo.bHasSmartCardAuthentication = TRUE;
	CertificateInfo.bIsCA = TRUE;
	GetSystemTime(&(CertificateInfo.StartTime));
	GetSystemTime(&(CertificateInfo.EndTime));
	CertificateInfo.EndTime.wYear += 10;
	CertificateInfo.fReturnCerticateContext = TRUE;
	CertificateInfo.szSubject = (PWSTR)szSubject.c_str();
	fReturn = CreateCertificate(&CertificateInfo);
	DWORD dwError = GetLastError();
	if (fReturn)
	{
		pRootCertificate = CertificateInfo.pNewCertificate;
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"OK");
	}
	else
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CreateCertificate 0x%08X", dwError);
	}
	SetLastError(dwError);
	return fReturn;
}

BOOL CreateSmartCardCertificate(PCCERT_CONTEXT pCertificate, PWSTR wszReader, PWSTR wszCard, WORD wValidityYears)
{
	BOOL fReturn;
	UI_CERTIFICATE_INFO CertificateInfo;
	std::wstring szSubject = EID::Format(L"CN=%s", szUserName);
	memset(&CertificateInfo, 0, sizeof(CertificateInfo));
	CertificateInfo.dwSaveon = UI_CERTIFICATE_INFO_SAVEON_SMARTCARD;
	CertificateInfo.wszReaderName = wszReader;
	CertificateInfo.wszCardName = wszCard;
	CertificateInfo.dwKeyType = AT_KEYEXCHANGE;
	CertificateInfo.bHasSmartCardAuthentication = TRUE;
	CertificateInfo.pRootCertificate = pCertificate;
	CertificateInfo.szSubject = (PWSTR)szSubject.c_str();
	GetSystemTime(&(CertificateInfo.StartTime));
	GetSystemTime(&(CertificateInfo.EndTime));
	// Use configurable validity period
	CertificateInfo.EndTime.wYear += wValidityYears;
	EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"Creating certificate with %d year validity", wValidityYears);
	fReturn = CreateCertificate(&CertificateInfo);
	DWORD dwError = GetLastError();
	if (fReturn)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"OK");
	}
	else
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CreateCertificate 0x%08X", dwError);
	}
	SetLastError(dwError);
	return fReturn;
}

VOID UpdateCertificatePanel(HWND hWnd)
{
	wchar_t szBuffer[1024];
	wchar_t szBuffer2[1024];
	wchar_t szLocalDate[255];
	wchar_t szLocalTime[255];
	SYSTEMTIME st;
	SendDlgItemMessage(hWnd,IDC_03CERTIFICATEPANEL,LB_RESETCONTENT,0,0);
	// object :
	CertGetNameString(pRootCertificate,CERT_NAME_SIMPLE_DISPLAY_TYPE,0,nullptr,szBuffer2,ARRAYSIZE(szBuffer2));
	std::wstring szMessage = EID::LoadStringW(g_hinst, IDS_03OBJECT);
	std::wstring szBuf = EID::Format(szMessage.c_str(), szBuffer2);
	SendDlgItemMessage(hWnd,IDC_03CERTIFICATEPANEL,LB_ADDSTRING,0,(LPARAM) szBuf.c_str());
	// delivered :
	FileTimeToSystemTime( &(pRootCertificate->pCertInfo->NotBefore), &st );
	GetDateFormat( LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, nullptr, szLocalDate, ARRAYSIZE(szLocalDate));
	GetTimeFormat( LOCALE_USER_DEFAULT, 0, &st, nullptr, szLocalTime, ARRAYSIZE(szLocalTime));

	szMessage = EID::LoadStringW(g_hinst, IDS_03DELIVERED);
	szBuf = EID::Format(szMessage.c_str(), szLocalDate, szLocalTime);
	SendDlgItemMessage(hWnd,IDC_03CERTIFICATEPANEL,LB_ADDSTRING,0,(LPARAM) szBuf.c_str());

	// expires :
	FileTimeToSystemTime( &(pRootCertificate->pCertInfo->NotAfter), &st );
	GetDateFormat( LOCALE_USER_DEFAULT, DATE_LONGDATE, &st, nullptr, szLocalDate, ARRAYSIZE(szLocalDate));
	GetTimeFormat( LOCALE_USER_DEFAULT, 0, &st, nullptr, szLocalTime, ARRAYSIZE(szLocalTime));

	szMessage = EID::LoadStringW(g_hinst, IDS_03EXPIRES);
	szBuf = EID::Format(szMessage.c_str(), szLocalDate, szLocalTime);
	SendDlgItemMessage(hWnd,IDC_03CERTIFICATEPANEL,LB_ADDSTRING,0,(LPARAM) szBuf.c_str());

	// select option
	CheckDlgButton(hWnd,IDC_03IMPORT,BST_UNCHECKED);
	CheckDlgButton(hWnd,IDC_03USETHIS,BST_CHECKED);
	CheckDlgButton(hWnd,IDC_03_CREATE,BST_UNCHECKED);
}

INT_PTR CALLBACK	WndProc_03NEW(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId;
	int wmEvent;
	LPNMHDR pnmh;
	CRYPTUI_VIEWCERTIFICATE_STRUCT certViewInfo;
	BOOL fPropertiesChanged = FALSE;
	switch(message)
	{
		case WM_NOTIFY :
        pnmh = (LPNMHDR)lParam;
        switch(pnmh->code)
        {
			case PSN_SETACTIVE :
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Activate");
				//this is an interior page
				PropSheet_SetWizButtons(hWnd, PSWIZB_BACK | PSWIZB_NEXT);
				// Initialize certificate validity years control
				SetDlgItemInt(hWnd, IDC_03VALIDITYYEARS, DEFAULT_CERT_VALIDITY_YEARS, FALSE);
				EID::SetWindowTextW(GetDlgItem(hWnd, IDC_03VALIDITYWARNING), L"");
				if (pRootCertificate)
				{
					CertFreeCertificateContext(pRootCertificate);
					pRootCertificate = nullptr;
				}
				pRootCertificate = SelectFirstCertificateWithPrivateKey();
				if (pRootCertificate)
				{
					CheckDlgButton(hWnd,IDC_03USETHIS,BST_CHECKED);
					UpdateCertificatePanel(hWnd);
					// Validate against root CA
					ValidateCertificateValidity(hWnd, pRootCertificate);
				}
				else
				{
					CheckDlgButton(hWnd,IDC_03_CREATE,BST_CHECKED);
				}
				break;
			case PSN_WIZBACK:
				if (pRootCertificate)
				{
					CertFreeCertificateContext(pRootCertificate);
					pRootCertificate = nullptr;
				}
				break;
			case PSN_WIZNEXT:
				if (IsDlgButtonChecked(hWnd,IDC_03DELETE))
				{
					EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"IDC_03DELETE");
					// delete all data
					if (!ClearCard(szReader, szCard))
					{
						DWORD dwError = GetLastError();
						if (dwError != SCARD_W_CANCELLED_BY_USER)
							MessageBoxWin32Ex(dwError,hWnd);
						SetWindowLongPtr(hWnd,DWLP_MSGRESULT,-1);
						return TRUE;
					}
				}
				if (IsDlgButtonChecked(hWnd,IDC_03_CREATE))
				{
					EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"IDC_03_CREATE");
					// Get validity years from UI
					WORD wValidityYears = (WORD)GetDlgItemInt(hWnd, IDC_03VALIDITYYEARS, nullptr, FALSE);
					if (wValidityYears < MIN_CERT_VALIDITY_YEARS) wValidityYears = MIN_CERT_VALIDITY_YEARS;
					if (wValidityYears > MAX_CERT_VALIDITY_YEARS) wValidityYears = MAX_CERT_VALIDITY_YEARS;
					// create self signed certificate as root
					DWORD dwReturn = -1;
					if (CreateRootCertificate())
					{
						if (CreateSmartCardCertificate(pRootCertificate, szReader, szCard, wValidityYears))
						{
							//  OK
						}
						else
						{
							DWORD dwError = GetLastError();
							if (dwError != SCARD_W_CANCELLED_BY_USER)
								MessageBoxWin32Ex(dwError,hWnd);
							SetWindowLongPtr(hWnd,DWLP_MSGRESULT,-1);
							return TRUE;
						}
					}
					else
					{
						MessageBoxWin32Ex(GetLastError(),hWnd);
						SetWindowLongPtr(hWnd,DWLP_MSGRESULT,-1);
						return TRUE;
					}
					// cancel
					break;
				}
				else if (IsDlgButtonChecked(hWnd,IDC_03USETHIS))
				{
					EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"IDC_03USETHIS");
					if (!pRootCertificate)
					{
						SetWindowLongPtr(hWnd,DWLP_MSGRESULT,-1);
						return TRUE;
					}
					// Get validity years from UI
					WORD wValidityYears = (WORD)GetDlgItemInt(hWnd, IDC_03VALIDITYYEARS, nullptr, FALSE);
					if (wValidityYears < MIN_CERT_VALIDITY_YEARS) wValidityYears = MIN_CERT_VALIDITY_YEARS;
					if (wValidityYears > MAX_CERT_VALIDITY_YEARS) wValidityYears = MAX_CERT_VALIDITY_YEARS;
					if (!CreateSmartCardCertificate(pRootCertificate, szReader, szCard, wValidityYears))
					{
						DWORD dwError = GetLastError();
						if (dwError != SCARD_W_CANCELLED_BY_USER)
							MessageBoxWin32Ex(dwError,hWnd);
						SetWindowLongPtr(hWnd,DWLP_MSGRESULT,-1);
						return TRUE;
					}
				}
				else if (IsDlgButtonChecked(hWnd,IDC_03IMPORT))
				{
					EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"IDC_03IMPORT");
					std::wstring szFileName = EID::GetWindowTextW(GetDlgItem(hWnd,IDC_03FILENAME));
					std::wstring wszImportPassword = EID::GetWindowTextW(GetDlgItem(hWnd,IDC_03IMPORTPASSWORD));
					if (!ImportFileToSmartCard((PWSTR)szFileName.c_str(), (PWSTR)wszImportPassword.c_str(), szReader, szCard))
					{
						MessageBoxWin32Ex(GetLastError(),hWnd);
						SetWindowLongPtr(hWnd,DWLP_MSGRESULT,-1);
						return TRUE;
					}

				}
				break;
		}
		break;
		case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
			case IDC_03VALIDITYYEARS:
				// Handle changes to validity years - validate against root CA
				if (wmEvent == EN_CHANGE && pRootCertificate)
				{
					ValidateCertificateValidity(hWnd, pRootCertificate);
				}
				break;
			case IDC_03SELECT:
				if (pRootCertificate)
				{
						CertFreeCertificateContext(pRootCertificate);
						pRootCertificate = nullptr;
				}
				pRootCertificate = SelectCertificateWithPrivateKey(hWnd);
				if (pRootCertificate)
				{
					UpdateCertificatePanel(hWnd);
					// Validate against new root CA
					ValidateCertificateValidity(hWnd, pRootCertificate);
				}
				break;
			case IDC_03SHOW:
				if (pRootCertificate)
				{
					std::wstring szTitle = EID::LoadStringW(g_hinst, IDS_03CERTVIEWTITLE);
					certViewInfo.dwSize = sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT);
					certViewInfo.hwndParent = hWnd;
					certViewInfo.dwFlags = CRYPTUI_DISABLE_EDITPROPERTIES | CRYPTUI_DISABLE_ADDTOSTORE | CRYPTUI_DISABLE_EXPORT | CRYPTUI_DISABLE_HTMLLINK;
					certViewInfo.szTitle = (PWSTR)szTitle.c_str();
					certViewInfo.pCertContext = pRootCertificate;
					certViewInfo.cPurposes = 0;
					certViewInfo.rgszPurposes = nullptr;
					certViewInfo.pCryptProviderData = nullptr;
					certViewInfo.hWVTStateData = nullptr;
					certViewInfo.fpCryptProviderDataTrustedUsage = FALSE;
					certViewInfo.idxSigner = 0;
					certViewInfo.idxCert = 0;
					certViewInfo.fCounterSigner = FALSE;
					certViewInfo.idxCounterSigner = 0;
					certViewInfo.cStores = 0;
					certViewInfo.rghStores = nullptr;
					certViewInfo.cPropSheetPages = 0;
					certViewInfo.rgPropSheetPages = nullptr;
					certViewInfo.nStartPage = 0;

					CryptUIDlgViewCertificate(&certViewInfo,&fPropertiesChanged);
				}
				break;
			case IDC_03SELECTFILE:
				SelectFile(hWnd);
				break;
		}
		break;
    }
	return FALSE;
}