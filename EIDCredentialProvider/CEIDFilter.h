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
#include <credentialprovider.h>
#include "helpers.h"

/**
  * Used to filter password credential when smart card logon is mandatory
  */
class CEIDFilter : public ICredentialProviderFilter
{
public:
	CEIDFilter(const CEIDFilter&) = delete;
	CEIDFilter& operator=(const CEIDFilter&) = delete;
	CEIDFilter();
	// IUnknown
    IMPL_IUNKNOWN_ADDREF_RELEASE()

    STDMETHOD (QueryInterface)(REFIID riid, void** ppv)
    {
        HRESULT hr;
        if (IID_IUnknown == riid || 
            IID_ICredentialProviderFilter == riid)
        {
            *ppv = this;
            reinterpret_cast<IUnknown*>(*ppv)->AddRef();
            hr = S_OK;
        }
        else
        {
            *ppv = NULL;
            hr = E_NOINTERFACE;
        }
        return hr;
    }
public:
	IFACEMETHODIMP Filter(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus, DWORD dwFlags, GUID *rgclsidProviders, BOOL *rgbAllow, DWORD cProviders);
	IFACEMETHODIMP UpdateRemoteCredential(const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION *pcpcsIn, CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION *pcpcsOut);
    
private:
	LONG                        _cRef;                  // Reference counter.
};



// Boilerplate method to create an instance of our provider. 
HRESULT CEIDFilter_CreateInstance(REFIID riid, void** ppv)
{
    HRESULT hr;
	if (riid != IID_ICredentialProviderFilter) return E_NOINTERFACE;
    CEIDFilter* pFilter = new CEIDFilter();

    if (pFilter)
    {
        hr = pFilter->QueryInterface(riid, ppv);
        pFilter->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    
    return hr;
}

