#ifndef __CREDENTIALMANAGEMENT_H__
#define __CREDENTIALMANAGEMENT_H__

#include <ntstatus.h>
#include <list>

#include "CertificateValidation.h"

// Forward declaration - CCredential is defined below after CSecurityContext
class CCredential;

class CSecurityContext
{
public:
	static CSecurityContext* CSecurityContext::CreateContext(CCredential* pCredential);
	explicit CSecurityContext(CCredential* pCredential);
	CSecurityContext(const CSecurityContext&) = delete;
	CSecurityContext& operator=(const CSecurityContext&) = delete;
	static BOOL Delete(ULONG_PTR pHandle);
	static CSecurityContext* GetContextFromHandle(ULONG_PTR);
	NTSTATUS InitializeSecurityContextInput(PSecBufferDesc);
	NTSTATUS InitializeSecurityContextOutput(PSecBufferDesc);
	NTSTATUS AcceptSecurityContextInput(PSecBufferDesc);
	NTSTATUS AcceptSecurityContextOutput(PSecBufferDesc);
	NTSTATUS BuildNegociateMessage(PSecBufferDesc Buffer);
	NTSTATUS ReceiveNegociateMessage(PSecBufferDesc Buffer);
	NTSTATUS BuildChallengeMessage(PSecBufferDesc Buffer);
	NTSTATUS ReceiveChallengeMessage(PSecBufferDesc Buffer);
	NTSTATUS BuildResponseMessage(PSecBufferDesc Buffer);
	NTSTATUS ReceiveResponseMessage(PSecBufferDesc Buffer);
	NTSTATUS BuildCompleteMessage(PSecBufferDesc Buffer);
	DWORD GetRid();
	~CSecurityContext();
	PWSTR GetUserName();
private:
	CCredential* _pCredential;
	EID_MESSAGE_STATE _State;
	UCHAR Hash[CERT_HASH_LENGTH];
	PCCERT_CONTEXT pCertContext;
	DWORD dwRid;
	PBYTE pbChallenge;
	DWORD dwChallengeSize;
	PBYTE pbResponse;
	DWORD dwResponseSize;
	PWSTR szUserName;
};

class CCredential
{
public:
	CCredential(PLUID LogonIdToUse, PCERT_CREDENTIAL_INFO pCertInfo,PWSTR szPin, ULONG CredentialUseFlags);
	ULONG Use;
	static BOOL Delete(ULONG_PTR pHandle);
	static CCredential* GetCredentialFromHandle(ULONG_PTR);
	std::list<CSecurityContext> _Contexts;
	PTSTR GetName();
	static CCredential* CreateCredential(PLUID LogonIdToUse, PCERT_CREDENTIAL_INFO pCertInfo,PWSTR szPin, ULONG CredentialUseFlags);
	BOOL Check(PLUID LogonId)
	{
		return (LogonId != nullptr) && (_LogonId.HighPart == LogonId->HighPart) && (_LogonId.LowPart == LogonId->LowPart);
	}
	~CCredential();
	LUID _LogonId;
	UCHAR _rgbHashOfCert[CERT_HASH_LENGTH];
	PWSTR _szPin;
	DWORD _dwLen;
	PCERT_CREDENTIAL_INFO _pCertInfo;
};

class CUsermodeContext
{
public:
	static NTSTATUS AddContextInfo(ULONG_PTR pHandle, PEID_SSP_CALLBACK_MESSAGE pMessage);
	static NTSTATUS DeleteContextInfo(ULONG_PTR pHandle);
	static NTSTATUS GetImpersonationHandle(ULONG_PTR pHandle,PHANDLE ImpersonationToken);
private:
	static CUsermodeContext* CUsermodeContext::GetContextFromHandle(ULONG_PTR Handle);
	HANDLE Handle;
	explicit CUsermodeContext(PEID_SSP_CALLBACK_MESSAGE pMessage);
};

#endif
