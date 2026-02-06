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


#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <tchar.h>
#define SECURITY_WIN32
#include <sspi.h>

#include <shlobj.h>
#include <ntsecapi.h>
#include <lm.h>

#include <ntsecpkg.h>
#include <strsafe.h>

#include "EidCardLibrary.h"
#include "Tracing.h"
#include "CertificateValidation.h"

constexpr LPCTSTR CREDENTIALPROVIDER = MS_ENH_RSA_AES_PROV;
constexpr DWORD CREDENTIALKEYLENGTH = 256;
constexpr ALG_ID CREDENTIALCRYPTALG = CALG_AES_256;
constexpr LPCWSTR CREDENTIAL_LSAPREFIX = L"L$_EID_";
constexpr LPCTSTR CREDENTIAL_CONTAINER = TEXT("EIDCredential");

#pragma comment(lib,"Crypt32")
#pragma comment(lib,"advapi32")
#pragma comment(lib,"Netapi32")



extern "C"
{
	NTSTATUS WINAPI SystemFunction007 (PUNICODE_STRING string, LPBYTE hash);
}

// level 1
#include "StoredCredentialManagement.h"
CStoredCredentialManager *CStoredCredentialManager::theSingleInstance = nullptr;

BOOL CStoredCredentialManager::GetUsernameFromCertContext(__in PCCERT_CONTEXT pContext, __out PWSTR *pszUsername, __out PDWORD pdwRid)
{
	NET_API_STATUS Status;
	PUSER_INFO_3 pUserInfo = nullptr;
	DWORD dwEntriesRead = 0, dwTotalEntries = 0;
	BOOL fReturn = FALSE;
	PEID_PRIVATE_DATA pPrivateData = nullptr;
	DWORD dwError = 0;
	__try
	{
		if (!pContext)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"ppContext null");
			dwError = ERROR_INVALID_PARAMETER;
			__leave;
		}
		if (!pszUsername)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"pszUsername null");
			dwError = ERROR_INVALID_PARAMETER;
			__leave;
		}
		if (!pdwRid)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"pdwRid null");
			dwError = ERROR_INVALID_PARAMETER;
			__leave;
		}
		*pdwRid = 0;
		Status = NetUserEnum(nullptr, 3,0, (PBYTE*) &pUserInfo, MAX_PREFERRED_LENGTH, &dwEntriesRead, &dwTotalEntries, nullptr);
		if (Status != NERR_Success)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"NetUserEnum 0x%08x",Status);
			dwError = Status;
			__leave;
		}
		for (DWORD dwI = 0; dwI < dwEntriesRead; dwI++)
		{
			// for each credential
			if (RetrievePrivateData(pUserInfo[dwI].usri3_user_id, &pPrivateData))
			{
				if (pPrivateData->dwCertificatSize == pContext->cbCertEncoded)
				{
					if (memcmp(pPrivateData->Data + pPrivateData->dwCertificatOffset, pContext->pbCertEncoded, pContext->cbCertEncoded) == 0)
					{
						// found
						*pdwRid = pUserInfo[dwI].usri3_user_id;
						PWSTR Username = pUserInfo[dwI].usri3_name;
						*pszUsername = (PWSTR) EIDAlloc((DWORD)(wcslen(Username) +1) * sizeof(WCHAR));
						
						if (*pszUsername)
						{
							wcscpy_s(*pszUsername, wcslen(Username) +1, Username);
							EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Found 0x%x %s",*pdwRid, *pszUsername);
							fReturn = TRUE;
						}
						else
						{
							dwError = GetLastError();
							EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CertCreateCertificateContext 0x%08x",dwError);
						}
						EIDFree(pPrivateData);
						break;
					}
					else
					{
						EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"%d don't match", pUserInfo[dwI].usri3_user_id);
					}
				}
				EIDFree(pPrivateData);
			}
		}
		if (!fReturn)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Not found");
		}
	}
	__finally
	{
		if (pUserInfo)
			NetApiBufferFree(pUserInfo);
	}
	SetLastError(dwError);
	return fReturn;
}

BOOL CStoredCredentialManager::HasStoredCredential(__in PCCERT_CONTEXT pContext)
{
	DWORD dwRid;
	PWSTR szUsername = nullptr;
	if (GetUsernameFromCertContext(pContext, &szUsername, &dwRid))
	{
		EIDFree(szUsername);
		return TRUE;
	}
	return FALSE;
}

BOOL CStoredCredentialManager::GetCertContextFromHash(__in PBYTE pbHash, __out PCCERT_CONTEXT* ppContext, __out PDWORD pdwRid)
{
	NET_API_STATUS Status;
	PUSER_INFO_3 pUserInfo = nullptr;
	DWORD dwEntriesRead = 0, dwTotalEntries = 0;
	BOOL fReturn = FALSE;
	PEID_PRIVATE_DATA pPrivateData = nullptr;
	DWORD dwError = 0;
	__try
	{
		if (!ppContext)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"ppContext null");
			dwError = ERROR_INVALID_PARAMETER;
			__leave;
		}
		if (!pdwRid)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"pdwRid null");
			dwError = ERROR_INVALID_PARAMETER;
			__leave;
		}
		Status = NetUserEnum(nullptr, 3,0, (PBYTE*) &pUserInfo, MAX_PREFERRED_LENGTH, &dwEntriesRead, &dwTotalEntries, nullptr);
		if (Status != NERR_Success)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"NetUserEnum 0x%08x",Status);
			dwError = Status;
			__leave;
		}
		for (DWORD dwI = 0; dwI < dwEntriesRead; dwI++)
		{
			// for each credential
			if (RetrievePrivateData(pUserInfo[dwI].usri3_user_id, &pPrivateData))
			{
				if (memcmp(pPrivateData->Hash, pbHash, CERT_HASH_LENGTH) == 0)
				{
					// found
					*pdwRid = pUserInfo[dwI].usri3_user_id;
					*ppContext = CertCreateCertificateContext(X509_ASN_ENCODING,
								pPrivateData->Data + pPrivateData->dwCertificatOffset, pPrivateData->dwCertificatSize);
					if (*ppContext)
					{
						fReturn = TRUE;
					}
					else
					{
						dwError = GetLastError();
						EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CertCreateCertificateContext 0x%08x",dwError);
					}
					EIDFree(pPrivateData);
					break;
				}
				EIDFree(pPrivateData);
			}
		}
	}
	__finally
	{
		if (pUserInfo)
			NetApiBufferFree(pUserInfo);
	}
	SetLastError(dwError);
	return fReturn;
}

BOOL CStoredCredentialManager::GetCertContextFromRid(__in DWORD dwRid, __out PCCERT_CONTEXT* ppContext, __out PBOOL pfEncryptPassword)
{
	BOOL fReturn = FALSE, fStatus;
	PEID_PRIVATE_DATA pEidPrivateData = nullptr;
	DWORD dwError = 0;
	__try
	{
		if (!dwRid)
		{
			dwError = ERROR_NONE_MAPPED;
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"dwRid 0x%08x",dwError);
			__leave;
		}
		if (!ppContext)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"ppContext null");
			dwError = ERROR_INVALID_PARAMETER;
			__leave;
		}
		if (!pfEncryptPassword)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"fEncryptPassword null");
			dwError = ERROR_INVALID_PARAMETER;
			__leave;
		}
		fStatus = RetrievePrivateData(dwRid,&pEidPrivateData);
		if (!fStatus)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"RetrievePrivateData 0x%08x",dwError);
			__leave;
		}
		*ppContext = CertCreateCertificateContext(X509_ASN_ENCODING, 
						pEidPrivateData->Data + pEidPrivateData->dwCertificatOffset,
						pEidPrivateData->dwCertificatSize);
		if (!*ppContext) 
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CertCreateCertificateContext 0x%08x",dwError);
			__leave;
		}
		*pfEncryptPassword = (pEidPrivateData->dwType == eidpdtCrypted);
		fReturn = TRUE;
	}
	__finally
	{
		if (!fReturn)
		{
			CertFreeCertificateContext(*ppContext);
			*ppContext = nullptr;
		}
		if (pEidPrivateData)
		{
			EIDFree(pEidPrivateData);
		}
	}
	SetLastError(dwError);
	return fReturn;
}

