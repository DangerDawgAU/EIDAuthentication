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

#include <tchar.h>

#include "../EIDCardLibrary/CertificateValidation.h"
#include "../EIDCardLibrary/CertificateUtilities.h"
#include "../EIDCardLibrary/CContainer.h"
#include "../EIDCardLibrary/GPO.h"
#include "../EIDCardLibrary/Package.h"
#include "../EIDCardLibrary/Tracing.h"
#include <LM.h>
#include <wincred.h>


template <typename T> 
CContainerHolderFactory<T>::CContainerHolderFactory()
{
	_cpus = CPUS_INVALID;
	_dwFlags = 0;
	InitializeCriticalSection(&CriticalSection);
}

template <typename T> 
CContainerHolderFactory<T>::~CContainerHolderFactory()
{
	CleanList();
	DeleteCriticalSection(&CriticalSection);
}

template <typename T> 
VOID CContainerHolderFactory<T>::Lock()
{
	EnterCriticalSection(&CriticalSection);
}

template <typename T> 
VOID CContainerHolderFactory<T>::Unlock()
{
	LeaveCriticalSection(&CriticalSection);
}

template <typename T> 
HRESULT CContainerHolderFactory<T>::SetUsageScenario(
    CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,
    DWORD dwFlags
    )
{
	_cpus = cpus;
	_dwFlags = dwFlags;
	return S_OK;
}

template <typename T> 
BOOL CContainerHolderFactory<T>::ConnectNotification(__in LPCTSTR szReaderName,__in LPCTSTR szCardName, __in USHORT ActivityCount)
{
	return ConnectNotificationGeneric(szReaderName,szCardName, ActivityCount);
}

// called to enumerate the credential built with a CContainer
template <typename T> 
BOOL CContainerHolderFactory<T>::ConnectNotificationGeneric(__in LPCTSTR szReaderName,__in LPCTSTR szCardName, __in USHORT ActivityCount)
{
	HCRYPTPROV HCryptProv;
	HCRYPTPROV hProv = NULL;
	BOOL bStatus;
	CHAR szContainerName[1024];
	DWORD dwContainerNameLen = ARRAYSIZE(szContainerName);
	TCHAR szProviderName[1024] = TEXT("");
	DWORD dwProviderNameLen = ARRAYSIZE(szProviderName);
	DWORD pKeySpecs[2] = {AT_KEYEXCHANGE,AT_SIGNATURE};
	DWORD dwKeyNumMax = 1;
	HCRYPTKEY hKey;
	// remove existing entries
	//DisconnectNotification(szReaderName);
	EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Connect Reader %s CardName %s",szReaderName,szCardName);
	// get provider name
	if (!SchGetProviderNameFromCardName(szCardName, szProviderName, &dwProviderNameLen))
	{
		return FALSE;
	}

	LPTSTR szMainContainerName = BuildContainerNameFromReader(szReaderName);
	if (!szMainContainerName)
	{
		return FALSE;
	}
	
	// if policy 
	if (GetPolicyValue(GPOPolicy::AllowSignatureOnlyKeys) || _cpus == CPUS_INVALID)
	{
		dwKeyNumMax = 2;
	}


	bStatus = CryptAcquireContext(&HCryptProv,
				szMainContainerName,
				szProviderName,
				PROV_RSA_FULL,
				CRYPT_SILENT);
	if (!bStatus)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptAcquireContext 1 0x%08x",GetLastError());
		// for the spanish EID
		bStatus = CryptAcquireContext(&HCryptProv,
				nullptr,
				szProviderName,
				PROV_RSA_FULL,
				CRYPT_SILENT);
		if (!bStatus)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptAcquireContext 2 0x%08x",GetLastError());
			EIDFree(szMainContainerName);
			return FALSE;
		}
	}
	DWORD dwFlags = CRYPT_FIRST;
	/* Enumerate all the containers */
	while (CryptGetProvParam(HCryptProv,
				PP_ENUMCONTAINERS,
				(LPBYTE) szContainerName,
				&dwContainerNameLen,
				dwFlags)
			)
	{
		// Ensure null-termination to prevent out-of-bounds read (CWE-125 fix for #31)
		if (dwContainerNameLen >= ARRAYSIZE(szContainerName))
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING, L"Container name too long: %d", dwContainerNameLen);
			dwFlags = CRYPT_NEXT;
			dwContainerNameLen = ARRAYSIZE(szContainerName);
			continue;
		}
		szContainerName[dwContainerNameLen] = '\0';

		// convert the container name to unicode
