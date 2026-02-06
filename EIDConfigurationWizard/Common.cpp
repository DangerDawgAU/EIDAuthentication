#include <Windows.h>
#include <tchar.h>
#include <LM.h>

#include "../EIDCardLibrary/GPO.h"
#include "../EIDCardLibrary/Tracing.h"
#include "../EIDCardLibrary/EIDCardLibrary.h"
#include "EIDConfigurationWizard.h"

extern HINSTANCE g_hinst;

BOOL IsElevated()
{
	BOOL fReturn = FALSE;
	HANDLE hToken	= nullptr;

	if ( !OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ) )
	{
		return FALSE;
	}

	TOKEN_ELEVATION te = { 0 };
	DWORD dwReturnLength = 0;

	if ( GetTokenInformation(
				hToken,
				TokenElevation,
				&te,
				sizeof( te ),
				&dwReturnLength ) )
	{
	
		fReturn = te.TokenIsElevated ? TRUE : FALSE; 
	}

	CloseHandle(hToken);

	return fReturn;
}

BOOL IsCurrentUserBelongToADomain()
{
	BOOL fReturn = FALSE;
	HANDLE hToken	= nullptr;
	PTOKEN_USER  ptiUser = nullptr;
	__try
	{
		if ( !OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ) )
		{
			__leave;
		}
		
		DWORD        cbti     = 0;
		// Obtain the size of the user information in the token.
		if (GetTokenInformation(hToken, TokenUser, nullptr, 0, &cbti)) {

			// Call should have failed due to zero-length buffer.
			__leave;
   
		} else {

			// Call should have failed due to zero-length buffer.
			if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
				__leave;
			}
		}

		// Allocate buffer for user information in the token.
		ptiUser = (PTOKEN_USER) HeapAlloc(GetProcessHeap(), 0, cbti);
		if (!ptiUser)
			__leave;

		// Retrieve the user information from the token.
		if (!GetTokenInformation(hToken, TokenUser, ptiUser, cbti, &cbti))
			__leave;

		TCHAR szUser[255];
		DWORD cchUser = ARRAYSIZE(szUser);
		TCHAR szDomain[255];
		DWORD cchDomain = ARRAYSIZE(szDomain);
		SID_NAME_USE snu;
		if (!LookupAccountSid(nullptr, ptiUser->User.Sid, szUser, &cchUser,
            szDomain, &cchDomain, &snu))
		{
			__leave;
		}
		TCHAR szComputerName[255];
		DWORD cchComputerName = ARRAYSIZE(szComputerName);
		if (!GetComputerName(szComputerName,&cchComputerName))
		{
			__leave;
		}
		if (_tcsicmp(szComputerName,szDomain) != 0)
		{
			fReturn = TRUE;
		}
	}
	__finally
	{
		if (hToken)
			CloseHandle(hToken);
		if (ptiUser)
			HeapFree(GetProcessHeap(), 0, ptiUser);
	}
	return fReturn;
}