BOOL CStoredCredentialManager::CreateCredential(__in DWORD dwRid, __in PCCERT_CONTEXT pCertContext, __in PWSTR szPassword, __in_opt USHORT usPasswordLen, __in BOOL fEncryptPassword, __in BOOL fCheckPassword)
{
	BOOL fReturn = FALSE, fStatus;
	DWORD dwError = 0;
	NTSTATUS Status;
	HCRYPTKEY hKey = NULL;
	HCRYPTKEY hSymetricKey = NULL;
	PBYTE pSymetricKey = nullptr;
	USHORT usSymetricKeySize = 0;
	PBYTE pEncryptedPassword = nullptr;
	USHORT usEncryptedPasswordSize = 0;
	PEID_PRIVATE_DATA pbSecret = nullptr;
	USHORT usSecretSize;
	USHORT usPasswordSize;
	HCRYPTPROV hProv = NULL;
	HCRYPTHASH hHash = NULL;
	PBYTE pbPublicKey = nullptr;
	DWORD dwSize = 0;
	__try
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Enter fEncryptPassword = %d",fEncryptPassword);
		// check password
		if (!dwRid)
		{
			dwError = ERROR_NONE_MAPPED;
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"dwRid 0x%08x",dwError);
			__leave;
		}
		if (fCheckPassword)
		{
			Status = CheckPassword(dwRid, szPassword);
			if (Status != STATUS_SUCCESS)
			{
				dwError = LsaNtStatusToWinError(Status);
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CheckPassword 0x%08x",dwError);
				__leave;
			}
		}
		if (usPasswordLen > 0)
		{
			usPasswordSize = usPasswordLen;
		}
		else if (szPassword == nullptr)
		{
			usPasswordSize = 0;
		}
		else
		{
			usPasswordSize = (USHORT) (wcslen(szPassword) * sizeof(WCHAR));
		}
		
		if (!usPasswordSize) fEncryptPassword = FALSE;
		if (fEncryptPassword)
		{
			fStatus = CryptDecodeObject(X509_ASN_ENCODING, RSA_CSP_PUBLICKEYBLOB, 
				pCertContext->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
				pCertContext->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData,
				0, nullptr, &dwSize);
			if (!fStatus)
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptDecodeObject 0x%08x",GetLastError());
				__leave;
			}
			pbPublicKey = (PBYTE) EIDAlloc(dwSize);
			if (!pbPublicKey)
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"EIDAlloc 0x%08x",GetLastError());
				__leave;
			}
			fStatus = CryptDecodeObject(X509_ASN_ENCODING, RSA_CSP_PUBLICKEYBLOB, 
				pCertContext->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
				pCertContext->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData,
				0, pbPublicKey, &dwSize);
			if (!fStatus)
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptDecodeObject 0x%08x",GetLastError());
				__leave;
			}
						// import the public key into hKey
			fStatus = CryptAcquireContext(&hProv,CREDENTIAL_CONTAINER,CREDENTIALPROVIDER,PROV_RSA_AES,0);
			if(!fStatus)
			{
				dwError = GetLastError();
				if (dwError == NTE_BAD_KEYSET)
				{
					fStatus = CryptAcquireContext(&hProv,CREDENTIAL_CONTAINER,CREDENTIALPROVIDER,PROV_RSA_AES,CRYPT_NEWKEYSET);
					dwError = GetLastError();
				}
				if (!fStatus)
				{
					dwError = GetLastError();
					EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptAcquireContext 0x%08x",dwError);
					__leave;
				}
			}
			else
			{
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Container already existed !!");
			}
			fStatus = CryptImportKey(hProv, pbPublicKey, dwSize, NULL, 0, &hKey);
			if (!fStatus)
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptImportKey 0x%08x",GetLastError());
				__leave;
			}
			// create a symetric key which can be used to crypt data and
			// which is saved and protected by the public key
			fStatus = GenerateSymetricKeyAndEncryptIt(hProv, hKey, &hSymetricKey, &pSymetricKey, &usSymetricKeySize);
			if(!fStatus)
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"GenerateSymetricKeyAndSaveIt");
				__leave;
			}
			// encrypt the password and save it
			fStatus = EncryptPasswordAndSaveIt(hSymetricKey,szPassword,usPasswordLen, &pEncryptedPassword, &usEncryptedPasswordSize);
			if(!fStatus)
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"EncryptPasswordAndSaveIt");
				__leave;
			}
			usSecretSize = (USHORT) sizeof(EID_PRIVATE_DATA) + usEncryptedPasswordSize + usSymetricKeySize + (USHORT) pCertContext->cbCertEncoded;
			pbSecret = (PEID_PRIVATE_DATA) EIDAlloc(usSecretSize);
			if (!pbSecret)
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%08x returned by EIDAlloc", GetLastError());
				__leave;
			}
			DWORD dwHashSize = CERT_HASH_LENGTH;  // SHA-256 = 32 bytes
			fStatus = CryptHashCertificate(NULL, CALG_SHA_256, 0, pCertContext->pbCertEncoded, pCertContext->cbCertEncoded, pbSecret->Hash, &dwHashSize);
			if (!fStatus)
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptHashCertificate 0x%08x",GetLastError());
				__leave;
			}

			// copy data
			pbSecret->dwType = eidpdtCrypted;
			pbSecret->dwCertificatSize = (USHORT) pCertContext->cbCertEncoded;
			pbSecret->dwSymetricKeySize = usSymetricKeySize;
			pbSecret->dwPasswordSize = usEncryptedPasswordSize;
			pbSecret->dwCertificatOffset = 0;
			memcpy(pbSecret->Data + pbSecret->dwCertificatOffset, pCertContext->pbCertEncoded, pbSecret->dwCertificatSize);
			pbSecret->dwSymetricKeyOffset = pbSecret->dwCertificatOffset + pbSecret->dwCertificatSize;
			memcpy(pbSecret->Data + pbSecret->dwSymetricKeyOffset, pSymetricKey, pbSecret->dwSymetricKeySize);
			pbSecret->dwPasswordOffset = pbSecret->dwSymetricKeyOffset + usSymetricKeySize;
			memcpy(pbSecret->Data + pbSecret->dwPasswordOffset, pEncryptedPassword, pbSecret->dwPasswordSize);
		}
		else
		{
			// Use DPAPI to encrypt password when certificate-based encryption is not available
			// (e.g., AT_SIGNATURE keys that cannot perform encryption)
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE, L"Using DPAPI encryption for credential storage");

			DATA_BLOB DataIn;
			DATA_BLOB DataOut;
			DataIn.pbData = (BYTE*)szPassword;
			DataIn.cbData = usPasswordSize;

			if (!CryptProtectData(&DataIn, L"EID Credential", nullptr, nullptr, nullptr, CRYPTPROTECT_LOCAL_MACHINE, &DataOut))
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING, L"CryptProtectData failed 0x%08x", dwError);
				__leave;
			}

			usSecretSize = (USHORT)(sizeof(EID_PRIVATE_DATA) + DataOut.cbData + (USHORT)pCertContext->cbCertEncoded);
			pbSecret = (PEID_PRIVATE_DATA)EIDAlloc(usSecretSize);
			if (!pbSecret)
			{
				LocalFree(DataOut.pbData);
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING, L"Error 0x%08x returned by EIDAlloc", GetLastError());
				__leave;
			}

			DWORD dwHashSize = CERT_HASH_LENGTH;
			fStatus = CryptHashCertificate(NULL, CALG_SHA_256, 0, pCertContext->pbCertEncoded, pCertContext->cbCertEncoded, pbSecret->Hash, &dwHashSize);
			if (!fStatus)
			{
				LocalFree(DataOut.pbData);
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING, L"CryptHashCertificate 0x%08x", GetLastError());
				__leave;
			}

			// copy data
			pbSecret->dwType = eidpdtDPAPI;
			pbSecret->dwCertificatSize = (USHORT)pCertContext->cbCertEncoded;
			pbSecret->dwSymetricKeySize = 0;  // Not used for DPAPI
			pbSecret->dwPasswordSize = (USHORT)DataOut.cbData;
			pbSecret->dwCertificatOffset = 0;
			memcpy(pbSecret->Data + pbSecret->dwCertificatOffset, pCertContext->pbCertEncoded, pbSecret->dwCertificatSize);
			pbSecret->dwSymetricKeyOffset = pbSecret->dwCertificatOffset + pbSecret->dwCertificatSize;
			pbSecret->dwPasswordOffset = pbSecret->dwSymetricKeyOffset;  // No symmetric key, so password immediately follows cert
			memcpy(pbSecret->Data + pbSecret->dwPasswordOffset, DataOut.pbData, pbSecret->dwPasswordSize);

			LocalFree(DataOut.pbData);
		}
		// save the data
		if (!StorePrivateData(dwRid, (PBYTE) pbSecret, usSecretSize))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"StorePrivateData");
			__leave;
		}
		fReturn = TRUE;
	}
	__finally
	{
		if (pbPublicKey)
			EIDFree(pbPublicKey);
		if (hHash)
			CryptDestroyHash(hHash);
		if (pSymetricKey)
		{
			SecureZeroMemory(pSymetricKey, usSymetricKeySize);
			EIDFree(pSymetricKey);
		}
		if (pEncryptedPassword)
		{
			SecureZeroMemory(pEncryptedPassword, usEncryptedPasswordSize);
			EIDFree(pEncryptedPassword);
		}
		if (pbSecret)
		{
			SecureZeroMemory(pbSecret, usSecretSize);
			EIDFree(pbSecret);
		}
		if (hSymetricKey)
			CryptDestroyKey(hSymetricKey);
		if (hKey)
			CryptDestroyKey(hKey);
		if (hProv)
		{
			CryptReleaseContext(hProv, 0);
			CryptAcquireContext(&hProv,CREDENTIAL_CONTAINER,CREDENTIALPROVIDER,PROV_RSA_AES,CRYPT_DELETEKEYSET);
		}
	}
	SetLastError(dwError);
	return fReturn;
}

BOOL CStoredCredentialManager::UpdateCredential(__in PLUID pLuid, __in PUNICODE_STRING Password)
{
	DWORD dwRid = 0;
	WCHAR szComputer[UNLEN+1];
	WCHAR szUser[256];
	DWORD dwSize = ARRAYSIZE(szComputer);
	DWORD dwError = 0;
	BOOL fReturn = FALSE;
	USER_INFO_3* pUserInfo = nullptr;
	PSECURITY_LOGON_SESSION_DATA pLogonSessionData = nullptr;
	NTSTATUS status;
	__try
	{
		status = LsaGetLogonSessionData(pLuid, &pLogonSessionData);
		if (status != STATUS_SUCCESS)
		{
			dwError = LsaNtStatusToWinError(status);
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"LsaGetLogonSessionData 0x%08x",status);
			__leave;
		}
		GetComputerName(szComputer,&dwSize);
		if (!((pLogonSessionData->LogonDomain.Length == dwSize * sizeof(WCHAR)
			&& memcmp(pLogonSessionData->LogonDomain.Buffer,szComputer, dwSize * sizeof(WCHAR)) == 0)))
		{
			dwError = ERROR_NONE_MAPPED;
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"not a local account '%wZ'", &(pLogonSessionData->LogonDomain));
			__leave;
		}
		// get the user ID (RID)
		PUNICODE_STRING UserName = &(pLogonSessionData->UserName);
		if (!UserName)
		{
			dwError = ERROR_NONE_MAPPED;
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"UserName null");
			__leave;
		}
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"using userName '%wZ'", UserName);

		// SECURITY FIX: Validate UserName->Length before memcpy to prevent buffer overflow (CWE-120)
		if (UserName->Buffer && UserName->Length)
		{
			if (UserName->Length >= sizeof(szUser))
			{
				dwError = ERROR_BUFFER_OVERFLOW;
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING, L"UserName too long: %d bytes (max %d)", UserName->Length, (int)(sizeof(szUser) - sizeof(WCHAR)));
				__leave;
			}
			memcpy(szUser, UserName->Buffer, UserName->Length);
		}
		szUser[UserName->Length/2] = L'\0';
		dwError = NetUserGetInfo(szComputer, szUser, 3, (LPBYTE*) &pUserInfo);
		if (NERR_Success != dwError)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"NetUserEnum 0x%08x",dwError);
			__leave;
		}
		dwRid = pUserInfo->usri3_user_id;
		if (!UpdateCredential(dwRid, (Password->Length > 0 ? Password->Buffer:  nullptr), Password->Length))
		{
			dwError = GetLastError();
			__leave;
		}
		fReturn = TRUE;
	}
	__finally
	{
		if (pUserInfo) NetApiBufferFree(pUserInfo);
		if (pLogonSessionData) LsaFreeReturnBuffer(pLogonSessionData);
	}
	SetLastError(dwError);
	return fReturn;
}

