#include <ntstatus.h>
#define WIN32_NO_STATUS 1

#include <Windows.h>
#include <assert.h>
#define SECURITY_WIN32
#include <sspi.h>
#include <wincred.h>
#include <NTSecAPI.h>
#include <NTSecPKG.h>
#include <LM.h>
#include <set>
#include <map>
#include "../EIDCardLibrary/EIDCardLibrary.h"
#include "../EIDCardLibrary/Tracing.h"
#include "../EIDCardLibrary/StoredCredentialManagement.h"
#include "../EIDCardLibrary/CertificateUtilities.h"
#include "credentialManagement.h"

#pragma comment(lib,"Winscard")
#pragma comment(lib,"Cryptui")

std::set<CCredential*> Credentials;
std::list<CSecurityContext*> Contexts;
std::map<ULONG_PTR, CUsermodeContext*> UserModeContexts;
using Credential_Pair = std::pair<LUID, CCredential*>;

// Critical section for thread-safe access to credential containers (CWE-416 fix for #24)
static CRITICAL_SECTION g_CredentialLock;

// Static initializer to ensure critical section is initialized before use
class CredentialLockInitializer {
public:
    CredentialLockInitializer() { InitializeCriticalSection(&g_CredentialLock); }
    ~CredentialLockInitializer() { DeleteCriticalSection(&g_CredentialLock); }
};
static CredentialLockInitializer g_CredentialLockInit;


CCredential* CCredential::CreateCredential(PLUID LogonIdToUse, PCERT_CREDENTIAL_INFO pCertInfo,PWSTR szPin, ULONG CredentialUseFlags)
{
	CCredential* credential = nullptr;

	if (!LogonIdToUse)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"LogonIdToUse NULL");
		return nullptr;
	}

	EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"new Credential");
	credential = new CCredential(LogonIdToUse,pCertInfo,szPin, CredentialUseFlags);

	EnterCriticalSection(&g_CredentialLock);
	Credentials.insert(credential);
	LeaveCriticalSection(&g_CredentialLock);

	return credential;
}

CCredential::CCredential(PLUID LogonIdToUse, PCERT_CREDENTIAL_INFO pCertInfo,PWSTR szPin, ULONG CredentialUseFlags)
{
	if (szPin)
	{
		_dwLen = (DWORD) wcslen(szPin) + 1;
		_szPin = new WCHAR[_dwLen];
		wcscpy_s(_szPin,_dwLen, szPin);
	}
	else
	{
		_dwLen = 0;
		_szPin = nullptr;
	}
	_LogonId = *LogonIdToUse;
	Use = CredentialUseFlags;
	// certinfo
	if (pCertInfo)
	{
		// Windows SDK defines CERT_CREDENTIAL_INFO.rgbHashOfCert with the SDK's CERT_HASH_LENGTH (20 bytes for SHA-1)
		// Our internal CERT_HASH_LENGTH is 32 (SHA-256). Only copy what the SDK structure actually contains.
		constexpr size_t SDK_CERT_HASH_LENGTH = 20;
		memset(_rgbHashOfCert, 0, sizeof(_rgbHashOfCert));
		memcpy_s(_rgbHashOfCert, sizeof(_rgbHashOfCert), pCertInfo->rgbHashOfCert, SDK_CERT_HASH_LENGTH);
		_pCertInfo = (PCERT_CREDENTIAL_INFO) EIDAlloc(pCertInfo->cbSize);
		memcpy(_pCertInfo, pCertInfo, pCertInfo->cbSize);
	}
	else
	{
		_pCertInfo = nullptr;
	}

}

CCredential::~CCredential()
{
	if (_szPin)
	{
		SecureZeroMemory(_szPin, _dwLen * sizeof(WCHAR));
		delete[] _szPin;
	}
	if (_pCertInfo)
	{
		EIDFree(_pCertInfo);
	}
}

BOOL CCredential::Delete(ULONG_PTR phCredential)
{
	CCredential* testedCredential = (CCredential*) phCredential;
	CCredential* toDelete = nullptr;

	EnterCriticalSection(&g_CredentialLock);
	for ( auto iter = Credentials.begin( ); iter != Credentials.end( ); iter++ )
	{
		CCredential* currentCredential = *iter;
		if (currentCredential == testedCredential)
		{
			toDelete = testedCredential;
			Credentials.erase(iter);  // Remove from container first
			break;
		}
	}
	LeaveCriticalSection(&g_CredentialLock);

	if (toDelete)
	{
		delete toDelete;  // Delete outside of lock
		return TRUE;
	}
	return FALSE;
}

