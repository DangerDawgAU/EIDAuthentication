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


BOOL IsEIDPackageAvailable();

HRESULT LsaInitString(PSTRING pszDestinationString, PCSTR pszSourceString);

//get the authentication package that will be used for our logon attempt
HRESULT RetrieveNegotiateAuthPackage(
    ULONG * pulAuthPackage
    );



//packages the credentials into the buffer that the system expects
HRESULT EIDUnlockLogonPack(
    const EID_INTERACTIVE_UNLOCK_LOGON& rkiulIn,
	const PEID_SMARTCARD_CSP_INFO pCspInfo,
    BYTE** prgb,
    DWORD* pcb
    );

//szAuthPackageValue must be freed by  LsaFreeMemory
HRESULT CallAuthPackage(LPCWSTR username ,LPWSTR * szAuthPackageValue, PULONG szAuthPackageLen);

VOID EIDDebugPrintEIDUnlockLogonStruct(UCHAR dwLevel, PEID_INTERACTIVE_UNLOCK_LOGON pUnlockLogon) ;

NTSTATUS RemapPointer(PEID_INTERACTIVE_UNLOCK_LOGON pUnlockLogon, PVOID ClientAuthenticationBase, ULONG AuthenticationInformationLength);

PTSTR GetUsernameFromRid(__in DWORD dwRid);
DWORD GetRidFromUsername(LPTSTR szUsername);
BOOL HasAccountOnCurrentComputer(PWSTR szUserName);
BOOL IsCurrentUser(PWSTR szUserName);
BOOL IsAdmin(PWSTR szUserName);

BOOL LsaEIDCreateStoredCredential(__in PWSTR szUsername, __in PWSTR szPassword, __in PCCERT_CONTEXT pCertContext, __in BOOL fEncryptPassword);

BOOL LsaEIDRemoveStoredCredential(__in_opt PWSTR szUsername);

BOOL LsaEIDHasStoredCredential(__in_opt PWSTR szUsername);

DWORD LsaEIDGetRIDFromStoredCredential(__in PCCERT_CONTEXT pContext);

//BOOL CanEncryptPassword(__in_opt HCRYPTPROV hProv, __in_opt DWORD dwKeySpec,  __in_opt PCCERT_CONTEXT pCertContext);