BOOL CStoredCredentialManager::UpdateCredential(__in DWORD dwRid, __in PWSTR szPassword, __in_opt USHORT usPasswordLen)
{
	BOOL fReturn = FALSE, fStatus;
	DWORD dwError = 0;
	PCCERT_CONTEXT pCertContext = nullptr;
	BOOL fEncrypt;
	__try
	{
		if (!dwRid)
		{
			dwError = ERROR_NONE_MAPPED;
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"dwRid 0x%08x",dwError);
			__leave;
		}
		fStatus = GetCertContextFromRid(dwRid, &pCertContext, &fEncrypt);
		if (!fStatus)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"GetCertContextFromRid 0x%08x",dwError);
			__leave;
		}
		fStatus = CreateCredential(dwRid, pCertContext, szPassword, usPasswordLen, fEncrypt, FALSE);
		if (!fStatus)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CreateCredential 0x%08x",dwError);
			__leave;
		}
		fReturn = TRUE;
	}
	__finally
	{
		// SECURITY FIX: Free certificate context to prevent memory leak (CWE-401 fix for #26)
		if (pCertContext)
			CertFreeCertificateContext(pCertContext);
	}
	SetLastError(dwError);
	return fReturn;
}
BOOL CStoredCredentialManager::GetChallenge(__in DWORD dwRid, __out PBYTE* ppChallenge, __out PDWORD pdwChallengeSize, __out PDWORD pType)
{
	BOOL fReturn = FALSE, fStatus;
	DWORD dwError = 0;
	PEID_PRIVATE_DATA pEidPrivateData = nullptr;
	HCRYPTPROV hProv = NULL;  // Windows handle type - keep as NULL
	__try
	{
		if (!dwRid)
		{
			dwError = ERROR_NONE_MAPPED;
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"dwRid 0x%08x",dwError);
			__leave;
		}
		if (!ppChallenge)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"ppChallenge null");
			dwError = ERROR_INVALID_PARAMETER;
			__leave;
		}
		fStatus = RetrievePrivateData(dwRid,&pEidPrivateData);
		if (!fStatus)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"RetrievePrivateData 0x%08x",dwError);
			__leave;
		}
		*pType = pEidPrivateData->dwType;
		switch(*pType)
		{
		case eidpdtCrypted:
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"dwType = eidpdtCrypted");
			*pdwChallengeSize = pEidPrivateData->dwSymetricKeySize;
			*ppChallenge = (PBYTE) EIDAlloc(pEidPrivateData->dwSymetricKeySize);
			if (*ppChallenge == nullptr)
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"EIDAlloc 0x%08x",dwError);
				__leave;
			}
			memcpy(*ppChallenge, pEidPrivateData->Data + pEidPrivateData->dwSymetricKeyOffset, pEidPrivateData->dwSymetricKeySize); 
			break;
		case eidpdtClearText:
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"dwType = eidpdtClearText");
			fStatus = GetSignatureChallenge(ppChallenge, pdwChallengeSize);
			if (!fStatus)
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"GetSignatureChallenge 0x%08x",dwError);
				__leave;
			}
			break;
		case eidpdtDPAPI:
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"dwType = eidpdtDPAPI");
			fStatus = GetSignatureChallenge(ppChallenge, pdwChallengeSize);
			if (!fStatus)
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"GetSignatureChallenge 0x%08x",dwError);
				__leave;
			}
			break;
		default:
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"dwType not implemented");
			__leave;
		}
		fReturn = TRUE;
	}
	__finally
	{
		if (pEidPrivateData)
			EIDFree(pEidPrivateData);
		if (hProv)
		{
			CryptReleaseContext(hProv, 0);
			CryptAcquireContext(&hProv,CREDENTIAL_CONTAINER,CREDENTIALPROVIDER,PROV_RSA_AES,CRYPT_DELETEKEYSET);
		}
	}
	SetLastError(dwError);
	return fReturn;
}

BOOL CStoredCredentialManager::GetSignatureChallenge(__out PBYTE* ppChallenge, __out PDWORD pdwChallengeSize)
{
	BOOL fReturn = FALSE, fStatus;
	HCRYPTPROV hProv = NULL;  // Windows handle type - keep as NULL
	DWORD dwError = 0;
	__try
	{
		fStatus = CryptAcquireContext(&hProv,CREDENTIAL_CONTAINER,CREDENTIALPROVIDER,PROV_RSA_AES,0);
		if(!fStatus)
		{
			dwError = GetLastError();
			if (dwError == NTE_BAD_KEYSET)
			{
				fStatus = CryptAcquireContext(&hProv,CREDENTIAL_CONTAINER,CREDENTIALPROVIDER,PROV_RSA_AES,CRYPT_NEWKEYSET);
				dwError = GetLastError();
			}
			if (!fStatus)
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptAcquireContext 0x%08x",dwError);
				__leave;
			}
		}
		else
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Container already existed !!");
		}
		*pdwChallengeSize = CREDENTIALKEYLENGTH;
		*ppChallenge = (PBYTE) EIDAlloc(CREDENTIALKEYLENGTH);
		if (*ppChallenge == nullptr)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"EIDAlloc 0x%08x",dwError);
			__leave;
		}
		fStatus = CryptGenRandom(hProv, CREDENTIALKEYLENGTH, *ppChallenge);
		if (!fStatus)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptGenRandom 0x%08x",dwError);
			__leave;
		}
		fReturn = TRUE;
	}
	__finally
	{

		if (hProv)
		{
			CryptReleaseContext(hProv, 0);
			CryptAcquireContext(&hProv,CREDENTIAL_CONTAINER,CREDENTIALPROVIDER,PROV_RSA_AES,CRYPT_DELETEKEYSET);
		}
	}
	SetLastError(dwError);
	return fReturn;
}
BOOL CStoredCredentialManager::RemoveStoredCredential(__in DWORD dwRid)
{
	return StorePrivateData(dwRid, nullptr, 0);
}
BOOL CStoredCredentialManager::RemoveAllStoredCredential()
{
	NET_API_STATUS Status;
	PUSER_INFO_3 pUserInfo = nullptr;
	DWORD dwEntriesRead = 0, dwTotalEntries = 0;
	BOOL fReturn = FALSE;
	__try
	{
		Status = NetUserEnum(nullptr, 3,0, (PBYTE*) &pUserInfo, MAX_PREFERRED_LENGTH, &dwEntriesRead, &dwTotalEntries, nullptr);
		if (Status != NERR_Success)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"NetUserEnum 0x%08x",Status);
			SetLastError(Status);
			__leave;
		}
		for (DWORD dwI = 0; dwI < dwEntriesRead; dwI++)
		{
			RemoveStoredCredential(pUserInfo[dwI].usri3_user_id);
		}
		fReturn = TRUE;
	}
	__finally
	{
		if (pUserInfo)
			NetApiBufferFree(pUserInfo);
	}
	return fReturn;
}

BOOL CStoredCredentialManager::GetPassword(__in DWORD dwRid, __in PCCERT_CONTEXT pContext, __in PWSTR szPin, __out PWSTR *pszPassword)
{
	BOOL fReturn = FALSE, fStatus;
	PBYTE pChallenge = nullptr;
	PBYTE pResponse = nullptr;
	DWORD dwResponseSize = 0, dwChallengeSize = 0;
	DWORD dwError = 0;
	DWORD type;
	__try
	{
		if (!dwRid)
		{
			dwError = ERROR_NONE_MAPPED;
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"dwRid 0x%08x",dwError);
			__leave;
		}
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"GetChallenge");
		fStatus = GetChallenge(dwRid, &pChallenge, &dwChallengeSize, &type);
		if (!fStatus)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"GetChallenge 0x%08x",dwError);
			__leave;
		}
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"GetResponseFromChallenge");
		EIDImpersonate();
		fStatus = GetResponseFromChallenge(pChallenge, dwChallengeSize, type, pContext, szPin, &pResponse, &dwResponseSize);
		EIDRevertToSelf();
		if (!fStatus)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"GetResponseFromChallenge 0x%08x",dwError);
			__leave;
		}
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"GetPasswordFromChallengeResponse");
		fStatus = GetPasswordFromChallengeResponse(dwRid, pChallenge, dwChallengeSize, type, pResponse, dwResponseSize, pszPassword);
		if (!fStatus)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"GetPasswordFromChallengeResponse 0x%08x",dwError);
			__leave;
		}
		fReturn = TRUE;
	}
	__finally
	{
		if (pChallenge)
		{
			SecureZeroMemory(pChallenge, dwChallengeSize);
			EIDFree(pChallenge);
		}
		if (pResponse)
		{
			SecureZeroMemory(pResponse, dwResponseSize);
			EIDFree(pResponse);
		}
	}
	SetLastError(dwError);
	return fReturn;
}
// level 2
////////////////////////////////////////////////////////////////////////////////
// LEVEL 1
////////////////////////////////////////////////////////////////////////////////

NTSTATUS CompletePrimaryCredential(__in PLSA_UNICODE_STRING AuthenticatingAuthority,
						__in PLSA_UNICODE_STRING AccountName,
						__in PSID UserSid,
						__in PLUID LogonId,
						__in PWSTR szPassword,
						__out  PSECPKG_PRIMARY_CRED PrimaryCredentials)
{

	EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Enter");
	memset(PrimaryCredentials, 0, sizeof(SECPKG_PRIMARY_CRED));
	PrimaryCredentials->LogonId.HighPart = LogonId->HighPart;
	PrimaryCredentials->LogonId.LowPart = LogonId->LowPart;

	PrimaryCredentials->DownlevelName.Length = AccountName->Length;
	PrimaryCredentials->DownlevelName.MaximumLength = AccountName->MaximumLength;
	PrimaryCredentials->DownlevelName.Buffer = (PWSTR) EIDAlloc(AccountName->MaximumLength);
	memcpy(PrimaryCredentials->DownlevelName.Buffer, AccountName->Buffer, AccountName->MaximumLength);

	PrimaryCredentials->DomainName.Length = AuthenticatingAuthority->Length;
	PrimaryCredentials->DomainName.MaximumLength = AuthenticatingAuthority->MaximumLength;
	PrimaryCredentials->DomainName.Buffer = (PWSTR) EIDAlloc(AuthenticatingAuthority->MaximumLength);
	if (PrimaryCredentials->DomainName.Buffer)
	{
		memcpy(PrimaryCredentials->DomainName.Buffer, AuthenticatingAuthority->Buffer, AuthenticatingAuthority->MaximumLength);
	}

	PrimaryCredentials->Password.Length = (USHORT) wcslen(szPassword) * sizeof(WCHAR);
	PrimaryCredentials->Password.MaximumLength = PrimaryCredentials->Password.Length;
	PrimaryCredentials->Password.Buffer = (PWSTR) EIDAlloc(PrimaryCredentials->Password.MaximumLength);
	if (PrimaryCredentials->Password.Buffer)
	{
		memcpy(PrimaryCredentials->Password.Buffer, szPassword, PrimaryCredentials->Password.Length);
	}

	// we decide that the password cannot be changed so copy it into old pass
	PrimaryCredentials->OldPassword.Length = 0;
	PrimaryCredentials->OldPassword.MaximumLength = 0;
	PrimaryCredentials->OldPassword.Buffer = nullptr;//(PWSTR) FunctionTable->AllocateLsaHeap(PrimaryCredentials->OldPassword.MaximumLength);;
	
	// the flag PRIMARY_CRED_INTERACTIVE_SMARTCARD_LOGON is used for the "force smart card policy"
	// the flag PRIMARY_CRED_CLEAR_PASSWORD is used to tell the password to DPAPI
	PrimaryCredentials->Flags = PRIMARY_CRED_CLEAR_PASSWORD | PRIMARY_CRED_INTERACTIVE_SMARTCARD_LOGON;

	PrimaryCredentials->UserSid = (PSID)EIDAlloc(GetLengthSid(UserSid));
	if (PrimaryCredentials->UserSid)
	{
		CopySid(GetLengthSid(UserSid),PrimaryCredentials->UserSid,UserSid);
	}

	PrimaryCredentials->DnsDomainName.Length = 0;
	PrimaryCredentials->DnsDomainName.MaximumLength = 0;
	PrimaryCredentials->DnsDomainName.Buffer = nullptr;

	PrimaryCredentials->Upn.Length = 0;
	PrimaryCredentials->Upn.MaximumLength = 0;
	PrimaryCredentials->Upn.Buffer = nullptr;

	PrimaryCredentials->LogonServer.Length = AuthenticatingAuthority->Length;
	PrimaryCredentials->LogonServer.MaximumLength = AuthenticatingAuthority->MaximumLength;
	PrimaryCredentials->LogonServer.Buffer = (PWSTR) EIDAlloc(AuthenticatingAuthority->MaximumLength);
	if (PrimaryCredentials->LogonServer.Buffer)
	{
		memcpy(PrimaryCredentials->LogonServer.Buffer, AuthenticatingAuthority->Buffer, AuthenticatingAuthority->MaximumLength);
	}
	EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Leave");
	return STATUS_SUCCESS;	
}