CCredential* CCredential::GetCredentialFromHandle(ULONG_PTR CredentialHandle)
{
	CCredential* pCredential = (CCredential*) CredentialHandle;
	CCredential* result = nullptr;

	EnterCriticalSection(&g_CredentialLock);
	for ( auto iter = Credentials.begin( ); iter != Credentials.end( ); iter++ )
	{
		CCredential* currentCredential = *iter;
		if (currentCredential == pCredential)
		{
			result = currentCredential;
			break;
		}
	}
	LeaveCriticalSection(&g_CredentialLock);

	if (!result)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"pCredential = %p not Found",pCredential);
	}
	return result;
}

PTSTR CCredential::GetName()
{
	return nullptr;
}

CSecurityContext* CSecurityContext::CreateContext(CCredential* pCredential)
{
	CSecurityContext* context = nullptr;
	if (!pCredential)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"pCredential NULL");
		return nullptr;
	}
	context = new CSecurityContext(pCredential);

	EnterCriticalSection(&g_CredentialLock);
	Contexts.push_back(context);
	LeaveCriticalSection(&g_CredentialLock);

	return context;
}


CSecurityContext::CSecurityContext(CCredential* pCredential)
{
	_pCredential = pCredential;
	_State = EID_MESSAGE_STATE::EIDMSNone;
	pbChallenge = nullptr;
	pbResponse = nullptr;
	dwChallengeSize = 0;
	dwResponseSize = 0;
	dwRid = 0;
	pCertContext = nullptr;
	szUserName = nullptr;
	if (pCredential && pCredential->_pCertInfo)
	{
		CRYPT_DATA_BLOB blob;
		blob.pbData = pCredential->_pCertInfo->rgbHashOfCert;
		blob.cbData = CERT_HASH_LENGTH;
		pCertContext = FindCertificateFromHash(&blob);
	}
}

BOOL CSecurityContext::Delete(ULONG_PTR phContext)
{
	CSecurityContext* testedContext = (CSecurityContext*) phContext;
	CSecurityContext* toDelete = nullptr;

	EnterCriticalSection(&g_CredentialLock);
	for ( auto iter = Contexts.begin( ); iter != Contexts.end( ); iter++ )
	{
		CSecurityContext* currentContext = *iter;
		if (currentContext == testedContext)
		{
			toDelete = testedContext;
			Contexts.erase(iter);  // Remove from container first
			break;
		}
	}
	LeaveCriticalSection(&g_CredentialLock);

	if (toDelete)
	{
		delete toDelete;  // Delete outside of lock
		return TRUE;
	}
	return FALSE;
}

CSecurityContext* CSecurityContext::GetContextFromHandle(ULONG_PTR context)
{
	CSecurityContext* testedContext = (CSecurityContext*) context;
	CSecurityContext* result = nullptr;

	EnterCriticalSection(&g_CredentialLock);
	for ( auto iter = Contexts.begin( ); iter != Contexts.end( ); iter++ )
	{
		CSecurityContext* currentContext = *iter;
		if (currentContext == testedContext)
		{
			result = currentContext;
			break;
		}
	}
	LeaveCriticalSection(&g_CredentialLock);

	return result;
}

