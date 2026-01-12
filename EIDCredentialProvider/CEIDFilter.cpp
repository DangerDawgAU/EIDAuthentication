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


#include "CEIDFilter.h"
#include "../EIDCardLibrary/GPO.h"

CEIDFilter::CEIDFilter():
    _cRef(1)
{
}

HRESULT CEIDFilter::UpdateRemoteCredential(      
    const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION *pcpcsIn,
    CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION *pcpcsOut
)
{
	UNREFERENCED_PARAMETER(pcpcsIn);
	UNREFERENCED_PARAMETER(pcpcsOut);
	return S_OK;
}

HRESULT CEIDFilter::Filter(      
    CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,
    DWORD dwFlags,
    GUID *rgclsidProviders,
    BOOL *rgbAllow,
    DWORD cProviders
)
{
	UNREFERENCED_PARAMETER(cpus);
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(rgclsidProviders);
	UNREFERENCED_PARAMETER(rgbAllow);
	UNREFERENCED_PARAMETER(cProviders);
	/*BOOL fFilter = FALSE;
	if (cpus == CPUS_LOGON || cpus == CPUS_UNLOCK_WORKSTATION)
	{
		fFilter = (GetPolicyValue(scforceoption) == 1);
	}
	if (fFilter)
	{
		for (DWORD dwI = 0; dwI < cProviders; dwI++)
		{
			if (rgclsidProviders[dwI] == CLSID_PasswordCredentialProvider)
			{
				rgbAllow[dwI] = FALSE;
			}
		}
	}*/
	return S_OK;
}