BOOL CStoredCredentialManager::GetResponseFromChallenge(__in PBYTE pChallenge, __in DWORD dwChallengeSize,__in DWORD dwChallengeType, __in PCCERT_CONTEXT pCertContext, __in PWSTR Pin, __out PBYTE *pSymetricKey, __out DWORD *usSize)
{
	switch(dwChallengeType)
	{
	case eidpdtClearText:
		return GetResponseFromSignatureChallenge(pChallenge,dwChallengeSize,pCertContext,Pin,pSymetricKey,usSize);
	case eidpdtCrypted:
		return GetResponseFromCryptedChallenge(pChallenge,dwChallengeSize,pCertContext,Pin,pSymetricKey,usSize);
	}
	EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Type not implemented");
	return FALSE;
}
BOOL CStoredCredentialManager::GetResponseFromCryptedChallenge(__in PBYTE pChallenge, __in DWORD dwChallengeSize, __in PCCERT_CONTEXT pCertContext, __in PWSTR Pin, __out PBYTE *pSymetricKey, __out DWORD *usSize)
{
	BOOL fReturn = FALSE;
	// check private key
	HCRYPTPROV hProv = NULL;  // Windows handle type - keep as NULL
	DWORD dwKeySpec;
	BOOL fCallerFreeProv = FALSE;
	HCRYPTKEY hCertKey = NULL;  // Windows handle type - keep as NULL
	LPSTR pbPin = nullptr;
	DWORD dwPinLen = 0;
	HCRYPTKEY hKey = NULL;  // Windows handle type - keep as NULL
	DWORD dwSize;
	DWORD dwBlockLen = 20000;
	DWORD dwError = 0;
	PCRYPT_KEY_PROV_INFO pProvInfo = nullptr;
	EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Enter");
	__try
	{
		if (!pSymetricKey)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"pSymetricKey NULL");
			dwError = ERROR_INVALID_PARAMETER;
			__leave;
		}
		*pSymetricKey = nullptr;
		// acquire context on private key
		dwSize = 0;
		if (!CertGetCertificateContextProperty(pCertContext, CERT_KEY_PROV_INFO_PROP_ID, nullptr, &dwSize))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%08x returned by CertGetCertificateContextProperty", GetLastError());
			__leave;
		}
		pProvInfo = (PCRYPT_KEY_PROV_INFO) EIDAlloc(dwSize);
		if (!pProvInfo)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"pProvInfo null");
			dwError = ERROR_OUTOFMEMORY;
			__leave;
		}
		if (!CertGetCertificateContextProperty(pCertContext, CERT_KEY_PROV_INFO_PROP_ID, pProvInfo, &dwSize))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%08x returned by CertGetCertificateContextProperty", GetLastError());
			__leave;
		}
		dwKeySpec = pProvInfo->dwKeySpec;
		// Security: Validate CSP provider before loading
		if (!IsAllowedCSPProvider(pProvInfo->pwszProvName))
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_ERROR, L"CSP provider '%s' not allowed", pProvInfo->pwszProvName);
			dwError = ERROR_ACCESS_DENIED;
			__leave;
		}
		if (!CryptAcquireCertificatePrivateKey(pCertContext,CRYPT_ACQUIRE_SILENT_FLAG | CRYPT_ACQUIRE_USE_PROV_INFO_FLAG,nullptr,&hProv,&dwKeySpec,&fCallerFreeProv))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%08x returned by CryptAcquireCertificatePrivateKey", GetLastError());
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"PIV fallback");
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE, L"Keyspec %S container %s provider %s", (pProvInfo->dwKeySpec == AT_SIGNATURE ?"AT_SIGNATURE":"AT_KEYEXCHANGE"),
					pProvInfo->pwszContainerName, pProvInfo->pwszProvName);
			if (!CryptAcquireContext(&hProv, nullptr, pProvInfo->pwszProvName, pProvInfo->dwProvType, CRYPT_SILENT))
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%08x returned by CryptAcquireContext", GetLastError());
				__leave;
			}
		}

		if (!CryptGetUserKey(hProv, dwKeySpec, &hKey))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%08x returned by CryptGetUserKey", GetLastError());
			__leave;
		}
		dwPinLen = (DWORD) (wcslen(Pin) + sizeof(CHAR));
		pbPin = (LPSTR) EIDAlloc(dwPinLen);
		if (!pbPin)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%08x returned by EIDAlloc", GetLastError());
			__leave;
		}
		if (!WideCharToMultiByte(CP_ACP, 0, Pin, -1, pbPin, dwPinLen, nullptr, nullptr))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%08x returned by WideCharToMultiByte", GetLastError());
			__leave;
		}
		if (!CryptSetProvParam(hProv, (dwKeySpec == AT_KEYEXCHANGE?PP_KEYEXCHANGE_PIN:PP_SIGNATURE_PIN), (PBYTE) pbPin , 0))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%08x returned by CryptSetProvParam - correct PIN ?", GetLastError());
			__leave;
		}
		dwSize = sizeof(DWORD);
		if (!CryptGetKeyParam(hKey, KP_BLOCKLEN, (PBYTE) &dwBlockLen, &dwSize, 0))
		{
			dwError = GetLastError();
			dwBlockLen = 20000; 
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%08x returned by CryptGetKeyParam - using %d as KP_BLOCKLEN", GetLastError(), dwBlockLen);
			dwError = 0;
		}
		*pSymetricKey = (PBYTE) EIDAlloc(dwBlockLen);
		if (!*pSymetricKey)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%08x returned by EIDAlloc", GetLastError());
			__leave;
		}
		memcpy(*pSymetricKey, pChallenge, dwChallengeSize);
		dwSize = dwChallengeSize;
		if (!CryptDecrypt(hKey, NULL, TRUE, 0, *pSymetricKey, &dwSize))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%08x returned by CryptDecrypt", GetLastError());
			__leave;
		}
		*usSize = (USHORT) dwSize;

		
		fReturn = TRUE;
	}
	__finally
	{
		if (!fReturn)
		{
			if (*pSymetricKey)
			{
				EIDFree(*pSymetricKey );
				*pSymetricKey = nullptr;
			}
		}
		if (pbPin)
		{
			SecureZeroMemory(pbPin , dwPinLen);
			EIDFree(pbPin);
		}
		if (hKey)
			CryptDestroyKey(hKey);
		if (hCertKey)
			CryptDestroyKey(hCertKey);
		if (fCallerFreeProv && hProv) 
			CryptReleaseContext(hProv,0);
		if (pProvInfo) 
			EIDFree(pProvInfo);
	}
	SetLastError(dwError);
	return fReturn;
}


BOOL CStoredCredentialManager::GetResponseFromSignatureChallenge(__in PBYTE pbChallenge, __in DWORD dwChallengeSize, __in PCCERT_CONTEXT pCertContext, __in PWSTR szPin, __out PBYTE *ppResponse, __out PDWORD pdwResponseSize)
{
	UNREFERENCED_PARAMETER(dwChallengeSize);
	BOOL fReturn = FALSE;
	LPSTR pbPin = nullptr;
	HCRYPTPROV hProv = NULL;  // Windows handle type - keep as NULL
	DWORD dwKeySpec;
	BOOL fCallerFreeProv = FALSE;
	HCRYPTKEY hCertKey = NULL;  // Windows handle type - keep as NULL
	HCRYPTHASH hHash = NULL;  // Windows handle type - keep as NULL
	DWORD dwPinLen = 0;
	DWORD dwError = 0;
	LPCTSTR sDescription = TEXT("");
	PCRYPT_KEY_PROV_INFO pKeyProvInfo = nullptr;
	__try
	{
		DWORD dwSize = 0;
		if (!CertGetCertificateContextProperty(pCertContext, CERT_KEY_PROV_INFO_PROP_ID, nullptr, &dwSize))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by CertGetCertificateContextProperty", GetLastError());
			__leave;
		}
		pKeyProvInfo = (PCRYPT_KEY_PROV_INFO) EIDAlloc(dwSize);
		if (!pKeyProvInfo)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by malloc", GetLastError());
			__leave;
		}
		if (!CertGetCertificateContextProperty(pCertContext, CERT_KEY_PROV_INFO_PROP_ID, (PBYTE) pKeyProvInfo, &dwSize))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by CertGetCertificateContextProperty", GetLastError());
			__leave;
		}
		*pdwResponseSize = 0;
		dwKeySpec = pKeyProvInfo->dwKeySpec;
		if (!CryptAcquireCertificatePrivateKey(pCertContext,CRYPT_ACQUIRE_SILENT_FLAG | CRYPT_ACQUIRE_USE_PROV_INFO_FLAG,nullptr,&hProv,&dwKeySpec,&fCallerFreeProv))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by CryptAcquireCertificatePrivateKey", GetLastError());
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE, L"Keyspec %S container %s provider %s", (pKeyProvInfo->dwKeySpec == AT_SIGNATURE ?"AT_SIGNATURE":"AT_KEYEXCHANGE"),
					pKeyProvInfo->pwszContainerName, pKeyProvInfo->pwszProvName);
			__leave;
		}
		dwPinLen = (DWORD) wcslen(szPin) + 1;
		pbPin = (LPSTR) EIDAlloc(dwPinLen);
		if (!pbPin)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by malloc", GetLastError());
			__leave;
		}
		if (!WideCharToMultiByte(CP_ACP, 0, szPin, -1, pbPin, dwPinLen, nullptr, nullptr))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by WideCharToMultiByte", GetLastError());
			__leave;
		}
		if (!CryptSetProvParam(hProv, (dwKeySpec == AT_KEYEXCHANGE?PP_KEYEXCHANGE_PIN:PP_SIGNATURE_PIN), (PBYTE) pbPin , 0))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by CryptSetProvParam - correct PIN ?", GetLastError());
			__leave;
		}
		if (!CryptCreateHash(hProv,CALG_SHA,NULL,0,&hHash))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by CryptCreateHash", GetLastError());
			__leave;
		}
		if (!CryptSetHashParam(hHash, HP_HASHVAL, pbChallenge, 0))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by CryptSetHashParam", GetLastError());
			__leave;
		}
		if (!CryptSignHash(hHash,dwKeySpec, sDescription, 0, nullptr, pdwResponseSize))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by CryptSignHash1", GetLastError());
			__leave;
		}
		*ppResponse = (PBYTE) EIDAlloc(*pdwResponseSize);
		if (!*ppResponse)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by malloc", GetLastError());
			__leave;
		}
		if (!CryptSignHash(hHash,dwKeySpec, sDescription, 0, *ppResponse, pdwResponseSize))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by CryptSignHash2", GetLastError());
			__leave;
		}
		fReturn = TRUE;
	}
	__finally
	{
		if (pbPin)
		{
			SecureZeroMemory(pbPin , dwPinLen);
			EIDFree(pbPin);
		}
		if (pKeyProvInfo)
			EIDFree(pKeyProvInfo);
		if (hCertKey)
			CryptDestroyKey(hCertKey);
		if (hHash)
			CryptDestroyHash(hHash);
		if (fCallerFreeProv && hProv) 
			CryptReleaseContext(hProv,0);
	}
	SetLastError(dwError);
	return fReturn;
}