NTSTATUS CSecurityContext::InitializeSecurityContextInput(PSecBufferDesc Buffer)
{
	NTSTATUS Status = STATUS_INVALID_SIGNATURE;
	switch (_State)
	{
		case EID_MESSAGE_STATE::EIDMSNegociate:
			Status = ReceiveChallengeMessage(Buffer);
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Init   Input  EIDMSNegociate Status = 0x%08X", Status);
			break;
		default:
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Init   Input  default Status = 0x%08X", Status);
			break;
	}
	return Status;
}
NTSTATUS CSecurityContext::InitializeSecurityContextOutput(PSecBufferDesc Buffer)
{
	NTSTATUS Status = STATUS_INVALID_SIGNATURE;
	switch (_State)
	{
		case EID_MESSAGE_STATE::EIDMSNone:
			Status = BuildNegociateMessage(Buffer);
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Init   Output EIDMSNone Status = 0x%08X", Status);
			break;
		case EID_MESSAGE_STATE::EIDMSChallenge:
			Status = BuildResponseMessage(Buffer);
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Init   Output EIDMSChallenge Status = 0x%08X", Status);
			break;
		default:
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Init   Output default Status = 0x%08X", Status);
			break;
	}
	return Status;
}
NTSTATUS CSecurityContext::AcceptSecurityContextInput(PSecBufferDesc Buffer)
{
	NTSTATUS Status = STATUS_INVALID_SIGNATURE;
	switch (_State)
	{
		case EID_MESSAGE_STATE::EIDMSNone:
			Status = ReceiveNegociateMessage(Buffer);
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Accept Input  EIDMSNone Status = 0x%08X", Status);
			break;
		case EID_MESSAGE_STATE::EIDMSChallenge:
			Status = ReceiveResponseMessage(Buffer);
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Accept Input  EIDMSChallenge Status = 0x%08X", Status);
			break;
		default:
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Accept Input  default Status = 0x%08X", Status);
			break;
	}
	return Status;
}
NTSTATUS CSecurityContext::AcceptSecurityContextOutput(PSecBufferDesc Buffer)
{
	NTSTATUS Status = STATUS_INVALID_SIGNATURE;
	switch (_State)
	{
		case EID_MESSAGE_STATE::EIDMSNegociate:
			Status = BuildChallengeMessage(Buffer);
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Accept Output EIDMSNegociate Status = 0x%08X", Status);
			break;
		case EID_MESSAGE_STATE::EIDMSComplete:
			Status = BuildCompleteMessage(Buffer);
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Accept Output EIDMSComplete Status = 0x%08X", Status);
			break;
		default:
			EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Accept Output default Status = 0x%08X", Status);
			break;
	}
	return Status;
}

NTSTATUS CSecurityContext::BuildNegociateMessage(PSecBufferDesc Buffer)
{
	Buffer->pBuffers[0].BufferType = SECBUFFER_TOKEN;
	if (Buffer->pBuffers[0].cbBuffer < 300)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"SEC_E_INSUFFICIENT_MEMORY");
		return SEC_E_INSUFFICIENT_MEMORY;
	}
	PEID_NEGOCIATE_MESSAGE message = (PEID_NEGOCIATE_MESSAGE) Buffer->pBuffers[0].pvBuffer;
	memset(message, 0, sizeof(EID_NEGOCIATE_MESSAGE));
	static_assert(sizeof(message->Signature) == sizeof(EID_MESSAGE_SIGNATURE), "Signature buffer sizes must match");
	memcpy_s(message->Signature, sizeof(message->Signature), EID_MESSAGE_SIGNATURE, sizeof(EID_MESSAGE_SIGNATURE));
	message->MessageType = static_cast<DWORD>(EID_MESSAGE_TYPE::EIDMTNegociate);
	message->Version = EID_MESSAGE_VERSION;
	static_assert(sizeof(Hash) == sizeof(_pCredential->_rgbHashOfCert), "Hash buffer sizes must match");
	memcpy_s(Hash, sizeof(Hash), _pCredential->_rgbHashOfCert, sizeof(Hash));
	memcpy_s(message->Hash, sizeof(message->Hash), _pCredential->_rgbHashOfCert, sizeof(message->Hash));
	_State = EID_MESSAGE_STATE::EIDMSNegociate;
	return SEC_I_CONTINUE_NEEDED;
}

