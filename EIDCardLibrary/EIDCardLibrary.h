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

// Suppress C4005 warnings for macro redefinitions from Windows SDK
// These warnings occur because Windows SDK headers (ntstatus.h, winnt.h, WinCred.h, etc.)
// define the same macros in different orders when included from different paths.
#pragma warning(push)
#pragma warning(disable:4005)

#include <ntsecapi.h>

constexpr const char* AUTHENTICATIONPACKAGENAME = "EIDAuthenticationPackage";
constexpr const wchar_t* AUTHENTICATIONPACKAGENAMEW = L"EIDAuthenticationPackage";
#define AUTHENTICATIONPACKAGENAMET TEXT("EIDAuthenticationPackage")

// The Windows SDK (WinCred.h) defines CERT_HASH_LENGTH as 20 (SHA-1), but we use SHA-256 (32)
// Undefine first to ensure our definition takes precedence without warnings
#undef CERT_HASH_LENGTH
#define CERT_HASH_LENGTH 32  // SHA-256 hashes are used for cert hashes (security upgrade from SHA-1)

#pragma warning(pop)

#define EIDAlloc(value) EIDAllocEx(__FILE__,__LINE__,__FUNCTION__,value)
#define EIDFree(value) EIDFreeEx(__FILE__,__LINE__,__FUNCTION__,value)

PVOID EIDAllocEx(PCSTR szFile, DWORD dwLine, PCSTR szFunction,DWORD);
VOID EIDFreeEx(PCSTR szFile, DWORD dwLine, PCSTR szFunction,PVOID);
VOID EIDImpersonate();
VOID EIDRevertToSelf();
BOOL EIDIsComponentInLSAContext();

// Secure DLL loading - prevents DLL hijacking by using full system paths
// Use this instead of LoadLibrary for system DLLs
HMODULE EIDLoadSystemLibrary(LPCTSTR szDllName);

// Window utility functions shared across Wizard and Elevated projects
VOID CenterWindow(HWND hWnd);
VOID SetIcon(HWND hWnd);

enum EID_INTERACTIVE_LOGON_SUBMIT_TYPE
{
	EID_INTERACTIVE_LOGON_SUBMIT_TYPE_VANILLA = 13, //KerbCertificateLogon = 13
};

struct EID_INTERACTIVE_LOGON
{
    EID_INTERACTIVE_LOGON_SUBMIT_TYPE MessageType; // KerbCertificateLogon
    UNICODE_STRING LogonDomainName;
    UNICODE_STRING UserName;
    UNICODE_STRING Pin;
	ULONG Flags;               // additional flags
    ULONG CspDataLength;
    PUCHAR CspData;            // contains the smartcard CSP data
};
using PEID_INTERACTIVE_LOGON = EID_INTERACTIVE_LOGON*;

struct EID_INTERACTIVE_UNLOCK_LOGON
{
    EID_INTERACTIVE_LOGON Logon;
    LUID LogonId;
};
using PEID_INTERACTIVE_UNLOCK_LOGON = EID_INTERACTIVE_UNLOCK_LOGON*;

enum EID_PROFILE_BUFFER_TYPE
{
	EIDInteractiveProfile = 2,
};

#pragma pack(push, EID_SMARTCARD_CSP_INFO, 1)
// based on _KERB_SMARTCARD_CSP_INFO
struct EID_SMARTCARD_CSP_INFO
{
  DWORD dwCspInfoLen;
  DWORD MessageType;
  union {
    PVOID ContextInformation;
    ULONG64 SpaceHolderForWow64;
  } ;
  DWORD flags;
  DWORD KeySpec;
  ULONG nCardNameOffset;
  ULONG nReaderNameOffset;
  ULONG nContainerNameOffset;
  ULONG nCSPNameOffset;
  TCHAR bBuffer[sizeof(DWORD)];
};
using PEID_SMARTCARD_CSP_INFO = EID_SMARTCARD_CSP_INFO*;
#pragma pack(pop, EID_SMARTCARD_CSP_INFO)

struct EID_INTERACTIVE_PROFILE
{
  EID_PROFILE_BUFFER_TYPE MessageType;
  USHORT LogonCount;
  USHORT BadPasswordCount;
  LARGE_INTEGER LogonTime;
  LARGE_INTEGER LogoffTime;
  LARGE_INTEGER KickOffTime;
  LARGE_INTEGER PasswordLastSet;
  LARGE_INTEGER PasswordCanChange;
  LARGE_INTEGER PasswordMustChange;
  UNICODE_STRING LogonScript;
  UNICODE_STRING HomeDirectory;
  UNICODE_STRING FullName;
  UNICODE_STRING ProfilePath;
  UNICODE_STRING HomeDirectoryDrive;
  UNICODE_STRING LogonServer;
  ULONG UserFlags;
};
using PEID_INTERACTIVE_PROFILE = EID_INTERACTIVE_PROFILE*;

enum EID_CREDENTIAL_PROVIDER_READER_STATE
{
	EIDCPRSConnecting,
	EIDCPRSConnected,
	EIDCPRSDisconnected,
	EIDCPRSThreadFinished,
};