struct KEY_BLOB {
  BYTE   bType;
  BYTE   bVersion;
  WORD   reserved;
  ALG_ID aiKeyAlg;
  ULONG cb;
  BYTE Data[CREDENTIALKEYLENGTH/8];
};

// create a symetric key which can be used to crypt data and
// which is saved and protected by the public key
BOOL CStoredCredentialManager::GenerateSymetricKeyAndEncryptIt(__in HCRYPTPROV hProv, __in HCRYPTKEY hKey, __out HCRYPTKEY *phKey, __out PBYTE* pSymetricKey, __out USHORT *usSize)
{
	BOOL fReturn = FALSE;
	BOOL fStatus;
	HCRYPTHASH hHash = NULL;  // Windows handle type - keep as NULL
	DWORD dwSize;
	KEY_BLOB bKey;
	DWORD dwError = 0;
	__try
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Enter");
		*pSymetricKey = nullptr;
		*phKey = NULL;
		dwSize = sizeof(DWORD);
		DWORD dwBlockLen;
		// key is generated here
		bKey.bType = PLAINTEXTKEYBLOB;
		bKey.bVersion = CUR_BLOB_VERSION;
		bKey.reserved = 0;
		bKey.aiKeyAlg = CREDENTIALCRYPTALG;
		bKey.cb = CREDENTIALKEYLENGTH/8;
		fStatus = CryptGenRandom(hProv,bKey.cb,bKey.Data);
		if(!fStatus)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptGenRandom 0x%08x",GetLastError());
			__leave;
		}
		fStatus = CryptImportKey(hProv,(PBYTE)&bKey,sizeof(KEY_BLOB),NULL,CRYPT_EXPORTABLE,phKey);
		if(!fStatus)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptImportKey 0x%08x",GetLastError());
			__leave;
		}
		// save
		dwBlockLen = 0;
		fStatus = CryptEncrypt(hKey, hHash,TRUE,0,nullptr,&dwBlockLen, 0);
		if(!fStatus)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptEncrypt 0x%08x",GetLastError());
			__leave;
		}
		*pSymetricKey = (PBYTE) EIDAlloc(dwBlockLen);
		if (!*pSymetricKey)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%08x returned by EIDAlloc", GetLastError());
			__leave;
		}
		memcpy(*pSymetricKey, bKey.Data, CREDENTIALKEYLENGTH/8);
		dwSize = CREDENTIALKEYLENGTH/8;
		fStatus = CryptEncrypt(hKey, hHash,TRUE,0,*pSymetricKey,&dwSize, dwBlockLen);
		if(!fStatus)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptEncrypt 0x%08x",GetLastError());
			__leave;
		}
		*usSize = (USHORT) dwSize;
		// bKey is know encrypted
		
		fReturn = TRUE;
	}
	__finally
	{
		if (!fReturn)
		{
			if (*pSymetricKey)
			{
				EIDFree(*pSymetricKey);
				*pSymetricKey = nullptr;
			}
			if (*phKey)
			{
				CryptDestroyKey(*phKey);
				*phKey = NULL;
			}
		}
		if (hHash)
			CryptDestroyHash(hHash);
	}
	SetLastError(dwError);
	return fReturn;
}

BOOL CStoredCredentialManager::EncryptPasswordAndSaveIt(__in HCRYPTKEY hKey, __in PWSTR szPassword, __in_opt USHORT dwPasswordLen, __out PBYTE *pEncryptedPassword, __out USHORT *usSize)
{
	BOOL fReturn = FALSE, fStatus;
	DWORD dwPasswordSize, dwSize, dwBlockLen, dwEncryptedSize;
	DWORD dwRoundNumber;
	DWORD dwError = 0;
	__try
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Enter");
		dwPasswordSize = (DWORD) (dwPasswordLen?dwPasswordLen:wcslen(szPassword)* sizeof(WCHAR));
		dwSize = sizeof(DWORD);
		if (!CryptGetKeyParam(hKey, KP_BLOCKLEN, (PBYTE) &dwBlockLen, &dwSize, 0))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%08x returned by CryptGetKeyParam", GetLastError());
			__leave;
		}
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"dwBlockLen = %d",dwBlockLen);
		// block size = 256             100 => 1     256 => 1      257  => 2
		dwRoundNumber = ((DWORD)(dwPasswordSize/dwBlockLen)) + ((dwPasswordSize%dwBlockLen) ? 1 : 0);
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"dwRoundNumber = %d",dwRoundNumber);
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"dwPasswordSize = %d",dwPasswordSize);
		*pEncryptedPassword = (PBYTE) EIDAlloc(dwRoundNumber * dwBlockLen);
		if (!*pEncryptedPassword)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"EIDAlloc 0x%08x",GetLastError());
			__leave;
		}	
		memset(*pEncryptedPassword, 0, dwRoundNumber * dwBlockLen);
		memcpy(*pEncryptedPassword, szPassword, dwPasswordSize);
		
		dwEncryptedSize = 0;
		for (DWORD dwI = 0; dwI < dwRoundNumber; dwI++)
		{
			dwEncryptedSize = (dwI == dwRoundNumber-1 ? dwPasswordSize%dwBlockLen : dwBlockLen);
			fStatus = CryptEncrypt(hKey, NULL,(dwI == dwRoundNumber-1 ? TRUE:FALSE),0,
						*pEncryptedPassword + dwI * dwBlockLen,
						&dwEncryptedSize, dwBlockLen);
			if(!fStatus)
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptEncrypt 0x%08x round = %d",GetLastError(), dwI);
				__leave;
			}
		}
		*usSize = (USHORT) ((dwRoundNumber -1 ) * dwBlockLen + dwEncryptedSize);
		// szPassword is know encrypted

		fReturn = TRUE;
	}
	__finally
	{
		if (!fReturn)
		{
			if (*pEncryptedPassword)
			{
				EIDFree(*pEncryptedPassword);
				*pEncryptedPassword = nullptr;
			}
		}
	}
	SetLastError(dwError);
	return fReturn;
}

BOOL CStoredCredentialManager::GetPasswordFromChallengeResponse(__in DWORD dwRid, __in PBYTE ppChallenge, __in DWORD dwChallengeSize, __in DWORD dwChallengeType, __in PBYTE pResponse, __in DWORD dwResponseSize, PWSTR *pszPassword)
{
	switch(dwChallengeType)
	{
	case eidpdtClearText:
		return GetPasswordFromSignatureChallengeResponse(dwRid,ppChallenge,dwChallengeSize,pResponse,dwResponseSize,pszPassword);
	case eidpdtCrypted:
		return GetPasswordFromCryptedChallengeResponse(dwRid,ppChallenge,dwChallengeSize,pResponse,dwResponseSize,pszPassword);
	case eidpdtDPAPI:
		return GetPasswordFromDPAPIChallengeResponse(dwRid,ppChallenge,dwChallengeSize,pResponse,dwResponseSize,pszPassword);
	}
	EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Type not implemented");
	return FALSE;
}

BOOL CStoredCredentialManager::GetPasswordFromCryptedChallengeResponse(__in DWORD dwRid, __in PBYTE ppChallenge, __in DWORD dwChallengeSize, __in PBYTE pResponse, __in DWORD dwResponseSize, PWSTR *pszPassword)
{
	UNREFERENCED_PARAMETER(ppChallenge);
	UNREFERENCED_PARAMETER(dwChallengeSize);
	BOOL fReturn = FALSE, fStatus;
	DWORD dwSize;
	HCRYPTPROV hProv = NULL;  // Windows handle type - keep as NULL
	HCRYPTKEY hKey = NULL;  // Windows handle type - keep as NULL
	KEY_BLOB bKey;
	*pszPassword = nullptr;
	DWORD dwBlockLen;
	DWORD dwRoundNumber;
	DWORD dwError = 0;
	PEID_PRIVATE_DATA pEidPrivateData = nullptr;
	__try
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Enter");
		// read the encrypted password
		if (!dwRid)
		{
			dwError = ERROR_NONE_MAPPED;
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"dwRid 0x%08x",dwError);
			__leave;
		}
		fStatus = RetrievePrivateData(dwRid,&pEidPrivateData);
		if (!fStatus)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"RetrievePrivateData 0x%08x",dwError);
			__leave;
		}
		if (pEidPrivateData->dwSymetricKeySize != dwChallengeSize)
		{
			dwError = ERROR_NONE_MAPPED;
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"dwChallengeSize = 0x%08x",dwChallengeSize);
			__leave;
		}
		// key is generated here
		bKey.bType = PLAINTEXTKEYBLOB;
		bKey.bVersion = CUR_BLOB_VERSION;
		bKey.reserved = 0;
		bKey.aiKeyAlg = CREDENTIALCRYPTALG;
		bKey.cb = dwResponseSize;
		memcpy(bKey.Data, pResponse, dwResponseSize);
		// import the aes key
		fStatus = CryptAcquireContext(&hProv,CREDENTIAL_CONTAINER,CREDENTIALPROVIDER,PROV_RSA_AES,0);
		if(!fStatus)
		{
			dwError = GetLastError();
			if (dwError == NTE_BAD_KEYSET)
			{
				fStatus = CryptAcquireContext(&hProv,CREDENTIAL_CONTAINER,CREDENTIALPROVIDER,PROV_RSA_AES,CRYPT_NEWKEYSET);
				dwError = GetLastError();
			}
			if (!fStatus)
			{
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptAcquireContext 0x%08x",dwError);
				__leave;
			}
		}
		else
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Container already existed !!");
		}
		fStatus = CryptImportKey(hProv,(PBYTE) &bKey,sizeof(KEY_BLOB),0,CRYPT_EXPORTABLE,&hKey);
		if(!fStatus)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptImportKey 0x%08x",GetLastError());
			__leave;
		}
		// decode it
		dwSize = sizeof(DWORD);
		if (!CryptGetKeyParam(hKey, KP_BLOCKLEN, (PBYTE) &dwBlockLen, &dwSize, 0))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%08x returned by CryptGetKeyParam", GetLastError());
			__leave;
		}
		dwRoundNumber = (DWORD)(pEidPrivateData->dwPasswordSize / dwBlockLen) + 
			((pEidPrivateData->dwPasswordSize % dwBlockLen) ? 1 : 0);
		*pszPassword = (PWSTR) EIDAlloc(dwRoundNumber *  dwBlockLen + sizeof(WCHAR));
		if (!*pszPassword)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"EIDAlloc 0x%08x", GetLastError());
			__leave;
		}
		memcpy(*pszPassword, pEidPrivateData->Data + pEidPrivateData->dwPasswordOffset, pEidPrivateData->dwPasswordSize);

		for (DWORD dwI = 0; dwI < dwRoundNumber ; dwI++)
		{
			dwSize = (dwI == dwRoundNumber -1 ? pEidPrivateData->dwPasswordSize%dwBlockLen : dwBlockLen);
			fStatus = CryptDecrypt(hKey, NULL,(dwI == dwRoundNumber -1 ?TRUE:FALSE),0,
				((PBYTE) *pszPassword) + dwI * dwBlockLen,&dwSize);
			if(!fStatus)
			{
				dwError = GetLastError();
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptDecrypt 0x%08x",GetLastError());
				__leave;
			}
		}
		(*pszPassword)[((dwRoundNumber-1) * dwBlockLen + dwSize)/sizeof(WCHAR)] = '\0';
		fReturn = TRUE;
	}
	__finally
	{
		if (!fReturn)
		{
			if (*pszPassword)
			{
				SecureZeroMemory(*pszPassword, wcslen(*pszPassword) * sizeof(WCHAR));
				EIDFree(*pszPassword);
				*pszPassword = nullptr;
			}
		}
		if (pEidPrivateData)
		{
			// Zero entire structure including password data
			SecureZeroMemory(pEidPrivateData, sizeof(EID_PRIVATE_DATA) + pEidPrivateData->dwCertificatSize + pEidPrivateData->dwSymetricKeySize + pEidPrivateData->dwPasswordSize);
			EIDFree(pEidPrivateData);
		}
		if (hKey)
			CryptDestroyKey(hKey);
		if (hProv)
		{
			CryptReleaseContext(hProv, 0);
			CryptAcquireContext(&hProv,CREDENTIAL_CONTAINER,CREDENTIALPROVIDER,PROV_RSA_AES,CRYPT_DELETEKEYSET);
		}
	}
	SetLastError(dwError);
	return fReturn;
}

