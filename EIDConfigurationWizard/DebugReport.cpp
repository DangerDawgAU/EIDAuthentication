#include <Windows.h>
#include <tchar.h>
#include <wincred.h>
#include <NTSecAPI.h>
#include <random>
#include <string>

#include "global.h"
#include "EIDConfigurationWizard.h"

#include "../EIDCardLibrary/EIDCardLibrary.h"
#include "../EIDCardLibrary/TraceExport.h"
#include "../EIDCardLibrary/Package.h"
#include "../EIDCardLibrary/Tracing.h"
#include "../EIDCardLibrary/StringConversion.h"
// OnlineDatabase.h removed - internet reporting functionality disabled
#include "../EIDCardLibrary/EIDAuthenticateVersion.h"

#include "../EIDCardLibrary/CContainer.h"
#include "../EIDCardLibrary/StringConversion.h"
#include <string>
#include "../EIDCardLibrary/CContainerHolderFactory.h"
#include "../EIDCardLibrary/StringConversion.h"
#include <string>

#include "CContainerHolder.h"
#pragma comment(lib,"Credui")

// Static buffers for Windows API compatibility (C++23 /Zc:strictStrings)
static char s_szMyTest[] = "MYTEST";
static constexpr size_t s_szMyTestLen = sizeof(s_szMyTest) - 1;  // Length without null terminator (SonarQube cpp:S5813)
static wchar_t s_wszEtlPath[] = L"c:\\Windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl";

BOOL TestLogon(HWND hMainWnd)
{
	BOOL save = false;
	DWORD authPackage = 0;
	LPVOID authBuffer;
	ULONG authBufferSize = 0;
	CREDUI_INFO credUiInfo;
	BOOL fReturn = FALSE;
	DWORD dwError = 0;

	LSA_HANDLE hLsa;
	LSA_STRING Origin = { (USHORT)s_szMyTestLen, (USHORT)sizeof(s_szMyTest), s_szMyTest };
	QUOTA_LIMITS Quota = {0};
	TOKEN_SOURCE Source = { "TEST", { 0, 101 } };
	MSV1_0_INTERACTIVE_PROFILE *Profile;
	ULONG ProfileLen;
	LUID Luid;
	NTSTATUS err,stat;
	HANDLE Token;
	DWORD dwFlag = CREDUIWIN_AUTHPACKAGE_ONLY | CREDUIWIN_ENUMERATE_CURRENT_USER;
	RetrieveNegotiateAuthPackage(&authPackage);
	
	CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
	TCHAR szTitle[256] = L"";
	TCHAR szMessage[256] = L"";
	TCHAR szCaption[256] = L"";
	LoadString(g_hinst, IDS_05CREDINFOCAPTION, szCaption, ARRAYSIZE(szCaption));
	credUiInfo.pszCaptionText = szCaption;
	credUiInfo.pszMessageText = szMessage;
	credUiInfo.cbSize = sizeof(credUiInfo);
	credUiInfo.hbmBanner = nullptr;
	credUiInfo.hwndParent = hMainWnd;

	DWORD result = CredUIPromptForWindowsCredentials(&credUiInfo, 0, &authPackage,
					nullptr, 0, &authBuffer, &authBufferSize, &save, dwFlag);
	if (result == ERROR_SUCCESS)
	{
		err = LsaConnectUntrusted(&hLsa);
		/* Find the setuid package and call it */
		err = LsaLogonUser(hLsa, &Origin, Interactive, authPackage, authBuffer,authBufferSize,nullptr, &Source, (PVOID*)&Profile, &ProfileLen, &Luid, &Token, &Quota, &stat);
		DWORD dwSize = sizeof(MSV1_0_INTERACTIVE_PROFILE);
		LsaDeregisterLogonProcess(hLsa);
		if (err)
		{
			dwError = LsaNtStatusToWinError(err);
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"LsaLogonUser error 0x%08X", result);
		}
		else
		{
			fReturn = TRUE;
			
			LsaFreeReturnBuffer(Profile);
			CloseHandle(Token);
			
		}
		CoTaskMemFree(authBuffer);
	}
	else
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CredUIPromptForWindowsCredentials error 0x%08X", result);
		dwError = result;
	}
	SetLastError(dwError);
	return fReturn;
}

