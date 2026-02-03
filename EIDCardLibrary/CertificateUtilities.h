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

#include <windows.h>
#include <wincrypt.h>
#include <tchar.h>

PCCERT_CONTEXT SelectFirstCertificateWithPrivateKey();
PCCERT_CONTEXT SelectCertificateWithPrivateKey(HWND hWnd = NULL);

BOOL AskForCard(LPWSTR szReader, DWORD ReaderLength,LPWSTR szCard,DWORD CardLength);

BOOL SchGetProviderNameFromCardName(__in LPCTSTR szCardName, __out LPTSTR szProviderName, __out PDWORD pdwProviderNameLen);

constexpr DWORD UI_CERTIFICATE_INFO_SAVEON_USERSTORE = 0;
constexpr DWORD UI_CERTIFICATE_INFO_SAVEON_SYSTEMSTORE = 1;
constexpr DWORD UI_CERTIFICATE_INFO_SAVEON_SYSTEMSTORE_MY = 2;
constexpr DWORD UI_CERTIFICATE_INFO_SAVEON_FILE = 3;
constexpr DWORD UI_CERTIFICATE_INFO_SAVEON_SMARTCARD = 4;

typedef struct _UI_CERTIFICATE_INFO
{
	LPTSTR szSubject;
	PCCERT_CONTEXT pRootCertificate;
	DWORD dwSaveon;
	LPTSTR szCard;
	LPTSTR szReader;
	DWORD dwKeyType;
	DWORD dwKeySizeInBits;
	BOOL bIsSelfSigned;
	BOOL bHasSmartCardAuthentication;
	BOOL bHasServerAuthentication;
	BOOL bHasClientAuthentication;
	BOOL bHasEFS;
	BOOL bIsCA;
	SYSTEMTIME StartTime;
	SYSTEMTIME EndTime;
	
	// used to return new certificate context if needed
	// need to free it if returned
	BOOL fReturnCerticateContext;
	PCCERT_CONTEXT pNewCertificate;
} UI_CERTIFICATE_INFO, * PUI_CERTIFICATE_INFO;

PCCERT_CONTEXT GetCertificateWithPrivateKey();
BOOL CreateCertificate(PUI_CERTIFICATE_INFO CertificateInfo);
BOOL ClearCard(PTSTR szReaderName, PTSTR szCardName);
BOOL ImportFileToSmartCard(PTSTR szFileName, PTSTR szPassword, PTSTR szReaderName, PTSTR szCardname);
PCCERT_CONTEXT FindCertificateFromHash(PCRYPT_DATA_BLOB pCertInfo);

// Sets CERT_KEY_PROV_INFO_PROP_ID and CERT_KEY_CONTEXT_PROP_ID on a certificate context.
BOOL SetupCertificateContextWithKeyInfo(
    __in PCCERT_CONTEXT pCertContext, __in HCRYPTPROV hProv,
    __in LPCWSTR pwszProviderName, __in LPCWSTR pwszContainerName, __in DWORD dwKeySpec);

// Returns allocated string "\\.\\<readerName>\\" - caller must EIDFree
LPTSTR BuildContainerNameFromReader(LPCTSTR szReaderName);