BOOL CStoredCredentialManager::GetPasswordFromSignatureChallengeResponse(__in DWORD dwRid, __in PBYTE ppChallenge, __in DWORD dwChallengeSize, __in PBYTE pResponse, __in DWORD dwResponseSize, PWSTR *pszPassword)
{
	BOOL fReturn = FALSE, fStatus;
	DWORD dwError = 0;
	PEID_PRIVATE_DATA pEidPrivateData = nullptr;
	HCRYPTPROV hProv = NULL;  // Windows handle type - keep as NULL
	HCRYPTKEY hKey = NULL;  // Windows handle type - keep as NULL
	HCRYPTHASH hHash = NULL;  // Windows handle type - keep as NULL
	PCCERT_CONTEXT pCertContextVerif = nullptr;
	PCRYPT_KEY_PROV_INFO pKeyProvInfo = nullptr;
	__try
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Enter");
		// read the encrypted password
		if (!dwRid)
		{
			dwError = ERROR_NONE_MAPPED;
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"dwRid 0x%08x",dwError);
			__leave;
		}
		if (CREDENTIALKEYLENGTH != dwChallengeSize)
		{
			dwError = ERROR_NONE_MAPPED;
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"dwChallengeSize = 0x%08x",dwChallengeSize);
			__leave;
		}
		if (pszPassword == nullptr)
		{
			dwError = ERROR_NONE_MAPPED;
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"pszPassword null");
			__leave;
		}
		*pszPassword = nullptr;
		fStatus = RetrievePrivateData(dwRid,&pEidPrivateData);
		if (!fStatus)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"RetrievePrivateData 0x%08x",dwError);
			__leave;
		}
		pCertContextVerif = CertCreateCertificateContext(X509_ASN_ENCODING,
			(PBYTE)pEidPrivateData->Data + pEidPrivateData->dwCertificatOffset, pEidPrivateData->dwCertificatSize);
		if (!pCertContextVerif)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CertCreateCertificateContext 0x%08x",dwError);
			__leave;
		}
		// import the public key
		fStatus = CryptAcquireContext(&hProv,CREDENTIAL_CONTAINER,CREDENTIALPROVIDER,PROV_RSA_AES,0);
		if(!fStatus)
		{
			dwError = GetLastError();
			if (dwError == NTE_BAD_KEYSET)
			{
				fStatus = CryptAcquireContext(&hProv,CREDENTIAL_CONTAINER,CREDENTIALPROVIDER,PROV_RSA_AES,CRYPT_NEWKEYSET);
				dwError = GetLastError();
			}
			if (!fStatus)
			{
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptAcquireContext 0x%08x",dwError);
				__leave;
			}
		}
		else
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Container already existed !!");
		}
		fStatus = CryptImportPublicKeyInfo(hProv, pCertContextVerif->dwCertEncodingType, &(pCertContextVerif->pCertInfo->SubjectPublicKeyInfo),&hKey);
		if (!fStatus)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptImportKey 0x%08x",GetLastError());
			__leave;
		}
		if (!CryptCreateHash(hProv,CALG_SHA,NULL,0,&hHash))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by CryptCreateHash", GetLastError());
			__leave;
		}
		if (!CryptSetHashParam(hHash, HP_HASHVAL, ppChallenge, 0))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by CryptSetHashParam", GetLastError());
			__leave;
		}

		if (!CryptVerifySignature(hHash, pResponse, dwResponseSize, hKey, TEXT(""), 0))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by CryptVerifySignature", GetLastError());
			__leave;
		}
		*pszPassword = (PWSTR) EIDAlloc(pEidPrivateData->dwPasswordSize + sizeof(WCHAR));
		if (!*pszPassword)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"EIDAlloc 0x%08x", GetLastError());
			__leave;
		}
		memcpy(*pszPassword, (PBYTE)pEidPrivateData->Data + pEidPrivateData->dwPasswordOffset,pEidPrivateData->dwPasswordSize);
		(*pszPassword)[pEidPrivateData->dwPasswordSize / sizeof(WCHAR)] = '\0';
		fReturn = TRUE;

	}
	__finally
	{
		if (!fReturn)
		{
			if (pszPassword)
			{
				if (*pszPassword)
				{
					SecureZeroMemory(*pszPassword, wcslen(*pszPassword) * sizeof(WCHAR));
					EIDFree(*pszPassword);
					*pszPassword = nullptr;
				}
			}
		}
		if (pEidPrivateData)
		{
			// Zero entire structure including password data
			SecureZeroMemory(pEidPrivateData, sizeof(EID_PRIVATE_DATA) + pEidPrivateData->dwCertificatSize + pEidPrivateData->dwSymetricKeySize + pEidPrivateData->dwPasswordSize);
			EIDFree(pEidPrivateData);
		}
		if (pKeyProvInfo)
			EIDFree(pKeyProvInfo);
		if (pCertContextVerif)
			CertFreeCertificateContext(pCertContextVerif);
		if (hHash)
			CryptDestroyHash(hHash);
		if (hKey)
			CryptDestroyKey(hKey);
		if (hProv)
		{
			CryptReleaseContext(hProv, 0);
			CryptAcquireContext(&hProv,CREDENTIAL_CONTAINER,CREDENTIALPROVIDER,PROV_RSA_AES,CRYPT_DELETEKEYSET);
		}
	}
	return fReturn;
}

BOOL CStoredCredentialManager::GetPasswordFromDPAPIChallengeResponse(__in DWORD dwRid, __in PBYTE ppChallenge, __in DWORD dwChallengeSize, __in PBYTE pResponse, __in DWORD dwResponseSize, PWSTR *pszPassword)
{
	BOOL fReturn = FALSE, fStatus;
	DWORD dwError = 0;
	PEID_PRIVATE_DATA pEidPrivateData = nullptr;
	HCRYPTPROV hProv = NULL;  // Windows handle type - keep as NULL
	HCRYPTKEY hKey = NULL;  // Windows handle type - keep as NULL
	HCRYPTHASH hHash = NULL;  // Windows handle type - keep as NULL
	PCCERT_CONTEXT pCertContextVerif = nullptr;
	DATA_BLOB DataIn = {0};
	DATA_BLOB DataOut = {0};
	__try
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Enter");
		// read the encrypted password
		if (!dwRid)
		{
			dwError = ERROR_NONE_MAPPED;
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"dwRid 0x%08x",dwError);
			__leave;
		}
		if (CREDENTIALKEYLENGTH != dwChallengeSize)
		{
			dwError = ERROR_NONE_MAPPED;
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"dwChallengeSize = 0x%08x",dwChallengeSize);
			__leave;
		}
		if (pszPassword == nullptr)
		{
			dwError = ERROR_NONE_MAPPED;
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"pszPassword null");
			__leave;
		}
		*pszPassword = nullptr;
		fStatus = RetrievePrivateData(dwRid,&pEidPrivateData);
		if (!fStatus)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"RetrievePrivateData 0x%08x",dwError);
			__leave;
		}
		pCertContextVerif = CertCreateCertificateContext(X509_ASN_ENCODING,
			(PBYTE)pEidPrivateData->Data + pEidPrivateData->dwCertificatOffset, pEidPrivateData->dwCertificatSize);
		if (!pCertContextVerif)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CertCreateCertificateContext 0x%08x",dwError);
			__leave;
		}
		// import the public key
		fStatus = CryptAcquireContext(&hProv,CREDENTIAL_CONTAINER,CREDENTIALPROVIDER,PROV_RSA_AES,0);
		if(!fStatus)
		{
			dwError = GetLastError();
			if (dwError == NTE_BAD_KEYSET)
			{
				fStatus = CryptAcquireContext(&hProv,CREDENTIAL_CONTAINER,CREDENTIALPROVIDER,PROV_RSA_AES,CRYPT_NEWKEYSET);
				dwError = GetLastError();
			}
			if (!fStatus)
			{
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptAcquireContext 0x%08x",dwError);
				__leave;
			}
		}
		else
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Container already existed !!");
		}
		fStatus = CryptImportPublicKeyInfo(hProv, pCertContextVerif->dwCertEncodingType, &(pCertContextVerif->pCertInfo->SubjectPublicKeyInfo),&hKey);
		if (!fStatus)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptImportKey 0x%08x",GetLastError());
			__leave;
		}
		if (!CryptCreateHash(hProv,CALG_SHA,NULL,0,&hHash))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by CryptCreateHash", GetLastError());
			__leave;
		}
		if (!CryptSetHashParam(hHash, HP_HASHVAL, ppChallenge, 0))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by CryptSetHashParam", GetLastError());
			__leave;
		}

		if (!CryptVerifySignature(hHash, pResponse, dwResponseSize, hKey, TEXT(""), 0))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by CryptVerifySignature", GetLastError());
			__leave;
		}

		// Signature verified - now decrypt the password using DPAPI
		DataIn.pbData = (PBYTE)pEidPrivateData->Data + pEidPrivateData->dwPasswordOffset;
		DataIn.cbData = pEidPrivateData->dwPasswordSize;

		if (!CryptUnprotectData(&DataIn, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_LOCAL_MACHINE, &DataOut))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptUnprotectData failed 0x%08x", dwError);
			__leave;
		}

		*pszPassword = (PWSTR) EIDAlloc(DataOut.cbData + sizeof(WCHAR));
		if (!*pszPassword)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"EIDAlloc 0x%08x", GetLastError());
			__leave;
		}
		memcpy(*pszPassword, DataOut.pbData, DataOut.cbData);
		(*pszPassword)[DataOut.cbData / sizeof(WCHAR)] = L'\0';
		fReturn = TRUE;

	}
	__finally
	{
		if (!fReturn)
		{
			if (pszPassword)
			{
				if (*pszPassword)
				{
					SecureZeroMemory(*pszPassword, wcslen(*pszPassword) * sizeof(WCHAR));
					EIDFree(*pszPassword);
					*pszPassword = nullptr;
				}
			}
		}
		if (DataOut.pbData)
		{
			SecureZeroMemory(DataOut.pbData, DataOut.cbData);
			LocalFree(DataOut.pbData);
		}
		if (pEidPrivateData)
		{
			// Zero entire structure including password data
			SecureZeroMemory(pEidPrivateData, sizeof(EID_PRIVATE_DATA) + pEidPrivateData->dwCertificatSize + pEidPrivateData->dwSymetricKeySize + pEidPrivateData->dwPasswordSize);
			EIDFree(pEidPrivateData);
		}
		if (pCertContextVerif)
			CertFreeCertificateContext(pCertContextVerif);
		if (hHash)
			CryptDestroyHash(hHash);
		if (hKey)
			CryptDestroyKey(hKey);
		if (hProv)
		{
			CryptReleaseContext(hProv, 0);
			CryptAcquireContext(&hProv,CREDENTIAL_CONTAINER,CREDENTIALPROVIDER,PROV_RSA_AES,CRYPT_DELETEKEYSET);
		}
	}
	SetLastError(dwError);
	return fReturn;
}