enum EID_CALLPACKAGE_MESSAGE
{
	EIDCMCreateStoredCredential,
	EIDCMUpdateStoredCredential,
	EIDCMRemoveStoredCredential,
	EIDCMHasStoredCredential,
	EIDCMRemoveAllStoredCredential,
	EIDCMGetStoredCredentialRid,
	EIDCMEIDGinaAuthenticationChallenge,
	EIDCMEIDGinaAuthenticationResponse,
};

//Message used for LsaApCallPackage
struct EID_CALLPACKAGE_BUFFER
{
	EID_CALLPACKAGE_MESSAGE MessageType;
	DWORD dwError;
	DWORD dwRid;
	PWSTR szPassword;		// used if EIDCMCreateStoredCredential
	USHORT usPasswordLen;	// can be 0 if null terminated
	PBYTE pbCertificate;
	USHORT dwCertificateSize;
	UCHAR Hash[CERT_HASH_LENGTH]; // to get challenge
	BOOL fEncryptPassword;

};
using PEID_CALLPACKAGE_BUFFER = EID_CALLPACKAGE_BUFFER*;

struct EID_MSGINA_AUTHENTICATION_CHALLENGE_REQUEST
{
	EID_CALLPACKAGE_MESSAGE MessageType;
    DWORD dwRid;
};
using PEID_MSGINA_AUTHENTICATION_CHALLENGE_REQUEST = EID_MSGINA_AUTHENTICATION_CHALLENGE_REQUEST*;

struct EID_MSGINA_AUTHENTICATION_CHALLENGE_ANSWER
{
	DWORD dwError;
	DWORD dwChallengeType;
	PBYTE pbChallenge;
	DWORD dwChallengeSize;
};
using PEID_MSGINA_AUTHENTICATION_CHALLENGE_ANSWER = EID_MSGINA_AUTHENTICATION_CHALLENGE_ANSWER*;

struct EID_MSGINA_AUTHENTICATION_RESPONSE_REQUEST
{
	EID_CALLPACKAGE_MESSAGE MessageType;
    DWORD dwRid;
	DWORD dwChallengeType;
	PBYTE pbChallenge;
	DWORD dwChallengeSize;
	PBYTE pbResponse;
	DWORD dwResponseSize;
};
using PEID_MSGINA_AUTHENTICATION_RESPONSE_REQUEST = EID_MSGINA_AUTHENTICATION_RESPONSE_REQUEST*;

struct EID_MSGINA_AUTHENTICATION_RESPONSE_ANSWER
{
	DWORD dwError;
	UNICODE_STRING Password;
};
using PEID_MSGINA_AUTHENTICATION_RESPONSE_ANSWER = EID_MSGINA_AUTHENTICATION_RESPONSE_ANSWER*;


constexpr DWORD EID_CERTIFICATE_FLAG_USERSTORE = 0x00000001;

struct EID_NEGOCIATE_MESSAGE
{
	BYTE Signature[8];
	DWORD MessageType;
	DWORD Flags;
	USHORT TargetLen;
	USHORT TargetMaxLen;
	DWORD TargetOffset;
	USHORT WorkstationLen;
	USHORT WorkstationMaxLen;
	USHORT WorkstationOffset;
	UCHAR Hash[CERT_HASH_LENGTH];
	DWORD Version;
};
using PEID_NEGOCIATE_MESSAGE = EID_NEGOCIATE_MESSAGE*;

struct EID_CHALLENGE_MESSAGE
{
	BYTE Signature[8];
	DWORD MessageType;
	DWORD Flags;
	DWORD UsernameLen;
	DWORD UsernameOffset;
	DWORD ChallengeLen;
	DWORD ChallengeOffset;
	DWORD Version;
};
using PEID_CHALLENGE_MESSAGE = EID_CHALLENGE_MESSAGE*;

struct EID_RESPONSE_MESSAGE
{
	BYTE Signature[8];
	DWORD MessageType;
	DWORD ResponseLen;
	DWORD ResponseOffset;
	DWORD Version;
};
using PEID_RESPONSE_MESSAGE = EID_RESPONSE_MESSAGE*;

enum EID_MESSAGE_STATE
{
	EIDMSNone,
	EIDMSNegociate,
	EIDMSChallenge,
	EIDMSResponse,
	EIDMSComplete,
};

enum EID_MESSAGE_TYPE
{
	EIDMTNegociate = 1,
	EIDMTChallenge = 2,
	EIDMTResponse = 3,
};

constexpr DWORD EID_MESSAGE_VERSION = 1;
// Signature is exactly 8 bytes (7 chars + null terminator) to match message Signature[8] fields
constexpr char EID_MESSAGE_SIGNATURE[8] = "EIDAuth";

enum EID_SSP_CALLER
{
	EIDSSPInitialize,
	EIDSSPAccept,
};

struct EID_SSP_CALLBACK_MESSAGE
{
	EID_SSP_CALLER Caller;
	HANDLE hToken;
};
using PEID_SSP_CALLBACK_MESSAGE = EID_SSP_CALLBACK_MESSAGE*;