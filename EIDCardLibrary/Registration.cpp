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
static WCHAR s_wszAuthenticationPackageName[] = L"EIDAuthenticationPackage";  // NOSONAR - GLOBAL-01: Runtime-initialized LSA state


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
	NTSTATUS Status;  // NOSONAR - EXPLICIT-TYPE-01: NTSTATUS visible for security audit
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
	NTSTATUS Status;  // NOSONAR - EXPLICIT-TYPE-01: NTSTATUS visible for security audit
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
	NTSTATUS Status;  // NOSONAR - EXPLICIT-TYPE-01: NTSTATUS visible for security audit
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
	// Register as Security Package (SSP interface)
	AppendValueToMultiSz(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Lsa", L"Security Packages", AUTHENTICATIONPACKAGENAMET);
	// Also register as Authentication Package (AP interface) for LsaLookupAuthenticationPackage
	AppendValueToMultiSz(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Lsa", L"Authentication Packages", AUTHENTICATIONPACKAGENAMET);
}

void EIDAuthenticationPackageDllUnRegister()
{
	RemoveValueFromMultiSz(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Lsa", L"Security Packages", AUTHENTICATIONPACKAGENAMET);
	RemoveValueFromMultiSz(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Lsa", L"Authentication Packages", AUTHENTICATIONPACKAGENAMET);
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

// Trace configuration registry path: HKLM\SOFTWARE\EIDAuthentication\LogManager
static const TCHAR szTraceConfigKey[] = L"SOFTWARE\\EIDAuthentication\\LogManager";

// Default values for trace configuration
static const DWORD dwDefaultLevel = 4;  // WINEVENT_LEVEL_INFO
static const DWORD dwDefaultMaxSize = 64;  // MB
static const DWORD dwDefaultFileCounter = 5;  // Number of rotated files
static const BOOL fDefaultAutoStart = FALSE;

BOOL SetTraceConfig(DWORD dwLevel, LPCWSTR szLogPath, DWORD dwMaxSizeMB, DWORD dwFileCounter, BOOL fAutoStart)
{
	HKEY hKey = nullptr;
	LONG err = 0;
	BOOL fReturn = FALSE;

	// Validate trace level (must be 1-5)
	if (dwLevel < WINEVENT_LEVEL_CRITICAL || dwLevel > WINEVENT_LEVEL_VERBOSE)
	{
		dwLevel = dwDefaultLevel;
	}

	// Validate max file size (1-1024 MB)
	if (dwMaxSizeMB < 1 || dwMaxSizeMB > 1024)
	{
		dwMaxSizeMB = dwDefaultMaxSize;
	}

	// Validate file counter (1-100)
	if (dwFileCounter < 1 || dwFileCounter > 100)
	{
		dwFileCounter = dwDefaultFileCounter;
	}

	__try
	{
		// Create or open the registry key
		err = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szTraceConfigKey, 0, nullptr,
			0, KEY_READ | KEY_WRITE, nullptr, &hKey, nullptr);
		if (err != ERROR_SUCCESS)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_ERROR, L"RegCreateKeyEx failed: 0x%08X", err);
			__leave;
		}

		// Write trace level
		err = RegSetValueEx(hKey, L"TraceLevel", 0, REG_DWORD,
			(const BYTE*)&dwLevel, sizeof(DWORD));
		if (err != ERROR_SUCCESS)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_ERROR, L"RegSetValueEx(TraceLevel) failed: 0x%08X", err);
			__leave;
		}

		// Write log file path
		if (szLogPath != nullptr && szLogPath[0] != L'\0')
		{
			err = RegSetValueEx(hKey, L"LogPath", 0, REG_SZ,
				(const BYTE*)szLogPath, (DWORD)((wcslen(szLogPath) + 1) * sizeof(WCHAR)));
			if (err != ERROR_SUCCESS)
			{
				EIDCardLibraryTrace(WINEVENT_LEVEL_ERROR, L"RegSetValueEx(LogPath) failed: 0x%08X", err);
				__leave;
			}
		}

		// Write max file size
		err = RegSetValueEx(hKey, L"MaxFileSize", 0, REG_DWORD,
			(const BYTE*)&dwMaxSizeMB, sizeof(DWORD));
		if (err != ERROR_SUCCESS)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_ERROR, L"RegSetValueEx(MaxFileSize) failed: 0x%08X", err);
			__leave;
		}

		// Write file counter
		err = RegSetValueEx(hKey, L"FileCounter", 0, REG_DWORD,
			(const BYTE*)&dwFileCounter, sizeof(DWORD));
		if (err != ERROR_SUCCESS)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_ERROR, L"RegSetValueEx(FileCounter) failed: 0x%08X", err);
			__leave;
		}

		// Write auto-start flag
		DWORD dwAutoStart = fAutoStart ? 1 : 0;
		err = RegSetValueEx(hKey, L"AutoStart", 0, REG_DWORD,
			(const BYTE*)&dwAutoStart, sizeof(DWORD));
		if (err != ERROR_SUCCESS)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_ERROR, L"RegSetValueEx(AutoStart) failed: 0x%08X", err);
			__leave;
		}

		fReturn = TRUE;
	}
	__finally
	{
		if (hKey != nullptr)
		{
			RegCloseKey(hKey);
		}
	}

	return fReturn;
}