BOOL CStoredCredentialManager::VerifySignatureChallengeResponse(__in DWORD dwRid, __in PBYTE ppChallenge, __in DWORD dwChallengeSize, __in PBYTE pResponse, __in DWORD dwResponseSize)
{
	UNREFERENCED_PARAMETER(dwChallengeSize);
	BOOL fReturn = FALSE, fStatus;
	DWORD dwError = 0;
	PEID_PRIVATE_DATA pEidPrivateData = nullptr;
	HCRYPTPROV hProv = NULL;  // Windows handle type - keep as NULL
	HCRYPTKEY hKey = NULL;  // Windows handle type - keep as NULL
	HCRYPTHASH hHash = NULL;  // Windows handle type - keep as NULL
	PCCERT_CONTEXT pCertContext = nullptr;
	__try
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Enter");
		// read the encrypted password
		if (!dwRid)
		{
			dwError = ERROR_NONE_MAPPED;
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"dwRid 0x%08x",dwError);
			__leave;
		}
		fStatus = RetrievePrivateData(dwRid,&pEidPrivateData);
		if (!fStatus)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"RetrievePrivateData 0x%08x",dwError);
			__leave;
		}
		pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING, 
			(PBYTE)pEidPrivateData->Data + pEidPrivateData->dwCertificatOffset, pEidPrivateData->dwCertificatSize);
		if (!pCertContext)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CertCreateCertificateContext 0x%08x",dwError);
			__leave;
		}
		// import the public key
		fStatus = CryptAcquireContext(&hProv,CREDENTIAL_CONTAINER,CREDENTIALPROVIDER,PROV_RSA_AES,0);
		if(!fStatus)
		{
			dwError = GetLastError();
			if (dwError == NTE_BAD_KEYSET)
			{
				fStatus = CryptAcquireContext(&hProv,CREDENTIAL_CONTAINER,CREDENTIALPROVIDER,PROV_RSA_AES,CRYPT_NEWKEYSET);
				dwError = GetLastError();
			}
			if (!fStatus)
			{
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptAcquireContext 0x%08x",dwError);
				__leave;
			}
		}
		else
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Container already existed !!");
		}
		fStatus = CryptImportPublicKeyInfo(hProv, pCertContext->dwCertEncodingType, &(pCertContext->pCertInfo->SubjectPublicKeyInfo),&hKey);
		if (!fStatus)
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"CryptImportKey 0x%08x",GetLastError());
			__leave;
		}
		if (!CryptCreateHash(hProv,CALG_SHA,NULL,0,&hHash))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by CryptCreateHash", GetLastError());
			__leave;
		}
		if (!CryptSetHashParam(hHash, HP_HASHVAL, ppChallenge, 0))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by CryptSetHashParam", GetLastError());
			__leave;
		}

		if (!CryptVerifySignature(hHash, pResponse, dwResponseSize, hKey, TEXT(""), 0))
		{
			dwError = GetLastError();
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%x returned by CryptVerifySignature", GetLastError());
			__leave;
		}
		fReturn = TRUE;

	}
	__finally
	{
		if (pEidPrivateData)
		{
			EIDFree(pEidPrivateData);
		}
		if (pCertContext)
			CertFreeCertificateContext(pCertContext);
		if (hHash)
			CryptDestroyHash(hHash);
		if (hKey)
			CryptDestroyKey(hKey);
		if (hProv)
		{
			CryptReleaseContext(hProv, 0);
			CryptAcquireContext(&hProv,CREDENTIAL_CONTAINER,CREDENTIALPROVIDER,PROV_RSA_AES,CRYPT_DELETEKEYSET);
		}
	}
	return fReturn;
}
////////////////////////////////////////////////////////////////////////////////
// LEVEL 3
////////////////////////////////////////////////////////////////////////////////

BOOL CStoredCredentialManager::StorePrivateData(__in DWORD dwRid, __in_opt PBYTE pbSecret, __in_opt USHORT usSecretSize)
{

	if (!EIDIsComponentInLSAContext())
 	{
		return StorePrivateDataDebug(dwRid,pbSecret, usSecretSize);
	}

	LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE LsaPolicyHandle = nullptr;

    LSA_UNICODE_STRING lusSecretName;
    LSA_UNICODE_STRING lusSecretData;
	WCHAR szLsaKeyName[256];
    NTSTATUS ntsResult = STATUS_SUCCESS;
    //  Object attributes are reserved, so initialize to zeros.
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
	DWORD dwError = 0;
	BOOL fReturn = FALSE;
	__try
	{
		//  Get a handle to the Policy object.
		ntsResult = LsaOpenPolicy(
			nullptr,    // local machine
			&ObjectAttributes, 
			POLICY_CREATE_SECRET | READ_CONTROL | WRITE_OWNER | WRITE_DAC,
			&LsaPolicyHandle);

		if( STATUS_SUCCESS != ntsResult )
		{
			//  An error occurred. Display it as a win32 error code.
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%08x returned by LsaOpenPolicy", ntsResult);
			dwError = LsaNtStatusToWinError(ntsResult);
			__leave;
		} 

		//  Initialize an LSA_UNICODE_STRING for the name of the
		if (FAILED(StringCchPrintfW(szLsaKeyName, ARRAYSIZE(szLsaKeyName), L"%s_%08X", CREDENTIAL_LSAPREFIX, dwRid)))
		{
			dwError = ERROR_BUFFER_OVERFLOW;
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING, L"StringCchPrintfW failed for LSA key name");
			__leave;
		}

		lusSecretName.Buffer = szLsaKeyName;
		lusSecretName.Length = (USHORT) wcslen(szLsaKeyName)* sizeof(WCHAR);
		lusSecretName.MaximumLength = lusSecretName.Length;
		//  If the pwszSecret parameter is NULL, then clear the secret.
		if( nullptr == pbSecret )
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"Clearing %x",dwRid);
			ntsResult = LsaStorePrivateData(
				LsaPolicyHandle,
				&lusSecretName,
				nullptr);
		}
		else
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,L"Setting %x",dwRid);
			//  Initialize an LSA_UNICODE_STRING for the value
			//  of the private data. 
			lusSecretData.Buffer = (PWSTR) pbSecret;
			lusSecretData.Length = usSecretSize;
			lusSecretData.MaximumLength = usSecretSize;
			ntsResult = LsaStorePrivateData(
				LsaPolicyHandle,
				&lusSecretName,
				&lusSecretData);
		}
		if( STATUS_SUCCESS != ntsResult )
		{
			//  An error occurred. Display it as a win32 error code.
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%08x returned by LsaStorePrivateData", ntsResult);
			dwError = LsaNtStatusToWinError(ntsResult);
			__leave;
		}
		fReturn = TRUE;
	}
	__finally
	{
		if (LsaPolicyHandle) LsaClose(LsaPolicyHandle);
	} 
	SetLastError(dwError);
    return fReturn;

}

BOOL CStoredCredentialManager::StorePrivateDataDebug(__in DWORD dwRid, __in_opt PBYTE pbSecret, __in_opt USHORT usSecretSize)
{
	// SECURITY FIX: Debug credential storage to TEMP files is disabled
	// This was a critical security vulnerability (CWE-532) that exposed credentials in plaintext
	// Credentials can only be stored securely via LSA private data storage
	UNREFERENCED_PARAMETER(dwRid);
	UNREFERENCED_PARAMETER(pbSecret);
	UNREFERENCED_PARAMETER(usSecretSize);

	EIDCardLibraryTrace(WINEVENT_LEVEL_ERROR, L"SECURITY: Debug credential file storage is disabled - must run in LSA context");
	SetLastError(ERROR_ACCESS_DENIED);
	return FALSE;
}

BOOL CStoredCredentialManager::RetrievePrivateData(__in DWORD dwRid, __out PEID_PRIVATE_DATA *ppPrivateData)
{
	if (!EIDIsComponentInLSAContext())
 	{
		return RetrievePrivateDataDebug(dwRid,ppPrivateData);
	}

	LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE LsaPolicyHandle = nullptr;
 PLSA_UNICODE_STRING pData = nullptr;
    LSA_UNICODE_STRING lusSecretName;
	WCHAR szLsaKeyName[256];
	NTSTATUS ntsResult = STATUS_SUCCESS;
    //  Object attributes are reserved, so initialize to zeros.
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
	DWORD dwError = 0;
	BOOL fReturn = FALSE;
	__try
	{
		//  Get a handle to the Policy object.
		ntsResult = LsaOpenPolicy(
			nullptr,    // local machine
			&ObjectAttributes, 
			POLICY_GET_PRIVATE_INFORMATION,
			&LsaPolicyHandle);

		if( STATUS_SUCCESS != ntsResult )
		{
			//  An error occurred. Display it as a win32 error code.
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%08x returned by LsaOpenPolicy", ntsResult);
			dwError = LsaNtStatusToWinError(ntsResult);
			__leave;
		} 

		//  Initialize an LSA_UNICODE_STRING for the name of the
		//  private data.
		if (FAILED(StringCchPrintfW(szLsaKeyName, ARRAYSIZE(szLsaKeyName), L"%s_%08X", CREDENTIAL_LSAPREFIX, dwRid)))
		{
			dwError = ERROR_BUFFER_OVERFLOW;
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING, L"StringCchPrintfW failed for LSA key name");
			__leave;
		}

		lusSecretName.Buffer = szLsaKeyName;
		lusSecretName.Length = (USHORT) wcslen(szLsaKeyName)* sizeof(WCHAR);
		lusSecretName.MaximumLength = lusSecretName.Length;

		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Reading dwRid = 0x%x", dwRid);
		ntsResult = LsaRetrievePrivateData(LsaPolicyHandle,&lusSecretName,&pData);
		if( STATUS_SUCCESS != ntsResult )
		{
			if (0xc0000034 == ntsResult)
			{
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Private info not found for dwRid = 0x%x", dwRid);
			}
			else
			{
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%08x returned by LsaRetrievePrivateData", ntsResult);
			}
			dwError = LsaNtStatusToWinError(ntsResult);
			__leave;
		} 
		*ppPrivateData = (PEID_PRIVATE_DATA) EIDAlloc(pData->Length);
		if (!*ppPrivateData)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Error 0x%08x returned by EIDAlloc", GetLastError());
			__leave;
		}
		memcpy(*ppPrivateData, pData->Buffer, pData->Length);
		fReturn = TRUE;
	}
	__finally
	{
		if (LsaPolicyHandle) LsaClose(LsaPolicyHandle);
		if (pData) LsaFreeMemory(pData);
	}
	SetLastError(dwError);
	return fReturn;
}