#ifdef UNICODE
		int wLen = MultiByteToWideChar(CP_ACP, 0, szContainerName, -1, nullptr, 0);
		LPTSTR szWideContainerName = (LPTSTR) EIDAlloc(sizeof(TCHAR)*wLen);
		if (szWideContainerName)
		{
			MultiByteToWideChar(CP_ACP, 0, szContainerName, -1, szWideContainerName, wLen);
#else
		LPTSTR szWideContainerName = (LPTSTR) EIDAlloc(sizeof(TCHAR)*(_tcslen(szContainerName)+1));
		if (szWideContainerName)
			{
			_tcscpy_s(szWideContainerName,_tcslen(szContainerName)+1,szContainerName);

#endif
			// create a CContainer item
			if (CryptAcquireContext(&hProv,
				szWideContainerName,
				szProviderName,
				PROV_RSA_FULL,
				CRYPT_SILENT))
			{
				for (DWORD i = 0; i < dwKeyNumMax; i++)
				{
					if (CryptGetUserKey(hProv,
							pKeySpecs[i],
							&hKey) )
					{
						BYTE Data[4096];
						DWORD DataSize = 4096;
						if (CryptGetKeyParam(hKey,
								KP_CERTIFICATE,
								Data,
								&DataSize,
								0))
						{
							CreateItemFromCertificateBlob(hProv, szReaderName,szCardName,szProviderName,
									szWideContainerName, pKeySpecs[i],ActivityCount, Data, DataSize);
							}
							CryptDestroyKey(hKey);
							hKey = NULL;
					}
				}
			}
			CryptReleaseContext(hProv, 0);
			hProv = NULL;
		}
		else
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING, L"Failed to allocate memory for container name");
		}
		dwFlags = CRYPT_NEXT;
		dwContainerNameLen = ARRAYSIZE(szContainerName);
		EIDFree(szWideContainerName);
	}
	if (dwFlags == CRYPT_FIRST)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptGetProvParam 0x%08x - creating containers manually",GetLastError());
		// the default container can be enumerated but PP_ENUMCONTAINERS doesn't work
		for (DWORD i = 0; i < dwKeyNumMax; i++)
		{
			if (CryptGetUserKey(HCryptProv,
					pKeySpecs[i],
					&hKey) )
			{
				BYTE Data[4096];
				DWORD DataSize = 4096;
				if (CryptGetKeyParam(hKey,
						KP_CERTIFICATE,
						Data,
						&DataSize,
						0))
				{
					CreateItemFromCertificateBlob(HCryptProv, szReaderName,szCardName,szProviderName,
						szMainContainerName, pKeySpecs[i],ActivityCount, Data, DataSize);
					}
				CryptDestroyKey(hKey);
				hKey = NULL;
			}
		}
	}
	CryptReleaseContext(HCryptProv,0);
	EIDFree(szMainContainerName);
	return TRUE;
}

template <typename T>
BOOL CContainerHolderFactory<T>::CreateContainer(__in LPCTSTR szReaderName,__in LPCTSTR szCardName,
															   __in LPCTSTR szProviderName, __in LPCTSTR szWideContainerName,
															   __in DWORD KeySpec, __in USHORT ActivityCount, __in PCCERT_CONTEXT pCertContext)
{
	CContainer* pContainer = nullptr;
	pContainer = new CContainer(szReaderName,szCardName,szProviderName,szWideContainerName, KeySpec, ActivityCount, pCertContext);
	this->Lock();
	T* ContainerHolder = new T(pContainer);
	ContainerHolder->SetUsageScenario(_cpus, _dwFlags);
	_CredentialList.push_back(ContainerHolder);
	this->Unlock();
	return TRUE;
}