NTSTATUS CSecurityContext::ReceiveNegociateMessage(PSecBufferDesc Buffer)
{
	if (Buffer->pBuffers[0].cbBuffer < sizeof(EID_NEGOCIATE_MESSAGE))
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"SEC_E_INSUFFICIENT_MEMORY");
		return SEC_E_INSUFFICIENT_MEMORY;
	}
	PEID_NEGOCIATE_MESSAGE message = (PEID_NEGOCIATE_MESSAGE) Buffer->pBuffers[0].pvBuffer;
	if (message->MessageType != static_cast<DWORD>(EID_MESSAGE_TYPE::EIDMTNegociate))
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Incorrect messageType");
		return STATUS_INVALID_SIGNATURE;
	}
	static_assert(sizeof(EID_MESSAGE_SIGNATURE) == sizeof(message->Signature), "Signature buffer sizes must match");
	if (memcmp(EID_MESSAGE_SIGNATURE, message->Signature, sizeof(message->Signature)) != 0)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"STATUS_INVALID_SIGNATURE");
		return STATUS_INVALID_SIGNATURE;
	}

	static_assert(sizeof(Hash) == sizeof(message->Hash), "Hash buffer sizes must match");
	memcpy_s(Hash, sizeof(Hash), message->Hash, sizeof(Hash));
	_State = EID_MESSAGE_STATE::EIDMSNegociate;
	return STATUS_SUCCESS;
}

NTSTATUS CSecurityContext::BuildChallengeMessage(PSecBufferDesc Buffer)
{
	DWORD dwEntriesRead;
	DWORD dwTotalEntries;
	DWORD dwI;
	USER_INFO_3 *pInfo = nullptr;
	NTSTATUS Status = STATUS_SUCCESS;
	__try
	{
		Buffer->pBuffers[0].BufferType = SECBUFFER_TOKEN;
		if (Buffer->pBuffers[0].cbBuffer < 300)
		{
			Status = SEC_E_INSUFFICIENT_MEMORY;
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"SEC_E_INSUFFICIENT_MEMORY");
			__leave;
		}
		PEID_CHALLENGE_MESSAGE message = (PEID_CHALLENGE_MESSAGE) Buffer->pBuffers[0].pvBuffer;
		memset(message, 0, sizeof(EID_CHALLENGE_MESSAGE));
		static_assert(sizeof(message->Signature) == sizeof(EID_MESSAGE_SIGNATURE), "Signature buffer sizes must match");
		memcpy_s(message->Signature, sizeof(message->Signature), EID_MESSAGE_SIGNATURE, sizeof(EID_MESSAGE_SIGNATURE));
		message->MessageType = static_cast<DWORD>(EID_MESSAGE_TYPE::EIDMTChallenge);
		message->Version = EID_MESSAGE_VERSION;
		CStoredCredentialManager* manager = CStoredCredentialManager::Instance();
		if (!manager->GetCertContextFromHash(Hash, &pCertContext, &dwRid))
		{
			Status = SEC_E_UNKNOWN_CREDENTIALS;
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"SEC_E_UNKNOWN_CREDENTIALS");
			__leave;
		}
		// get username
		Status = NetUserEnum(nullptr, 3, 0, (PBYTE*)&pInfo, MAX_PREFERRED_LENGTH, &dwEntriesRead,&dwTotalEntries, nullptr);
		if (Status != NERR_Success)
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"NetUserEnum = 0x%08X",Status);
			__leave;
		}
		for (dwI = 0; dwI < dwEntriesRead; dwI++)
		{
			if ( pInfo[dwI].usri3_user_id == dwRid)
			{
				DWORD dwLen= (DWORD)(wcslen(pInfo[dwI].usri3_name)+1);
				szUserName = (PWSTR) EIDAlloc(dwLen*sizeof(WCHAR));
				if (!szUserName)
				{
					EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"No memory");
					__leave;
				}
				wcscpy_s(szUserName, dwLen, pInfo[dwI].usri3_name);
				break;
			}
		}
		if (dwI >= dwEntriesRead)
		{
			Status = SEC_E_INTERNAL_ERROR;
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Userid not found");
			__leave;
		}
		if (!manager->GetSignatureChallenge(&pbChallenge, &dwChallengeSize))
		{
			Status = SEC_E_INTERNAL_ERROR;
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"GetSignatureChallenge 0x%08x",GetLastError());
			__leave;
		}
		message->ChallengeLen = dwChallengeSize;
		message->ChallengeOffset = sizeof(EID_CHALLENGE_MESSAGE);
		memcpy((PBYTE)message + message->ChallengeOffset, pbChallenge, dwChallengeSize);
		message->UsernameLen = (DWORD)wcslen(szUserName) * sizeof(WCHAR);
		message->UsernameOffset = message->ChallengeOffset + message->ChallengeLen;
		memcpy((PBYTE)message + message->UsernameOffset,szUserName,message->UsernameLen);
		_State = EID_MESSAGE_STATE::EIDMSChallenge;
	}
	__finally
	{
		// SEH cleanup - no action needed
	}
	return SEC_I_CONTINUE_NEEDED;
}