BOOL CStoredCredentialManager::RetrievePrivateDataDebug(__in DWORD dwRid, __out PEID_PRIVATE_DATA *ppPrivateData)
{
	// SECURITY FIX: Debug credential retrieval from TEMP files is disabled
	// This was a critical security vulnerability (CWE-532) that read credentials from plaintext files
	// Credentials can only be retrieved securely via LSA private data storage
	UNREFERENCED_PARAMETER(dwRid);

	if (ppPrivateData)
	{
		*ppPrivateData = nullptr;
	}

	EIDCardLibraryTrace(WINEVENT_LEVEL_ERROR, L"SECURITY: Debug credential file retrieval is disabled - must run in LSA context");
	SetLastError(ERROR_ACCESS_DENIED);
	return FALSE;
}

BOOL CStoredCredentialManager::HasStoredCredential(__in DWORD dwRid)
{
	BOOL fReturn = FALSE;
	PEID_PRIVATE_DATA pSecret;
	DWORD dwError = 0;
	if (RetrievePrivateData(dwRid, &pSecret))
	{
		dwError = GetLastError();
		fReturn = TRUE;
		EIDFree(pSecret);
	}
	EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"%s",(fReturn?L"TRUE":L"FALSE"));
	SetLastError(dwError);
	return fReturn;
}
//////////////////////////////////////////////////////////////


struct ENCRYPTED_LM_OWF_PASSWORD {
    unsigned char data[16];
};
using PENCRYPTED_LM_OWF_PASSWORD = ENCRYPTED_LM_OWF_PASSWORD*;
using ENCRYPTED_NT_OWF_PASSWORD = ENCRYPTED_LM_OWF_PASSWORD;
using PENCRYPTED_NT_OWF_PASSWORD = ENCRYPTED_NT_OWF_PASSWORD*;

struct SAMPR_USER_INTERNAL1_INFORMATION {
    ENCRYPTED_NT_OWF_PASSWORD  EncryptedNtOwfPassword;
    ENCRYPTED_LM_OWF_PASSWORD  EncryptedLmOwfPassword;
    unsigned char              NtPasswordPresent;
    unsigned char              LmPasswordPresent;
    unsigned char              PasswordExpired;
};
using PSAMPR_USER_INTERNAL1_INFORMATION = SAMPR_USER_INTERNAL1_INFORMATION*;

enum USER_INFORMATION_CLASS {
    UserInternal1Information = 18,
};
using PUSER_INFORMATION_CLASS = USER_INFORMATION_CLASS*;

using PSAMPR_USER_INFO_BUFFER = PSAMPR_USER_INTERNAL1_INFORMATION;

using PSAMPR_SERVER_NAME = WCHAR*;
using SAMPR_HANDLE = PVOID;


// opnum 0
using SamrConnect = NTSTATUS (NTAPI*)(
    __in PSAMPR_SERVER_NAME ServerName,
    __out SAMPR_HANDLE * ServerHandle,
    __in DWORD DesiredAccess,
	__in DWORD
    );

// opnum 1
using SamrCloseHandle = NTSTATUS (NTAPI*)(
    __inout SAMPR_HANDLE * SamHandle
    );

// opnum 7
using SamrOpenDomain = NTSTATUS (NTAPI*)(
    __in SAMPR_HANDLE ServerHandle,
    __in DWORD   DesiredAccess,
    __in PSID DomainId,
    __out SAMPR_HANDLE * DomainHandle
    );


		// opnum 34
using SamrOpenUser = NTSTATUS (NTAPI*)(
    __in SAMPR_HANDLE DomainHandle,
    __in DWORD   DesiredAccess,
    __in DWORD   UserId,
    __out SAMPR_HANDLE  * UserHandle
    );

// opnum 36
using SamrQueryInformationUser = NTSTATUS (NTAPI*)(
    __in SAMPR_HANDLE UserHandle,
    __in USER_INFORMATION_CLASS  UserInformationClass,
	__out PSAMPR_USER_INFO_BUFFER * Buffer
    );

using SamIFree_SAMPR_USER_INFO_BUFFER = NTSTATUS (NTAPI*)(
	__in PSAMPR_USER_INFO_BUFFER Buffer,
	__in USER_INFORMATION_CLASS UserInformationClass
	);

HMODULE samsrvDll = nullptr;
SamrConnect MySamrConnect;
SamrCloseHandle MySamrCloseHandle;
SamrOpenDomain MySamrOpenDomain;
SamrOpenUser MySamrOpenUser;
SamrQueryInformationUser MySamrQueryInformationUser;
SamIFree_SAMPR_USER_INFO_BUFFER MySamIFree;


NTSTATUS LoadSamSrv()
{
	samsrvDll = EIDLoadSystemLibrary(TEXT("samsrv.dll"));
	if (!samsrvDll)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"LoadSam failed 0x%08x",GetLastError());
		return STATUS_FAIL_CHECK;
	}
	MySamrConnect = (SamrConnect) GetProcAddress(samsrvDll,"SamIConnect");
	MySamrCloseHandle = (SamrCloseHandle) GetProcAddress(samsrvDll,"SamrCloseHandle");
	MySamrOpenDomain = (SamrOpenDomain) GetProcAddress(samsrvDll,"SamrOpenDomain");
	MySamrOpenUser = (SamrOpenUser) GetProcAddress(samsrvDll,"SamrOpenUser");
	MySamrQueryInformationUser = (SamrQueryInformationUser) GetProcAddress(samsrvDll,"SamrQueryInformationUser");
	MySamIFree = (SamIFree_SAMPR_USER_INFO_BUFFER) GetProcAddress(samsrvDll,"SamIFree_SAMPR_USER_INFO_BUFFER");
	if (!MySamrConnect || !MySamrCloseHandle || !MySamrOpenDomain || !MySamrOpenUser
		|| !MySamrQueryInformationUser || !MySamIFree)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Null pointer function");
		FreeLibrary(samsrvDll);
		samsrvDll = nullptr;
		return STATUS_FAIL_CHECK;
	}
	return STATUS_SUCCESS;
}

NTSTATUS CStoredCredentialManager::CheckPassword( __in DWORD dwRid, __in PWSTR szPassword)
{
	NTSTATUS Status = STATUS_SUCCESS;
	LSA_OBJECT_ATTRIBUTES connectionAttrib;
    LSA_HANDLE handlePolicy = nullptr;
    PPOLICY_ACCOUNT_DOMAIN_INFO structInfoPolicy = nullptr;// -> http://msdn2.microsoft.com/en-us/library/ms721895(VS.85).aspx.
 SAMPR_HANDLE hSam = nullptr, hDomain = nullptr, hUser = nullptr;
 PSAMPR_USER_INTERNAL1_INFORMATION UserInfo = nullptr;
	unsigned char bHash[16];
	UNICODE_STRING EncryptedPassword;
	EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Enter");
	__try
	{
        samsrvDll = nullptr;
		memset(&connectionAttrib,0,sizeof(LSA_OBJECT_ATTRIBUTES));
        connectionAttrib.Length = sizeof(LSA_OBJECT_ATTRIBUTES);
		Status = LoadSamSrv();
		if (Status!= STATUS_SUCCESS)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"LoadSamSrv failed 0x%08x",Status);
			__leave;
		}
		Status = LsaOpenPolicy(nullptr,&connectionAttrib,POLICY_ALL_ACCESS,&handlePolicy);
		if (Status!= STATUS_SUCCESS)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"LsaOpenPolicy failed 0x%08x",Status);
			__leave;
		}
		Status = LsaQueryInformationPolicy(handlePolicy , PolicyAccountDomainInformation , (PVOID*)&structInfoPolicy);
		if (Status!= STATUS_SUCCESS)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"LsaQueryInformationPolicy failed 0x%08x",Status);
			__leave;
		}
		Status = MySamrConnect(nullptr , &hSam , MAXIMUM_ALLOWED, 1);  // nullptr is already correct
		if (Status!= STATUS_SUCCESS)	
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"SamrConnect failed 0x%08x",Status);
			__leave;
		}
		Status = MySamrOpenDomain(hSam , 0xf07ff , structInfoPolicy->DomainSid , &hDomain);
		if (Status!= STATUS_SUCCESS)	
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"SamrOpenDomain failed 0x%08x",Status);
			__leave;
		}
		Status = MySamrOpenUser(hDomain , MAXIMUM_ALLOWED , dwRid , &hUser);
		if (Status!= STATUS_SUCCESS)	
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"SamrOpenUser failed 0x%08x rid = %d",Status,dwRid);
			__leave;
		}
		Status = MySamrQueryInformationUser(hUser , UserInternal1Information , &UserInfo);
		if (Status!= STATUS_SUCCESS)	
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"SamrQueryInformationUser failed 0x%08x",Status);
			__leave;
		}
		EncryptedPassword.Length = (USHORT) wcslen(szPassword) * sizeof(WCHAR);
		EncryptedPassword.MaximumLength = (USHORT) wcslen(szPassword) * sizeof(WCHAR);
		EncryptedPassword.Buffer = szPassword;
		Status = SystemFunction007(&EncryptedPassword, bHash);
		if (Status!= STATUS_SUCCESS)	
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"SystemFunction007 failed 0x%08x",Status);
			__leave;
		}
		for (DWORD dwI = 0 ; dwI < 16; dwI++)
		{
			if (bHash[dwI] != UserInfo->EncryptedNtOwfPassword.data[dwI])
			{
				Status = STATUS_WRONG_PASSWORD;
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"STATUS_WRONG_PASSWORD");
				break;
			}
		}
	}
	__finally
	{
		if (UserInfo)
			MySamIFree(UserInfo, UserInternal1Information);
		if (hUser)
			MySamrCloseHandle(&hUser);
		if (hDomain)
			MySamrCloseHandle(&hDomain);
		if (hSam)
			MySamrCloseHandle(&hSam);
		if (structInfoPolicy)
			LsaFreeMemory(structInfoPolicy);
		if (handlePolicy)
			LsaClose(handlePolicy);
		if (samsrvDll)
			FreeLibrary(samsrvDll);
	}
	EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Leave with status = 0x%08x",Status);
	return Status;
}