BOOL GetTraceConfig(DWORD* pdwLevel, LPWSTR szLogPath, DWORD cchPath, DWORD* pdwMaxSizeMB, DWORD* pdwFileCounter, BOOL* pfAutoStart)
{
	HKEY hKey = nullptr;
	LONG err = 0;
	BOOL fReturn = FALSE;
	DWORD dwType = 0;
	DWORD dwSize = 0;

	// Set default values
	if (pdwLevel != nullptr) *pdwLevel = dwDefaultLevel;
	if (szLogPath != nullptr && cchPath > 0)
		wcscpy_s(szLogPath, cchPath, L"c:\\windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl");
	if (pdwMaxSizeMB != nullptr) *pdwMaxSizeMB = dwDefaultMaxSize;
	if (pdwFileCounter != nullptr) *pdwFileCounter = dwDefaultFileCounter;
	if (pfAutoStart != nullptr) *pfAutoStart = fDefaultAutoStart;

	__try
	{
		// Open the registry key
		err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTraceConfigKey, 0, KEY_READ, &hKey);
		if (err != ERROR_SUCCESS)
		{
			// Key doesn't exist yet, return defaults
			fReturn = TRUE;
			__leave;
		}

		// Read trace level
		if (pdwLevel != nullptr)
		{
			dwSize = sizeof(DWORD);
			err = RegQueryValueEx(hKey, L"TraceLevel", nullptr, &dwType,
				(LPBYTE)pdwLevel, &dwSize);
			if (err == ERROR_SUCCESS && dwType == REG_DWORD)
			{
				// Validate the read value
				if (*pdwLevel < WINEVENT_LEVEL_CRITICAL || *pdwLevel > WINEVENT_LEVEL_VERBOSE)
				{
					*pdwLevel = dwDefaultLevel;
				}
			}
			else
			{
				*pdwLevel = dwDefaultLevel;
			}
		}

		// Read log file path
		if (szLogPath != nullptr && cchPath > 0)
		{
			dwSize = cchPath * sizeof(WCHAR);
			err = RegQueryValueEx(hKey, L"LogPath", nullptr, &dwType,
				(LPBYTE)szLogPath, &dwSize);
			if (err != ERROR_SUCCESS || dwType != REG_SZ)
			{
				wcscpy_s(szLogPath, cchPath, L"c:\\windows\\system32\\LogFiles\\WMI\\EIDCredentialProvider.etl");
			}
		}

		// Read max file size
		if (pdwMaxSizeMB != nullptr)
		{
			dwSize = sizeof(DWORD);
			err = RegQueryValueEx(hKey, L"MaxFileSize", nullptr, &dwType,
				(LPBYTE)pdwMaxSizeMB, &dwSize);
			if (err != ERROR_SUCCESS || dwType != REG_DWORD)
			{
				*pdwMaxSizeMB = dwDefaultMaxSize;
			}
		}

		// Read file counter
		if (pdwFileCounter != nullptr)
		{
			dwSize = sizeof(DWORD);
			err = RegQueryValueEx(hKey, L"FileCounter", nullptr, &dwType,
				(LPBYTE)pdwFileCounter, &dwSize);
			if (err != ERROR_SUCCESS || dwType != REG_DWORD)
			{
				*pdwFileCounter = dwDefaultFileCounter;
			}
		}

		// Read auto-start flag
		if (pfAutoStart != nullptr)
		{
			DWORD dwAutoStart = 0;
			dwSize = sizeof(DWORD);
			err = RegQueryValueEx(hKey, L"AutoStart", nullptr, &dwType,
				(LPBYTE)&dwAutoStart, &dwSize);
			if (err == ERROR_SUCCESS && dwType == REG_DWORD)
			{
				*pfAutoStart = (dwAutoStart != 0);
			}
			else
			{
				*pfAutoStart = fDefaultAutoStart;
			}
		}

		fReturn = TRUE;
	}
	__finally
	{
		if (hKey != nullptr)
		{
			RegCloseKey(hKey);
		}
	}

	return fReturn;
}

