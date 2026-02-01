#include <windows.h>
#include <tchar.h>
#include <wincred.h>
#include <ntsecapi.h>
#include <wmistr.h>
#include <Evntrace.h>
#include <random>

#include "global.h"
#include "EIDConfigurationWizard.h"

#include "../EIDCardLibrary/EIDCardLibrary.h"
#include "../EIDCardLibrary/Package.h"
#include "../EIDCardLibrary/Tracing.h"
// OnlineDatabase.h removed - internet reporting functionality disabled
#include "../EIDCardLibrary/EIDAuthenticateVersion.h"

#include "../EIDCardLibrary/CContainer.h"
#include "../EIDCardLibrary/CContainerHolderFactory.h"

#include "CContainerHolder.h"
#pragma comment(lib,"Credui")

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
	LSA_STRING Origin = { (USHORT)strlen("MYTEST"), (USHORT)sizeof("MYTEST"), "MYTEST" };
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
	TCHAR szTitle[256] = TEXT("");
	TCHAR szMessage[256] = TEXT("");
	TCHAR szCaption[256] = TEXT("");
	LoadString(g_hinst, IDS_05CREDINFOCAPTION, szCaption, ARRAYSIZE(szCaption));
	//LoadString(g_hinst, IDS_05CREDINFOMESSAGE, szMessage, ARRAYSIZE(szMessage));
	credUiInfo.pszCaptionText = szCaption;
	credUiInfo.pszMessageText = szMessage;
	credUiInfo.cbSize = sizeof(credUiInfo);
	credUiInfo.hbmBanner = nullptr;
	credUiInfo.hwndParent = hMainWnd;

	DWORD result = CredUIPromptForWindowsCredentials(&(credUiInfo), 0, &(authPackage), 
					nullptr, 0, &authBuffer, &authBufferSize, &(save), dwFlag);
	if (result == ERROR_SUCCESS)
	{
		err = LsaConnectUntrusted(&hLsa);
		/* Find the setuid package and call it */
		err = LsaLogonUser(hLsa, &Origin, (SECURITY_LOGON_TYPE)  Interactive , authPackage, authBuffer,authBufferSize,nullptr, &Source, (PVOID*)&Profile, &ProfileLen, &Luid, &Token, &Quota, &stat);
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
	else //if (result == ERROR_CANCELLED)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CredUIPromptForWindowsCredentials error 0x%08X", result);
		//fReturn = TRUE;
		dwError = result;
	}
	//else
	//{
	//	//MessageBoxWin32(GetLastError());
	//}
	//CredUIConfirmCredentials(nullptr,FALSE);
	SetLastError(dwError);
	return fReturn;
}

HANDLE hInternalLogWriteHandle = nullptr;

VOID WINAPI ProcessEvents(PEVENT_TRACE pEvent)
{
  // Is this the first event of the session? The event is available only if
  // you are consuming events from a log file, not a real-time session.
  {
    //Process the event. The pEvent->MofData member is a pointer to 
    //the event specific data, if it exists.
	  if (pEvent->MofLength && pEvent->Header.Class.Level > 0)
	  {
		DWORD dwWritten;
		FILETIME ft;
		SYSTEMTIME st;
		ft.dwHighDateTime = pEvent->Header.TimeStamp.HighPart;
		ft.dwLowDateTime = pEvent->Header.TimeStamp.LowPart;
		FileTimeToSystemTime(&ft,&st);
		TCHAR szLocalDate[255], szLocalTime[255];
		_stprintf_s(szLocalDate, ARRAYSIZE(szLocalDate),TEXT("%04d-%02d-%02d"),st.wYear,st.wMonth,st.wDay);
		_stprintf_s(szLocalTime, ARRAYSIZE(szLocalTime),TEXT("%02d:%02d:%02d"),st.wHour,st.wMinute,st.wSecond);
		WriteFile ( hInternalLogWriteHandle, szLocalDate, (DWORD)_tcslen(szLocalDate) * (DWORD)sizeof(TCHAR), &dwWritten, nullptr);
		WriteFile ( hInternalLogWriteHandle, TEXT(";"), 1 * (DWORD)sizeof(TCHAR), &dwWritten, nullptr);
		WriteFile ( hInternalLogWriteHandle, szLocalTime, (DWORD)_tcslen(szLocalTime) * (DWORD)sizeof(TCHAR), &dwWritten, nullptr);
		WriteFile ( hInternalLogWriteHandle, TEXT(";"), 1 * (DWORD)sizeof(TCHAR), &dwWritten, nullptr);
		WriteFile ( hInternalLogWriteHandle, pEvent->MofData, (DWORD)_tcslen((PTSTR) pEvent->MofData) * (DWORD)sizeof(TCHAR), &dwWritten, nullptr);
		WriteFile ( hInternalLogWriteHandle, TEXT("\r\n"), 2 * (DWORD)sizeof(TCHAR), &dwWritten, nullptr);
	  }
  }

  return;
}

