/*
    EID Authentication - Smart card authentication for Windows
    Copyright (C) 2009 Vincent Le Toux
    Copyright (C) 2026 Contributors

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/


#include <Windows.h>
#include <tchar.h>
#define SECURITY_WIN32
#include <sspi.h>

#include "../EIDCardLibrary/EIDCardLibrary.h"
#include "../EIDCardLibrary/guid.h"
#include "../EIDCardLibrary/Tracing.h"
#include "../EIDCardLibrary/StringConversion.h"
#include <string>


// Non-const string buffers for Windows API compatibility (AddSecurityPackage/DeleteSecurityPackage require LPWSTR)
static WCHAR s_wszAuthenticationPackageName[] = L"EIDAuthenticationPackage";


/** Used to append a string to a multi string reg key */
void AppendValueToMultiSz(HKEY hKey, LPCTSTR szKey, LPCTSTR szValue, LPCTSTR szData)
{
	HKEY hkResult;
	DWORD Status;
	Status=RegOpenKeyEx(hKey,szKey,0,KEY_READ|KEY_QUERY_VALUE|KEY_WRITE,&hkResult);
	if (Status != ERROR_SUCCESS) {
		MessageBoxWin32(Status);
		return;
	}
	DWORD RegType;
	DWORD RegSize;
	PTSTR Buffer = nullptr;
	PTSTR Pointer;
	RegSize = 0;
	Status = RegQueryValueEx( hkResult,szValue,nullptr,&RegType,nullptr,&RegSize);
	if (Status != ERROR_SUCCESS) {
		MessageBoxWin32(Status);
		RegCloseKey(hkResult);
		return;
	}
	RegSize += (DWORD) (_tcslen(szData) + 1 ) * sizeof(TCHAR);
	Buffer = (PTSTR) EIDAlloc(RegSize);
	if (!Buffer)
	{
		MessageBoxWin32(GetLastError());
		RegCloseKey(hkResult);
		return;
	}
	Status = RegQueryValueEx( hkResult,szValue,nullptr,&RegType,(LPBYTE)Buffer,&RegSize);
	if (Status != ERROR_SUCCESS) {
		MessageBoxWin32(Status);
		RegCloseKey(hkResult);
		EIDFree(Buffer);
		return;
	}

	char bFound = FALSE;
	Pointer = Buffer;
	while (*Pointer) 
	{
		if (_tcscmp(Pointer,szData)==0) {
			bFound = TRUE;
			break;
		}
		Pointer = Pointer + _tcslen(Pointer) + 1;
	}
	if (bFound == FALSE) {
		// add the data
		_tcscpy_s(Pointer, _tcslen(szData) + 1, szData);
		Pointer[_tcslen(szData) + 1] = 0;
		RegSize += (DWORD) (_tcslen(szData) + 1 ) * sizeof(TCHAR);
		Status = RegSetValueEx(hkResult, szValue, 0, RegType, (PBYTE) Buffer, RegSize);
		if (Status != ERROR_SUCCESS) {
			MessageBoxWin32(Status);
		}
	}
	EIDFree(Buffer);
	RegCloseKey(hkResult);
}