HANDLE hInternalLogWriteHandle = nullptr;

HANDLE StartReport(PTSTR szLogFile)
{
	DWORD dwError = 0;
	BOOL fSuccess = FALSE;
	HANDLE hOutput = INVALID_HANDLE_VALUE;
	__try
	{
		//  Creates the new file to write to for the upper-case version.
		hOutput = CreateFile(szLogFile, // file name 
							   GENERIC_WRITE,        // open for write 
							   0,                    // do not share
							   nullptr,                 // default security
							   CREATE_ALWAYS,        // overwrite existing
							   FILE_ATTRIBUTE_NORMAL,// normal file
							   nullptr);                // no template
		if (hOutput == INVALID_HANDLE_VALUE) 
		{ 
			dwError = GetLastError();
			__leave;
		}
		hInternalLogWriteHandle = hOutput;
		// disable the logging, just in case if was active
		StopLogging();
		// enable the logging
		if (!StartLogging())
		{
			dwError = GetLastError();
			__leave;
		}
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Starting report");
		fSuccess = TRUE;
	}
	__finally
	{
		if (!fSuccess)
		{
			if (hOutput != INVALID_HANDLE_VALUE)
				CloseHandle(hOutput);
			hOutput = INVALID_HANDLE_VALUE;
		}
	}
	SetLastError(dwError);
	return hOutput;
}

// from previous step
// credentials
extern CContainerHolderFactory<CContainerHolderTest> *pCredentialList;
// selected credential
extern DWORD dwCurrentCredential;

BOOL DoTheActionToBeTraced()
{
	DWORD dwError;
	BOOL fSuccess = FALSE;
	PCCERT_CONTEXT pCertContext = nullptr;
	__try
	{
		
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Starting report");
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Version : %S", EIDAuthenticateVersionText);
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"===============");
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Register the certificate");
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"===============");
		// register the package again
		CContainerHolderTest* MyTest = pCredentialList->GetContainerHolderAt(dwCurrentCredential);
		CContainer* container = MyTest->GetContainer();
		pCertContext = container->GetCertificate();
		fSuccess = LsaEIDCreateStoredCredential(szUserName, szPassword, pCertContext, container->GetKeySpec() == AT_KEYEXCHANGE);
		if (!fSuccess)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Test failed with 0x%08X", dwError);
			__leave;
		}
		
		// call for a test
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Test Logon");
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"===============");
		if (!TestLogon(nullptr))
		{
			dwError = GetLastError();
			if (dwError == ERROR_CANCELLED)
			{
				EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"TestLogonCancelled");
				__leave;
			}
			else
			{
				EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Test failed with 0x%08X", dwError);
			}
		}
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Success !!!");
		fSuccess = TRUE;
	}
	__finally
	{
		if (pCertContext)
			CertFreeCertificateContext(pCertContext);
	}
	EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Ending tests");
	EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"===============");
	SetLastError(dwError);
	return fSuccess;
}

