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
#include <tchar.h>
#include "global.h"

enum CheckType {
	CHECK_SIGNATUREONLY = 0,
	CHECK_TRUST = 1,
	CHECK_CRYPTO = 2,
	CHECK_MAX = 3
};

// Image result constants for GetImage function
#define CHECK_SUCCESS 1
#define CHECK_FAILED 2
#define CHECK_WARNING 3

#define CHECK_SIGNATUREONLY CheckType::CHECK_SIGNATUREONLY
#define CHECK_TRUST CheckType::CHECK_TRUST
#define CHECK_CRYPTO CheckType::CHECK_CRYPTO
#define CHECK_MAX CheckType::CHECK_MAX

class CContainerHolderTest
{
public:
	CContainerHolderTest(CContainer* pContainer);
	virtual ~CContainerHolderTest();
	void Release();
	CContainer* GetContainer();
	int GetIconIndex();
	BOOL HasSignatureUsageOnly();
	BOOL IsTrusted();
	BOOL SupportEncryption();
	int GetCheckCount();
	int GetImage(DWORD dwCheckNum);
	PTSTR GetDescription(DWORD dwCheckNum);
	PTSTR GetSolveDescription(DWORD dwCheckNum);
	BOOL Solve(DWORD dwCheckNum);
	HRESULT SetUsageScenario(__in CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,__in DWORD dwFlags);
private:
	CContainer* _pContainer;
	BOOL _IsTrusted;
	BOOL _SupportEncryption;
	DWORD _dwTrustError;
};