template <typename T>
BOOL CContainerHolderFactory<T>::CreateItemFromCertificateBlob(__in HCRYPTPROV hProv,
																__in LPCTSTR szReaderName,__in LPCTSTR szCardName,
															   __in LPCTSTR szProviderName, __in LPCTSTR szWideContainerName,
															   __in DWORD KeySpec, __in USHORT ActivityCount,
															   __in PBYTE Data, __in DWORD DataSize)
{
	BOOL fReturn = FALSE;
	PCCERT_CONTEXT pCertContext = nullptr;
	BOOL fSuccess;
	PTSTR szUsername = nullptr;
	DWORD dwError = 0;
	__try
	{
		pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING, Data, DataSize);
		if (!pCertContext)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CertCreateCertificateContext 0x%08x",dwError);
			__leave;
		}
		
		fReturn = SetupCertificateContextWithKeyInfo(pCertContext, hProv, szProviderName, szWideContainerName, KeySpec);
		if (!fReturn)
		{
			dwError = GetLastError();
			__leave;
		}
		// important : the hprov will be used later and free if the certificatecontext is free
		// so we have to add 1 to the reference count
		fReturn = CryptContextAddRef(hProv, nullptr, 0);
		if (!fReturn)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptContextAddRef 0x%08x",dwError);
			__leave;
		}

		if (_cpus != CPUS_CREDUI && _cpus != CPUS_INVALID)
		{
			fSuccess = IsTrustedCertificate(pCertContext);
			if (!fSuccess)
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Untrusted certificate IsTrustedCertificate 0x%08x",dwError);
				__leave;
			}
		}
		else if (_cpus == CPUS_CREDUI)
		{
			fSuccess = HasCertificateRightEKU(pCertContext);
			if (!fSuccess)
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Untrusted certificate HasCertificateRightEKU 0x%08x",dwError);
				__leave;
			}
		}
		// check if the Container meet the requirement
		if ((_cpus == CPUS_LOGON) || (_cpus == CPUS_UNLOCK_WORKSTATION))
		{
			// check if the user has an account to this workstation
			DWORD dwRid = LsaEIDGetRIDFromStoredCredential(pCertContext);	
			if (!dwRid)
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"HasAccountOnCurrentComputer 0x%08x",dwError);
				__leave;
			}
			szUsername = GetUsernameFromRid(dwRid);
			if (!szUsername)
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"GetUsernameFromRid 0x%08x",dwError);
				__leave;
			}
			if ((_dwFlags & CREDUIWIN_ENUMERATE_CURRENT_USER) || (_cpus == CPUS_UNLOCK_WORKSTATION))
			{
				fSuccess = IsCurrentUser(szUsername);
				if (!fSuccess)
				{
					dwError = GetLastError();
					EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"IsCurrentUser 0x%08x",dwError);
					__leave;
				}
			}
			if (_dwFlags & CREDUIWIN_ENUMERATE_ADMINS)
			{
				fSuccess = IsAdmin(szUsername);
				if (!fSuccess)
				{
					dwError = GetLastError();
					EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"IsAdmin 0x%08x",dwError);
					__leave;
				}
			}
		}
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Creating container szReaderName='%s'", szReaderName);
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Creating container szCardName='%s'", szCardName);
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Creating container szProviderName='%s'", szProviderName);
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Creating container szWideContainerName='%s' KeySpec=%d ActivityCount=%d",
				szWideContainerName, KeySpec, ActivityCount);
		fSuccess = CreateContainer(szReaderName, szCardName, szProviderName, szWideContainerName, KeySpec, ActivityCount, pCertContext);
		if (!fSuccess) __leave;
		fReturn = TRUE;
	}
	__finally
	{
		if (szUsername) EIDFree(szUsername);
		if (!fReturn && pCertContext)
		{
			CertFreeCertificateContext(pCertContext);
		}
	}
	SetLastError(dwError);
	return fReturn;
}

template <typename T>
BOOL CContainerHolderFactory<T>::DisconnectNotification(LPCTSTR szReaderName)
{
	this->Lock();
	auto l_iter = _CredentialList.begin();
	while(l_iter!=_CredentialList.end())
	{
		T* item = (T *)*l_iter;
		CContainer* container = item->GetContainer();

#ifndef UNICODE
		int wLen = MultiByteToWideChar(CP_ACP, 0, szReaderName, -1, nullptr, 0);
		LPWSTR szWideReaderName = (LPWSTR) EIDAlloc(sizeof(WCHAR)*wLen);
		if (szWideReaderName)
		{
			MultiByteToWideChar(CP_ACP, 0, szReaderName, -1, szWideReaderName, wLen);
#else
		LPWSTR szWideReaderName = (LPWSTR) EIDAlloc((DWORD)(sizeof(WCHAR)*(_tcslen(szReaderName)+1)));
		if (szWideReaderName)
			{
			_tcscpy_s(szWideReaderName,_tcslen(szReaderName)+1,szReaderName);

#endif
			if(container->IsOnReader(szWideReaderName))
			{
				l_iter = _CredentialList.erase(l_iter);
				item->Release();
			}
			else
			{
				++l_iter;
			}
			EIDFree(szWideReaderName);
		}
	}
	this->Unlock();
	return TRUE;
}

template <typename T>
BOOL CContainerHolderFactory<T>::CleanList()
{
	this->Lock();
	auto l_iter = _CredentialList.begin();
	while(l_iter!=_CredentialList.end())
	{
		T* item = (T *)*l_iter;
		l_iter = _CredentialList.erase(l_iter);
		item->Release();
	}
	this->Unlock();
	return TRUE;
}

template <typename T>
BOOL CContainerHolderFactory<T>::HasContainerHolder() const
{
	const_cast<CContainerHolderFactory<T>*>(this)->Lock();
	BOOL result = _CredentialList.size() > 0;
	const_cast<CContainerHolderFactory<T>*>(this)->Unlock();
	return result;
}


template <typename T>
DWORD CContainerHolderFactory<T>::ContainerHolderCount() const
{
	const_cast<CContainerHolderFactory<T>*>(this)->Lock();
	DWORD count = (DWORD) _CredentialList.size();
	const_cast<CContainerHolderFactory<T>*>(this)->Unlock();
	return count;
}

template <typename T>
T* CContainerHolderFactory<T>::GetContainerHolderAt(DWORD dwIndex)
{
	this->Lock();
	T* result = nullptr;
	if (dwIndex < _CredentialList.size())
	{
		auto it = _CredentialList.begin();
		std::advance(it, dwIndex);
		result = *it;
	}
	this->Unlock();
	return result;
}