BOOL EnableLogging()
{
	struct RegEntry { LPCTSTR szSubKey; LPCTSTR szValueName; DWORD dwType; const void* pData; DWORD cbData; };

	static const TCHAR szBaseKey[] = L"SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider";
	static const TCHAR szGuidKey[] = L"SYSTEM\\CurrentControlSet\\Control\\WMI\\Autologger\\EIDCredentialProvider\\{B4866A0A-DB08-4835-A26F-414B46F3244C}";
	static const TCHAR szGuidValue[] = L"{B4866A0A-DB08-4835-A26F-414B46F3244C}";

	// Read configuration from registry
	DWORD dwConfigLevel;
	DWORD dwConfigMaxSize;
	DWORD dwConfigFileCounter;
	BOOL fConfigAutoStart;
	WCHAR szConfigPath[MAX_PATH];

	GetTraceConfig(&dwConfigLevel, szConfigPath, MAX_PATH, &dwConfigMaxSize, &dwConfigFileCounter, &fConfigAutoStart);

	// Static values
	static const DWORD dw0 = 0;
	static const DWORD dw1 = 1;
	static const DWORD dw8 = 8;
	static const DWORD dw4864 = 4864;
	static const DWORD64 qw0 = 0;

	// Dynamic values from configuration
	DWORD dwEnableLevel = dwConfigLevel;
	DWORD dwMaxFileSize = dwConfigMaxSize;
	DWORD dwFileCounter = dwConfigFileCounter;
	DWORD dwStart = fConfigAutoStart ? 1 : 0;
	DWORD dwFileMax = dwConfigFileCounter + 3;  // Add buffer for FileMax

	// Use static array instead of vector to avoid C++ unwinding in SEH
	RegEntry entries[19];
	DWORD dwPathSize = (DWORD)((wcslen(szConfigPath) + 1) * sizeof(WCHAR));

	// Build registry entries dynamically based on configuration
	entries[0]  = { szBaseKey, L"Guid",            REG_SZ,    szGuidValue,   (DWORD)sizeof(szGuidValue) };
	entries[1]  = { szBaseKey, L"FileName",        REG_SZ,    szConfigPath,  dwPathSize };
	entries[2]  = { szBaseKey, L"FileMax",         REG_DWORD, &dwFileMax,     sizeof(DWORD) };
	entries[3]  = { szBaseKey, L"Start",           REG_DWORD, &dwStart,       sizeof(DWORD) };
	entries[4]  = { szBaseKey, L"BufferSize",      REG_DWORD, &dw8,           sizeof(DWORD) };
	entries[5]  = { szBaseKey, L"FlushTimer",      REG_DWORD, &dw0,           sizeof(DWORD) };
	entries[6]  = { szBaseKey, L"MaximumBuffers",  REG_DWORD, &dw0,           sizeof(DWORD) };
	entries[7]  = { szBaseKey, L"MinimumBuffers",  REG_DWORD, &dw0,           sizeof(DWORD) };
	entries[8]  = { szBaseKey, L"ClockType",       REG_DWORD, &dw1,           sizeof(DWORD) };
	entries[9]  = { szBaseKey, L"MaxFileSize",     REG_DWORD, &dwMaxFileSize, sizeof(DWORD) };
	entries[10] = { szBaseKey, L"LogFileMode",     REG_DWORD, &dw4864,        sizeof(DWORD) };
	entries[11] = { szBaseKey, L"FileCounter",     REG_DWORD, &dwFileCounter, sizeof(DWORD) };
	entries[12] = { szBaseKey, L"Status",          REG_DWORD, &dw0,           sizeof(DWORD) };
	entries[13] = { szGuidKey, L"Enabled",         REG_DWORD, &dw1,           sizeof(DWORD) };
	entries[14] = { szGuidKey, L"EnableLevel",     REG_DWORD, &dwEnableLevel, sizeof(DWORD) };
	entries[15] = { szGuidKey, L"EnableProperty",  REG_DWORD, &dw0,           sizeof(DWORD) };
	entries[16] = { szGuidKey, L"Status",          REG_DWORD, &dw0,           sizeof(DWORD) };
	entries[17] = { szGuidKey, L"MatchAllKeyword", REG_QWORD, &qw0,           sizeof(DWORD64) };
	entries[18] = { szGuidKey, L"MatchAnyKeyword", REG_QWORD, &qw0,           sizeof(DWORD64) };

	LONG err = 0;
	BOOL fReturn = FALSE;
	__try
	{
		for (int i = 0; i < 19; i++)
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
      fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(L"kernel32"),"IsWow64Process");  // NOSONAR (EXPLICIT-TYPE-04) - Explicit type preferred for code clarity
 
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