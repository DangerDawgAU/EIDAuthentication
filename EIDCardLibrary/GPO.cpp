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
// needed to avoid LNK2001 with the gpedit.h file (IID_IGroupPolicyObject)
#include <initguid.h>
#include <GPEdit.h>
#include "GPO.h"
#include "Tracing.h"
#include "StringConversion.h"
#include <string>

#pragma comment(lib,"Advapi32")

/** Used to manage policy key retrieval */

constexpr LPCWSTR szMainGPOKey = L"SOFTWARE\\Policies\\Microsoft\\Windows\\SmartCardCredentialProvider";
constexpr LPCWSTR szRemoveGPOKey = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";
constexpr LPCWSTR szForceGPOKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System";
struct GPOInfo
{
	LPCWSTR Key;
	LPCWSTR Value;
};

GPOInfo MyGPOInfo[] =
{
  {szMainGPOKey, L"AllowSignatureOnlyKeys" },
  {szMainGPOKey, L"AllowCertificatesWithNoEKU" },
  {szMainGPOKey, L"AllowTimeInvalidCertificates" },
  {szMainGPOKey, L"AllowIntegratedUnblock" },
  {szMainGPOKey, L"ReverseSubject" },
  {szMainGPOKey, L"X509HintsNeeded" },
  {szMainGPOKey, L"IntegratedUnblockPromptString" },
  {szMainGPOKey, L"CertPropEnabledString" },
  {szMainGPOKey, L"CertPropRootEnabledString" },
  {szMainGPOKey, L"RootsCleanupOption" },
  {szMainGPOKey, L"FilterDuplicateCertificates" },
  {szMainGPOKey, L"ForceReadingAllCertificates" },
  {szForceGPOKey, L"scforceoption" },
  {szRemoveGPOKey, L"scremoveoption" },
  {szMainGPOKey, L"EnforceCSPWhitelist" }  // Security: block CSPs not in whitelist
};

// Validates that a GPOPolicy enum value is within valid bounds to prevent array overflow
static BOOL IsValidPolicy(GPOPolicy policy)
{
	return (policy >= AllowSignatureOnlyKeys && policy <= EnforceCSPWhitelist);
}

DWORD GetPolicyValue( GPOPolicy Policy)
{
	// Validate Policy enum bounds to prevent array overflow
	if (!IsValidPolicy(Policy))
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Invalid policy index %d", (int)Policy);
		return 0;
	}
	HKEY key;
	DWORD value = 0;
	DWORD size = sizeof(DWORD);
	DWORD type=REG_SZ;
	wchar_t szValue[2] = L"0";
	DWORD size2 = sizeof(szValue);
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,MyGPOInfo[Policy].Key,0, KEY_READ, &key)==ERROR_SUCCESS){
		// scremoveoption: DWORD value stored as PTSTR
		if (Policy == scremoveoption && RegQueryValueEx(key,MyGPOInfo[Policy].Value,nullptr, &type,(LPBYTE) &szValue, &size2)==ERROR_SUCCESS)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"Policy %s found = %s",MyGPOInfo[Policy].Value,szValue);
			value = _tstoi(szValue);
		}
		else if (RegQueryValueEx(key,MyGPOInfo[Policy].Value,nullptr, &type,(LPBYTE) &value, &size)==ERROR_SUCCESS)
		{
			// Validate registry value type to prevent misinterpretation of non-DWORD data
			if (type != REG_DWORD)
			{
				value = 0;
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Policy %s has unexpected type %d, ignoring",MyGPOInfo[Policy].Value,type);
			}
			else
			{
				EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"Policy %s found = %x",MyGPOInfo[Policy].Value,value);
			}
		}
		else
		{
			value = 0;
			EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"Policy %s value not found = %x",MyGPOInfo[Policy].Value,value);
		}
		RegCloseKey(key);
	}
	else
	{
		value = 0;
		EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"Policy %s key not found = %x",MyGPOInfo[Policy].Value,value);
		
	}
	return value;
}

BOOL SetRemovePolicyValue(DWORD dwActivate)
{
	wchar_t szValue[2];
	LONG lReturn;
	DWORD dwError = 0;
	SC_HANDLE hService = nullptr;
	SC_HANDLE hServiceManager = nullptr;
	SERVICE_STATUS ServiceStatus;

	swprintf_s(szValue, ARRAYSIZE(szValue), L"%d", dwActivate);
	__try
	{
		lReturn = RegSetKeyValue(HKEY_LOCAL_MACHINE,
			MyGPOInfo[scremoveoption].Key,
			MyGPOInfo[scremoveoption].Value, REG_SZ, szValue, sizeof(wchar_t)*ARRAYSIZE(szValue));
		if ( lReturn != ERROR_SUCCESS)
		{
			dwError = lReturn;
			__leave;
		}
		hServiceManager = OpenSCManager(nullptr,nullptr,SC_MANAGER_CONNECT);
		if (!hServiceManager)
		{
			dwError = GetLastError();
			__leave;
		}
		hService = OpenService(hServiceManager, L"ScPolicySvc", SERVICE_CHANGE_CONFIG | SERVICE_START | SERVICE_STOP | SERVICE_QUERY_STATUS);
		if (!hService)
		{
			dwError = GetLastError();
			__leave;
		}
		if (dwActivate)
		{	
			// start service
			if (!ChangeServiceConfig(hService, SERVICE_NO_CHANGE, SERVICE_AUTO_START, SERVICE_NO_CHANGE, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr))
			{
				dwError = GetLastError();
				__leave;
			}
			if (!StartService(hService,0,nullptr))
			{
				dwError = GetLastError();
				__leave;
			}
		}
		else
		{ 
			// stop service
			if (!ChangeServiceConfig(hService, SERVICE_NO_CHANGE, SERVICE_DEMAND_START, SERVICE_NO_CHANGE, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr))
			{
				dwError = GetLastError();
				__leave;
			}
			if (!ControlService(hService,SERVICE_CONTROL_STOP,&ServiceStatus))
			{
				dwError = GetLastError();
				if (dwError == ERROR_SERVICE_NOT_ACTIVE)
				{
					// service not active is not an error
					dwError = 0;
				}
				__leave;
			}
			//Boucle d'attente de l'arret
			do{
				if (!QueryServiceStatus(hService,&ServiceStatus))
				{
					dwError = GetLastError();
					__leave;
				}
				Sleep(100);
			} while(ServiceStatus.dwCurrentState != SERVICE_STOPPED); 
		}
	}
	__finally
	{
		if (hService)
			CloseServiceHandle(hService);
		if (hServiceManager)
			CloseServiceHandle(hServiceManager);
	}
	return dwError == 0;
}

BOOL SetPolicyValue(GPOPolicy Policy, DWORD dwValue)
{
	// Validate Policy enum bounds to prevent array overflow
	if (!IsValidPolicy(Policy))
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Invalid policy index %d", (int)Policy);
		return FALSE;
	}
	BOOL fReturn = FALSE;
	if (Policy == scremoveoption)
	{
		// special case because a service has to be configured and the value is stored as string instead of DWORD
		fReturn = SetRemovePolicyValue(dwValue);
	}
	else
	{
		fReturn = RegSetKeyValue(	HKEY_LOCAL_MACHINE, 
				MyGPOInfo[Policy].Key,
				MyGPOInfo[Policy].Value, REG_DWORD, &dwValue,sizeof(dwValue));
	}
	return fReturn;
}