/** Used to Remove a string to a multi string reg key */
void RemoveValueFromMultiSz(HKEY hKey, LPCTSTR szKey, LPCTSTR szValue, LPCTSTR szData)
{
	HKEY hkResult;
	DWORD Status;
	Status=RegOpenKeyEx(hKey,szKey,0,KEY_READ|KEY_QUERY_VALUE|KEY_WRITE,&hkResult);
	if (Status != ERROR_SUCCESS) {
		MessageBoxWin32(Status);
		return;
	}
	DWORD RegType;
	DWORD RegSize;
	DWORD RegSizeOut;
	PTSTR BufferIn = nullptr;
	PTSTR BufferOut = nullptr;
	PTSTR PointerIn;
	PTSTR PointerOut;
	RegSize = 0;
	Status = RegQueryValueEx( hkResult,szValue,nullptr,&RegType,nullptr,&RegSize);
	if (Status != ERROR_SUCCESS) {
		MessageBoxWin32(Status);
		RegCloseKey(hkResult);
		return;
	}
	BufferIn = (PTSTR) EIDAlloc(RegSize);
	if (!BufferIn)
	{
		MessageBoxWin32(GetLastError());
		RegCloseKey(hkResult);
		return;
	}
	BufferOut = (PTSTR) EIDAlloc(RegSize);
	if (!BufferOut)
	{
		MessageBoxWin32(GetLastError());
		EIDFree(BufferIn);
		RegCloseKey(hkResult);
		return;
	}
	Status = RegQueryValueEx( hkResult,szValue,nullptr,&RegType,(LPBYTE)BufferIn,&RegSize);
	if (Status != ERROR_SUCCESS) {
		MessageBoxWin32(Status);
		EIDFree(BufferIn);
		EIDFree(BufferOut);
		RegCloseKey(hkResult);
		return;
	}

	PointerIn = BufferIn;
	PointerOut = BufferOut;
	RegSizeOut = 0;
	
	while (*PointerIn) 
	{
		// copy string if <> szData
		
		if (_tcscmp(PointerIn,szData)!=0) {			
			_tcscpy_s(PointerOut,(RegSize - RegSizeOut) /sizeof(TCHAR), PointerIn);
			RegSizeOut += (DWORD) (_tcslen(PointerOut) + 1) * sizeof(TCHAR);
			PointerOut += _tcslen(PointerOut) + 1;
		}
		PointerIn += _tcslen(PointerIn) + 1;
	}
	
	// last null char
	*PointerOut = 0;
	RegSizeOut += sizeof(TCHAR);
	
	Status = RegSetValueEx(hkResult, szValue, 0, RegType, (PBYTE) BufferOut, RegSizeOut);
	if (Status != ERROR_SUCCESS) {
		MessageBoxWin32(Status);
	}
	
	EIDFree(BufferIn);
	EIDFree(BufferOut);
	RegCloseKey(hkResult);
}



BOOL IsSecurityPackageLoaded(LPCTSTR szPackageName)
{
	NTSTATUS Status;
	DWORD dwNbPackage;
	PSecPkgInfo pPackageInfo;
	BOOL fFound = FALSE;

	Status = EnumerateSecurityPackages(&dwNbPackage, &pPackageInfo);
	if (Status == SEC_E_OK)
	{
		for(DWORD dwI = 0; dwI < dwNbPackage; dwI++)
		{
			PTSTR szPackage = pPackageInfo[dwI].Name;
			if (_tcscmp(szPackage, szPackageName) == 0)
			{
				fFound = TRUE;
				break;
			}
		}
		FreeContextBuffer(pPackageInfo);
	}
	return fFound;
}

BOOL RegisterTheSecurityPackage()
{
	NTSTATUS Status;
	BOOL fReturn = FALSE;
	DWORD dwError = 0;
	__try
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Starting...");
		if (IsSecurityPackageLoaded(AUTHENTICATIONPACKAGENAMET))
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"The security package was loaded before");
			dwError = ERROR_FAIL_NOACTION_REBOOT;
			__leave;
		}
		SECURITY_PACKAGE_OPTIONS options = {sizeof(SECURITY_PACKAGE_OPTIONS)};
		Status = AddSecurityPackage(s_wszAuthenticationPackageName, &options);
		if (Status != SEC_E_OK)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Unable to register the package 0x%08X 0x%08X",Status, GetLastError());
			dwError = LsaNtStatusToWinError(Status);
			__leave;
		}
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Sucessfully registered the package");
		fReturn = TRUE;
	}
	__finally
	{
		// SEH cleanup - no action needed
	}
	SetLastError(dwError);
	return fReturn;
}

BOOL UnRegisterTheSecurityPackage()
{
	NTSTATUS Status;
	BOOL fReturn = FALSE;
	DWORD dwError = 0;
	__try
	{
		// maybe use lsacallpackage to run UnloadPackage inside the SSP
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Starting...");
		if (!IsSecurityPackageLoaded(AUTHENTICATIONPACKAGENAMET))
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"The security package was not loaded before");
			fReturn = TRUE;
			__leave;
		}
		Status = DeleteSecurityPackage(s_wszAuthenticationPackageName);
		if (Status != SEC_E_OK)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Unable to unregister the package 0x%08X 0x%08X",Status, GetLastError());
			dwError = ERROR_FAIL_NOACTION_REBOOT;
			__leave;
		}
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Sucessfully unregistered the package");
		fReturn = TRUE;
	}
	__finally
	{
		// SEH cleanup - no action needed
	}
	SetLastError(dwError);
	return fReturn;
}

/** Installation and uninstallation routine
*/