NTSTATUS CSecurityContext::ReceiveChallengeMessage(PSecBufferDesc Buffer)
{
	PEID_CHALLENGE_MESSAGE message = (PEID_CHALLENGE_MESSAGE) Buffer->pBuffers[0].pvBuffer;
	if (message->MessageType != static_cast<DWORD>(EID_MESSAGE_TYPE::EIDMTChallenge))
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Incorrect messageType");
		return STATUS_INVALID_SIGNATURE;
	}
	static_assert(sizeof(EID_MESSAGE_SIGNATURE) == sizeof(message->Signature), "Signature buffer sizes must match");
	if (memcmp(EID_MESSAGE_SIGNATURE, message->Signature, sizeof(message->Signature)) != 0)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"STATUS_INVALID_SIGNATURE");
		return STATUS_INVALID_SIGNATURE;
	}

	szUserName = (PWSTR) EIDAlloc(message->UsernameLen + sizeof(WCHAR));
	memcpy(szUserName, (PBYTE) message + message->UsernameOffset, message->UsernameLen);
	memset((PBYTE) szUserName + message->UsernameLen,0,sizeof(WCHAR));
	pbChallenge = (PBYTE) EIDAlloc(message->ChallengeLen);
	memcpy(pbChallenge, (PBYTE)message + message->ChallengeOffset, message->ChallengeLen);
	dwChallengeSize = message->ChallengeLen;
	_State = EID_MESSAGE_STATE::EIDMSChallenge;
	return STATUS_SUCCESS;
}

NTSTATUS CSecurityContext::BuildResponseMessage(PSecBufferDesc Buffer)
{
	Buffer->pBuffers[0].BufferType = SECBUFFER_TOKEN;
	if (Buffer->pBuffers[0].cbBuffer < 300)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"SEC_E_INSUFFICIENT_MEMORY");
		return SEC_E_INSUFFICIENT_MEMORY;
	}
	PEID_RESPONSE_MESSAGE message = (PEID_RESPONSE_MESSAGE) Buffer->pBuffers[0].pvBuffer;
	memset(message, 0, sizeof(EID_RESPONSE_MESSAGE));
	static_assert(sizeof(message->Signature) == sizeof(EID_MESSAGE_SIGNATURE), "Signature buffer sizes must match");
	memcpy_s(message->Signature, sizeof(message->Signature), EID_MESSAGE_SIGNATURE, sizeof(EID_MESSAGE_SIGNATURE));
	message->MessageType = static_cast<DWORD>(EID_MESSAGE_TYPE::EIDMTResponse);
	message->Version = EID_MESSAGE_VERSION;
	CStoredCredentialManager* manager = CStoredCredentialManager::Instance();
	if (!manager->GetResponseFromSignatureChallenge(pbChallenge, dwChallengeSize, pCertContext,_pCredential->_szPin, &pbResponse, &dwResponseSize))
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"SEC_E_LOGON_DENIED");
		return SEC_E_LOGON_DENIED;
	}
	message->ResponseLen = dwResponseSize;
	message->ResponseOffset = sizeof(EID_RESPONSE_MESSAGE);
	memcpy((PBYTE)message + message->ResponseOffset, pbResponse, dwResponseSize);
	_State = EID_MESSAGE_STATE::EIDMSComplete;
	return STATUS_SUCCESS;
}

NTSTATUS CSecurityContext::ReceiveResponseMessage(PSecBufferDesc Buffer)
{
	PEID_RESPONSE_MESSAGE message = (PEID_RESPONSE_MESSAGE) Buffer->pBuffers[0].pvBuffer;
	if (message->MessageType != static_cast<DWORD>(EID_MESSAGE_TYPE::EIDMTResponse))
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Incorrect messageType");
		return STATUS_INVALID_SIGNATURE;
	}
	static_assert(sizeof(EID_MESSAGE_SIGNATURE) == sizeof(message->Signature), "Signature buffer sizes must match");
	if (memcmp(EID_MESSAGE_SIGNATURE, message->Signature, sizeof(message->Signature)) != 0)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"STATUS_INVALID_SIGNATURE");
		return STATUS_INVALID_SIGNATURE;
	}

	pbResponse = (PBYTE) EIDAlloc(message->ResponseLen);
	dwResponseSize = message->ResponseLen;
	memcpy(pbResponse, (PBYTE)message + message->ResponseOffset, dwResponseSize);
	_State = EID_MESSAGE_STATE::EIDMSComplete;
	return STATUS_SUCCESS;
}

