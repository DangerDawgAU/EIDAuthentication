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


#pragma once
#include <iostream>
#include <list>
#include "../EIDCardLibrary/EIDCardLibrary.h"

class CContainer 
{

  public:
    CContainer(__in LPCTSTR szReaderName, __in LPCTSTR szCardName, __in LPCTSTR szProviderName, 
		__in LPCTSTR szContainerName, __in DWORD KeySpec, __in USHORT ActivityCount, __in PCCERT_CONTEXT pCertContext);

    virtual ~CContainer();

	PTSTR GetUserName();
	PTSTR GetProviderName();
	PTSTR GetContainerName();
	DWORD GetRid();
	DWORD GetKeySpec();

	PCCERT_CONTEXT GetCertificate();
	BOOL IsOnReader(__in LPCTSTR szReaderName);
	
	PEID_SMARTCARD_CSP_INFO GetCSPInfo();
	void FreeCSPInfo(PEID_SMARTCARD_CSP_INFO);

	BOOL Erase();
	BOOL ViewCertificate(HWND hWnd = NULL);

	BOOL TriggerRemovePolicy();
	PEID_INTERACTIVE_LOGON AllocateLogonStruct(PWSTR szPin, PDWORD pdwSize);
//	PEID_MSGINA_AUTHENTICATION CContainer::AllocateGinaStruct(PWSTR szPin, PDWORD pdwSize);
  private:
 static LPTSTR ValidateAndCopyString(LPCTSTR szSource, DWORD maxLength, LPCWSTR szFieldName);

 LPTSTR					_szReaderName;
 LPTSTR					_szCardName;
 LPTSTR					_szProviderName;
 LPTSTR					_szContainerName;
 LPTSTR					_szUserName;
 DWORD					_KeySpec;
 USHORT					_ActivityCount;
 PCCERT_CONTEXT			_pCertContext;
 DWORD					_dwRid;
};