void EIDAuthenticationPackageDllRegister()
{
	AppendValueToMultiSz(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Lsa", L"Security Packages", AUTHENTICATIONPACKAGENAMET);
}

void EIDAuthenticationPackageDllUnRegister()
{
	RemoveValueFromMultiSz(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Lsa", L"Security Packages", AUTHENTICATIONPACKAGENAMET);
}

void EIDPasswordChangeNotificationDllRegister()
{
	AppendValueToMultiSz(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Lsa", L"Notification Packages", L"EIDPasswordChangeNotification");
}

void EIDPasswordChangeNotificationDllUnRegister()
{
	RemoveValueFromMultiSz(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Lsa", L"Notification Packages", L"EIDPasswordChangeNotification");
}


void EIDCredentialProviderDllRegister()
{
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\Credential Providers\\{B4866A0A-DB08-4835-A26F-414B46F3244C}", 
		nullptr, REG_SZ, L"EidCredentialProvider",sizeof(L"EidCredentialProvider"));
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\Credential Provider Filters\\{B4866A0A-DB08-4835-A26F-414B46F3244C}", 
		nullptr, REG_SZ, L"EidCredentialProvider",sizeof(L"EidCredentialProvider"));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		L"CLSID\\{B4866A0A-DB08-4835-A26F-414B46F3244C}", 
		nullptr, REG_SZ, L"EidCredentialProvider",sizeof(L"EidCredentialProvider"));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		L"CLSID\\{B4866A0A-DB08-4835-A26F-414B46F3244C}\\InprocServer32",
		nullptr, REG_SZ, L"EidCredentialProvider.dll",sizeof(L"EidCredentialProvider.dll"));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		L"CLSID\\{B4866A0A-DB08-4835-A26F-414B46F3244C}\\InprocServer32",
		L"ThreadingModel",REG_SZ, L"Apartment",sizeof(L"Apartment"));
}

BOOL LsaEIDRemoveAllStoredCredential();

void EIDCredentialProviderDllUnRegister()
{
	RegDeleteTree(HKEY_CLASSES_ROOT, L"CLSID\\{B4866A0A-DB08-4835-A26F-414B46F3244C}");
	RegDeleteTree(HKEY_LOCAL_MACHINE, 
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\Credential Providers\\{B4866A0A-DB08-4835-A26F-414B46F3244C}");
	RegDeleteTree(HKEY_LOCAL_MACHINE, 
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\Credential Provider Filters\\{B4866A0A-DB08-4835-A26F-414B46F3244C}");
	LsaEIDRemoveAllStoredCredential();
}

void EIDConfigurationWizardDllRegister()
{
	RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel\\NameSpace\\{F5D846B4-14B0-11DE-B23C-27A355D89593}",
		nullptr,REG_SZ, L"EIDConfigurationWizard",sizeof(L"EIDConfigurationWizard"));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		L"CLSID\\{F5D846B4-14B0-11DE-B23C-27A355D89593}", 
		nullptr, REG_SZ, L"EIDConfigurationWizard",sizeof(L"EIDConfigurationWizard"));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		L"CLSID\\{F5D846B4-14B0-11DE-B23C-27A355D89593}",
		L"System.ApplicationName",REG_SZ, L"EID.EIDConfigurationWizard",sizeof(L"EID.EIDConfigurationWizard"));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		L"CLSID\\{F5D846B4-14B0-11DE-B23C-27A355D89593}",
		L"System.ControlPanel.Category",REG_SZ, L"10",sizeof(L"10"));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		L"CLSID\\{F5D846B4-14B0-11DE-B23C-27A355D89593}",
		L"LocalizedString",REG_EXPAND_SZ, L"Smart Card Logon",sizeof(L"Smart Card Logon"));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		L"CLSID\\{F5D846B4-14B0-11DE-B23C-27A355D89593}",
		L"InfoTip",REG_EXPAND_SZ, L"Smart Card Logon",sizeof(L"Smart Card Logon"));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		L"CLSID\\{F5D846B4-14B0-11DE-B23C-27A355D89593}\\DefaultIcon",
		nullptr,REG_EXPAND_SZ, L"%SystemRoot%\\system32\\imageres.dll,-58",
			sizeof(L"%SystemRoot%\\system32\\imageres.dll,-58"));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		L"CLSID\\{F5D846B4-14B0-11DE-B23C-27A355D89593}\\Shell\\Open\\Command",
		nullptr,REG_EXPAND_SZ, L"%SystemRoot%\\system32\\EIDConfigurationWizard.exe",
			sizeof(L"%SystemRoot%\\system32\\EIDConfigurationWizard.exe"));
	RegSetKeyValue(	HKEY_CLASSES_ROOT, 
		L"CLSID\\{F5D846B4-14B0-11DE-B23C-27A355D89593}",
		L"System.Software.TasksFileUrl",REG_SZ, L"%SystemRoot%\\system32\\EIDConfigurationWizard.exe,-68",sizeof(L"%SystemRoot%\\system32\\EIDConfigurationWizard.exe,-68"));
	

}

