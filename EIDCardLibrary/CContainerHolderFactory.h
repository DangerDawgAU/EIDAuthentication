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
#include <wincrypt.h>
#include <credentialprovider.h>
#include <list>

template <typename T> 

class CContainerHolderFactory
{
public:	
	CContainerHolderFactory();
	virtual ~CContainerHolderFactory();

	HRESULT SetUsageScenario(__in CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,__in DWORD dwFlags);
	BOOL DisconnectNotification(__in LPCTSTR szReaderName);
	BOOL ConnectNotification(__in LPCTSTR szReaderName,__in LPCTSTR szCardName, __in USHORT ActivityCount);
	BOOL CContainerHolderFactory<T>::CreateContainer(__in LPCTSTR szReaderName,__in LPCTSTR szCardName,
															   __in LPCTSTR szProviderName, __in LPCTSTR szWideContainerName,
															   __in DWORD KeySpec, __in USHORT ActivityCount, __in PCCERT_CONTEXT pCertContext);
	BOOL CreateItemFromCertificateBlob(__in HCRYPTPROV hProv, __in LPCTSTR szReaderName,__in LPCTSTR szCardName,
															   __in LPCTSTR szProviderName, __in LPCTSTR szContainerName,
															   __in DWORD KeySpec, __in USHORT ActivityCount,
															   __in PBYTE Data, __in DWORD DataSize);
	VOID Lock();
	VOID Unlock();
	BOOL HasContainerHolder();
	DWORD ContainerHolderCount();
	T* GetContainerHolderAt(DWORD dwIndex);
private:
	BOOL ConnectNotificationGeneric(__in LPCTSTR szReaderName,__in LPCTSTR szCardName, __in USHORT ActivityCount);
	BOOL ConnectNotificationBeid(__in LPCTSTR szReaderName,__in LPCTSTR szCardName, __in USHORT ActivityCount);
	BOOL CleanList();
	CREDENTIAL_PROVIDER_USAGE_SCENARIO _cpus;
    DWORD _dwFlags;
	std::list<T*> _CredentialList;
	CRITICAL_SECTION CriticalSection;
	
};



#include "CContainerHolderFactory.cpp"