void ExportOneTraceFile(PTSTR szTraceFile)
{
	ULONG rc;
	TRACEHANDLE handle = NULL;
	EVENT_TRACE_LOGFILE trace;
	memset(&trace,0, sizeof(EVENT_TRACE_LOGFILE));
	trace.LoggerName = TEXT("EIDCredentialProvider"); 
	//trace.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
	trace.LogFileName = szTraceFile;
	trace.EventCallback = (PEVENT_CALLBACK) (ProcessEvents);
	handle = OpenTrace(&trace);
	if ((TRACEHANDLE)INVALID_HANDLE_VALUE == handle)
	{
		// Handle error as appropriate for your application.
	}
	else
	{
		FILETIME now, start;
		SYSTEMTIME sysNow, sysstart;
		GetLocalTime(&sysNow);
		SystemTimeToFileTime(&sysNow, &now);
		memcpy(&sysstart, &sysNow, sizeof(SYSTEMTIME));
		sysstart.wYear -= 1;
		SystemTimeToFileTime(&sysstart, &start);
		DWORD dwWritten;
		TCHAR szBuffer[256];
		_tcscpy_s(szBuffer,ARRAYSIZE(szBuffer),TEXT("================================================\r\n"));
		WriteFile ( hInternalLogWriteHandle, szBuffer, (DWORD)_tcslen(szBuffer) * (DWORD)sizeof(TCHAR), &dwWritten, nullptr);
		WriteFile ( hInternalLogWriteHandle, szTraceFile, (DWORD)_tcslen(szTraceFile) * (DWORD)sizeof(TCHAR), &dwWritten, nullptr);
		_tcscpy_s(szBuffer,ARRAYSIZE(szBuffer),TEXT("\r\n"));
		WriteFile ( hInternalLogWriteHandle, szBuffer, (DWORD)_tcslen(szBuffer) * (DWORD)sizeof(TCHAR), &dwWritten, nullptr);
		_tcscpy_s(szBuffer,ARRAYSIZE(szBuffer),TEXT("================================================\r\n"));
		WriteFile ( hInternalLogWriteHandle, szBuffer, (DWORD)_tcslen(szBuffer) * (DWORD)sizeof(TCHAR), &dwWritten, nullptr);
		rc = ProcessTrace(&handle, 1, 0, 0);
		if (rc != ERROR_SUCCESS && rc != ERROR_CANCELLED)
		{
			if (rc ==  0x00001069)
			{
			}
			else
			{
			}
		}
		CloseTrace(handle);
	}
}

HANDLE StartReport(PTSTR szLogFile)
{
	DWORD dwError = 0;
	BOOL fSuccess = FALSE;
	HANDLE hOutput = INVALID_HANDLE_VALUE;
	__try
	{
		//  Creates the new file to write to for the upper-case version.
		hOutput = CreateFile((LPTSTR) szLogFile, // file name 
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
		// hInternalLogWriteHandle MUST be a module variable because the callback can't use any parameter
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
	TCHAR szNamedPipeName[256] = TEXT("\\\\.\\pipe\\EIDAuthenticateWizard");
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
		_stprintf_s(szParameter,ARRAYSIZE(szParameter),TEXT("REPORT %s"),szNamedPipeName);
		// launch the wizard elevated
		SHELLEXECUTEINFO shExecInfo;
		TCHAR szName[1024];
		GetModuleFileName(GetModuleHandle(nullptr),szName, ARRAYSIZE(szName));

		shExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		shExecInfo.hwnd = nullptr;
		shExecInfo.lpVerb = TEXT("runas");
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
			WriteFile(hNamedPipe,TEXT("\n"),2 * sizeof(TCHAR),nullptr,nullptr);
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
		
		// Warning : hReport is used as hInternalLogWriteHandle
		// Side Effect !!!!!!!!
		// hInternalLogWriteHandle MUST be a module variable because the callback can't use any parameter
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
		ExportOneTraceFile(TEXT("c:\\Windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl"));
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