// called from the wizard (non elevated)
// create the elevated process
BOOL CreateDebugReport(PTSTR szLogFile)
{
	BOOL fReturn = FALSE;
	DWORD dwError = 0;
	TCHAR szNamedPipeName[256] = L"\\\\.\\pipe\\EIDAuthenticateWizard";
	HANDLE hNamedPipe = INVALID_HANDLE_VALUE;
	TCHAR szParameter[356];
	DWORD dwWrite;
	__try
	{
		// run the process of the wizard with a special parameter, elevated
		// so the tracing can be done in another process
		// and no information is transmitted to that process

		// create a named pipe for intercommunication process
		// generate the name

		static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);

		for (int i = 0; i < 10; ++i) {
			szNamedPipeName[_tcslen(szNamedPipeName)] = alphanum[dis(gen)];
		}

		hNamedPipe = CreateNamedPipe(szNamedPipeName,PIPE_ACCESS_DUPLEX,0,PIPE_UNLIMITED_INSTANCES,0,0,0,nullptr);
		if (hNamedPipe == INVALID_HANDLE_VALUE)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CreateNamedPipe 0x%08X", GetLastError());
			__leave;
		}
		_stprintf_s(szParameter,ARRAYSIZE(szParameter),L"REPORT %s",szNamedPipeName);
		// launch the wizard elevated
		SHELLEXECUTEINFO shExecInfo;
		TCHAR szName[1024];
		GetModuleFileName(GetModuleHandle(nullptr),szName, ARRAYSIZE(szName));

		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		shExecInfo.hwnd = nullptr;
		shExecInfo.lpVerb = L"runas";
		shExecInfo.lpFile = szName;
		// sending the named pipe name so the other process can connect
		shExecInfo.lpParameters = szParameter;
		shExecInfo.lpDirectory = nullptr;
		shExecInfo.nShow = SW_NORMAL;
		shExecInfo.hInstApp = nullptr;

		if (!ShellExecuteEx(&shExecInfo))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CreateNamedPipe 0x%08X", GetLastError());
		}
		else
		{
			if (! (ConnectNamedPipe(hNamedPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED)))
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CreateNamedPipe 0x%08X", GetLastError());
				__leave;
			}
			// send to the process the log file name
			WriteFile(hNamedPipe,szLogFile,(DWORD) ((_tcslen(szLogFile) +1) * sizeof(TCHAR)),&dwWrite,nullptr);
			Sleep(1000);
			// do the action ...
			DoTheActionToBeTraced();
			Sleep(1000);
			// send to the process the order to stop
			// an empty line will do it
			WriteFile(hNamedPipe,L"\n",2 * sizeof(TCHAR),nullptr,nullptr);
			DisconnectNamedPipe(hNamedPipe);
			// then wait to its stop to have the file (if we don't, the file can be not written)
			if (WaitForSingleObject(shExecInfo.hProcess, INFINITE) == WAIT_OBJECT_0)
			{
				fReturn = TRUE;
			}
			else
			{
				dwError = GetLastError();
			}
		}
	}
	__finally
	{
		if (hNamedPipe != INVALID_HANDLE_VALUE)
			CloseHandle(hNamedPipe);
	}
	SetLastError(dwError);
	return fReturn;
}

// called from the elevated process
VOID CreateReport(PTSTR szNamedPipeName)
{
	// read the file from the command line
	TCHAR szFile [256];
	HANDLE hReport = INVALID_HANDLE_VALUE;
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	DWORD dwRead;
	__try
	{
		hPipe = CreateFile( szNamedPipeName,GENERIC_READ |GENERIC_WRITE,0,nullptr,OPEN_EXISTING, 0,nullptr);
		if (hPipe == INVALID_HANDLE_VALUE)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"INVALID_HANDLE_VALUE 1");
			if (GetLastError() != ERROR_PIPE_BUSY) 
			{
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"hPipe connect 0x%08X",GetLastError());
				__leave;
			}
			if ( ! WaitNamedPipe(szNamedPipeName, 20000)) 
			{
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"WaitNamedPipe 0x%08X",GetLastError());
				__leave;
			}
			hPipe = CreateFile( szNamedPipeName,GENERIC_READ |GENERIC_WRITE,0,nullptr,OPEN_EXISTING, 0,nullptr);
		}
		if (hPipe == INVALID_HANDLE_VALUE)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"INVALID_HANDLE_VALUE 2 0X%08X",GetLastError());
			__leave;
		}
		if (!ReadFile(hPipe,szFile,ARRAYSIZE(szFile) * sizeof(TCHAR), &dwRead,nullptr))
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"ReadFile 0X%08X",GetLastError());
			__leave;
		}
		
		hReport = StartReport(szFile);
		
		// fait for <Enter> to quit
		if (!ReadFile(hPipe,szFile,1 * sizeof(TCHAR), &dwRead,nullptr))
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"ReadFile 0X%08X",GetLastError());
			__leave;
		}

		// write the ouput to the log fil
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Ending report");
		// disable the logging
		StopLogging();
		// get the text
		ExportOneTraceFile(hInternalLogWriteHandle, s_wszEtlPath);
	}
	__finally
	{
		if (hReport != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hReport);
		}
	}
}

// SendReport function removed - internet reporting functionality disabled