NTSTATUS CSecurityContext::BuildCompleteMessage(PSecBufferDesc Buffer)
{
	// v�rification du challenge
	UNREFERENCED_PARAMETER(Buffer);
	CStoredCredentialManager* manager = CStoredCredentialManager::Instance();
	if (!manager->VerifySignatureChallengeResponse(dwRid, pbChallenge, dwChallengeSize, pbResponse, dwResponseSize))
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"SEC_E_LOGON_DENIED");
		return SEC_E_LOGON_DENIED;
	}
	return STATUS_SUCCESS;
}

DWORD CSecurityContext::GetRid()
{
	return dwRid;
}

PWSTR CSecurityContext::GetUserName()
{
	if (!szUserName)
		return nullptr;
	DWORD dwLen = (DWORD) wcslen(szUserName) + 1;
	PWSTR szString = (PWSTR) EIDAlloc(dwLen * sizeof(WCHAR));
	if (!szString) return nullptr;
	wcscpy_s(szString,dwLen,szUserName);
	return szString;
}

CSecurityContext::~CSecurityContext()
{
	if (pbChallenge)
	{
		SecureZeroMemory(pbChallenge, dwChallengeSize);
		EIDFree(pbChallenge);
	}
	if (pbResponse)
	{
		SecureZeroMemory(pbResponse, dwResponseSize);
		EIDFree(pbResponse);
	}
	if (szUserName)
	{
		EIDFree(szUserName);
	}
	if (pCertContext)
	{
		CertFreeCertificateContext(pCertContext);
	}
}

CUsermodeContext::CUsermodeContext(PEID_SSP_CALLBACK_MESSAGE pMessage)
{
	Handle = pMessage->hToken;
	EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Token = 0x%08X", Handle);
}

NTSTATUS CUsermodeContext::AddContextInfo(ULONG_PTR pHandle, PEID_SSP_CALLBACK_MESSAGE pMessage)
{
	NTSTATUS Status = STATUS_SUCCESS;
	CUsermodeContext* pContext = GetContextFromHandle(pHandle);
	if (!pContext)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Inserting context 0x%08X", pHandle);
		pContext = new CUsermodeContext(pMessage);
		UserModeContexts.insert(std::pair<ULONG_PTR,CUsermodeContext*> (pHandle, pContext));
	}
	return Status ;
}

NTSTATUS CUsermodeContext::DeleteContextInfo(ULONG_PTR pHandle)
{
	// C++17 init-statement: it is only used within this if/else block
	if (auto it = UserModeContexts.find(pHandle); it != UserModeContexts.end())
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Context 0X%08X deleted", pHandle);
		UserModeContexts.erase(it);
		return STATUS_SUCCESS;
	}
	else
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"SEC_E_INVALID_HANDLE 0X%08X", pHandle);
		return SEC_E_INVALID_HANDLE;
	}
}

NTSTATUS CUsermodeContext::GetImpersonationHandle(ULONG_PTR pHandle,PHANDLE ImpersonationToken)
{
	NTSTATUS Status = STATUS_SUCCESS;
	CUsermodeContext* pContext = GetContextFromHandle(pHandle);
	if (!pContext)
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"SEC_E_INVALID_HANDLE 0X%08X", pHandle);
		return SEC_E_INVALID_HANDLE;
	}
	*ImpersonationToken = pContext->Handle;
	return Status ;
}

CUsermodeContext* CUsermodeContext::GetContextFromHandle(ULONG_PTR pHandle)
{
	// C++17 init-statement: it is only used within this if/else block
	if (auto it = UserModeContexts.find(pHandle); it != UserModeContexts.end())
	{
		return (*it).second;
	}
	else
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING,L"Context not found = 0x%08X", pHandle);
		return nullptr;
	}
}