void EIDConfigurationWizardDllUnRegister()
{
	RegDeleteTree(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel\\NameSpace\\{F5D846B4-14B0-11DE-B23C-27A355D89593}");
	RegDeleteTree(HKEY_CLASSES_ROOT, L"CLSID\\{F5D846B4-14B0-11DE-B23C-27A355D89593}");
}

BOOL EnableLogging()
{
	struct RegEntry { LPCTSTR szSubKey; LPCTSTR szValueName; DWORD dwType; const void* pData; DWORD cbData; };

	static const TCHAR szBaseKey[] = L"SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider";
	static const TCHAR szGuidKey[] = L"SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider\\{B4866A0A-DB08-4835-A26F-414B46F3244C}";
	static const TCHAR szGuidValue[] = L"{B4866A0A-DB08-4835-A26F-414B46F3244C}";
	static const TCHAR szFileName[] = L"c:\\windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl";
	static const DWORD dw0 = 0;
	static const DWORD dw1 = 1;
	static const DWORD dw5 = 5;
	static const DWORD dw8 = 8;
	static const DWORD dw64 = 64;
	static const DWORD dw4864 = 4864;
	static const DWORD64 qw0 = 0;

	static const RegEntry entries[] = {
		{ szBaseKey, L"Guid",            REG_SZ,    szGuidValue, sizeof(szGuidValue) },
		{ szBaseKey, L"FileName",        REG_SZ,    szFileName,  sizeof(szFileName) },
		{ szBaseKey, L"FileMax",         REG_DWORD, &dw8,   sizeof(DWORD) },
		{ szBaseKey, L"Start",           REG_DWORD, &dw1,   sizeof(DWORD) },
		{ szBaseKey, L"BufferSize",      REG_DWORD, &dw8,   sizeof(DWORD) },
		{ szBaseKey, L"FlushTimer",      REG_DWORD, &dw0,   sizeof(DWORD) },
		{ szBaseKey, L"MaximumBuffers",  REG_DWORD, &dw0,   sizeof(DWORD) },
		{ szBaseKey, L"MinimumBuffers",  REG_DWORD, &dw0,   sizeof(DWORD) },
		{ szBaseKey, L"ClockType",       REG_DWORD, &dw1,   sizeof(DWORD) },
		{ szBaseKey, L"MaxFileSize",     REG_DWORD, &dw64,  sizeof(DWORD) },
		{ szBaseKey, L"LogFileMode",     REG_DWORD, &dw4864,sizeof(DWORD) },
		{ szBaseKey, L"FileCounter",     REG_DWORD, &dw5,   sizeof(DWORD) },
		{ szBaseKey, L"Status",          REG_DWORD, &dw0,   sizeof(DWORD) },
		{ szGuidKey, L"Enabled",         REG_DWORD, &dw1,   sizeof(DWORD) },
		{ szGuidKey, L"EnableLevel",     REG_DWORD, &dw5,   sizeof(DWORD) },
		{ szGuidKey, L"EnableProperty",  REG_DWORD, &dw0,   sizeof(DWORD) },
		{ szGuidKey, L"Status",          REG_DWORD, &dw0,   sizeof(DWORD) },
		{ szGuidKey, L"MatchAllKeyword", REG_QWORD, &qw0,   sizeof(DWORD64) },
		{ szGuidKey, L"MatchAnyKeyword", REG_QWORD, &qw0,   sizeof(DWORD64) },
	};

	LONG err = 0;
	BOOL fReturn = FALSE;
	__try
	{
		for (int i = 0; i < _countof(entries); i++)
		{
			err = RegSetKeyValue(HKEY_LOCAL_MACHINE, entries[i].szSubKey,
				entries[i].szValueName, entries[i].dwType, entries[i].pData, entries[i].cbData);
			if (err != ERROR_SUCCESS) __leave;
		}
		if (!StartLogging())
		{
			err = GetLastError();
			__leave;
		}
		fReturn = TRUE;
	}
	__finally
	{
		// SEH cleanup - no action needed
	}
	SetLastError(err);
	return fReturn;
}

BOOL DisableLogging()
{
	BOOL fReturn = FALSE;
	LONG lReturn = 0;
	__try
	{
		lReturn = RegDeleteTree(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider");
		if (lReturn) __leave;
		if (!StopLogging())
		{
			lReturn = GetLastError();
			__leave;
		}
		fReturn = TRUE;
	}
	__finally
	{
		// SEH cleanup - no action needed
	}
	SetLastError(lReturn);
	return fReturn;
}

BOOL IsLoggingEnabled()
{
	HKEY hkResult;
	DWORD Status;
	BOOL fReturn = FALSE;
	Status=RegOpenKeyEx(HKEY_LOCAL_MACHINE,L"SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider",0,KEY_READ|KEY_QUERY_VALUE|KEY_WRITE,&hkResult);
	if (Status == ERROR_SUCCESS) {
		fReturn = TRUE;
		RegCloseKey(hkResult);
	}
	return fReturn;
}

BOOL Is64BitOS()
{
   BOOL bIs64BitOS = FALSE;

   // We check if the OS is 64 Bit
   using LPFN_ISWOW64PROCESS = BOOL (WINAPI*)(HANDLE, PBOOL); 

   LPFN_ISWOW64PROCESS
      fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(L"kernel32"),"IsWow64Process");
 
   if (fnIsWow64Process && !fnIsWow64Process(GetCurrentProcess(),&bIs64BitOS))
   {
      //error
   }
   return bIs64BitOS;
}

void EnableCrashDump(PTSTR szPath)
{
	DWORD dwDumpType = 2;
	DWORD dwFlag = 0;
#if defined _M_IX86
	if (Is64BitOS())
	{
		dwFlag = KEY_WOW64_64KEY;
	}
#endif
	DWORD Status;
	HKEY hkResult = nullptr;
	__try
	{
		Status=RegCreateKeyEx(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\LocalDumps\\lsass.exe",
			0,nullptr,0,KEY_READ|KEY_QUERY_VALUE|KEY_WRITE|dwFlag,nullptr,&hkResult,nullptr);
		if (Status != ERROR_SUCCESS) {MessageBoxWin32(Status); __leave;}
		Status = RegSetValueEx(hkResult,L"DumpFolder",0,REG_SZ, (PBYTE) szPath,((DWORD)sizeof(TCHAR))*((DWORD)_tcslen(szPath)+1));
		if (Status != ERROR_SUCCESS) {MessageBoxWin32(Status); __leave;}
		Status = RegSetValueEx(hkResult,L"DumpType",0, REG_DWORD, (PBYTE)&dwDumpType,sizeof(dwDumpType));
		if (Status != ERROR_SUCCESS) {MessageBoxWin32(Status); __leave;}
	}
	__finally
	{
		if (hkResult)
			RegCloseKey(hkResult);
	}
}

void DisableCrashDump()
{
	HKEY hkResult;
	DWORD Status;
	DWORD dwFlag = 0;
#if defined _M_IX86
	if (Is64BitOS())
	{
		dwFlag = KEY_WOW64_64KEY;
	}
#endif
	Status=RegOpenKeyEx(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\LocalDumps",0,KEY_READ|KEY_QUERY_VALUE|KEY_WRITE|dwFlag,&hkResult);
	if (Status == ERROR_SUCCESS) {
		RegDeleteKey(hkResult, L"lsass.exe");
		RegCloseKey(hkResult);
	}
}

BOOL IsCrashDumpEnabled()
{
	HKEY hkResult;
	DWORD Status;
	DWORD dwFlag = 0;
	BOOL fReturn = FALSE;
#if defined _M_IX86
	if (Is64BitOS())
	{
		dwFlag = KEY_WOW64_64KEY;
	}
#endif
	Status=RegOpenKeyEx(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows\\Windows Error Reporting\\LocalDumps\\lsass.exe",0,KEY_READ|KEY_QUERY_VALUE|KEY_WRITE|dwFlag,&hkResult);
	if (Status == ERROR_SUCCESS) {
		fReturn = TRUE;
		RegCloseKey(hkResult);
	}
	return fReturn;
}