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

//
// Standard dll required functions and class factory implementation.

#include <windows.h>
#include <unknwn.h>
#include <credentialprovider.h>

#include "Dll.h"
#include "../EIDCardLibrary/guid.h"
#include "../EIDCardLibrary/Registration.h"
#include "../EIDCardLibrary/CommonManifest.h"

static LONG g_cRef = 0;   // global dll reference count

// IClassFactory ///////////////////////////////////////////////////////////////////////

extern HRESULT CEIDProvider_CreateInstance(REFIID riid, void** ppv);
extern HRESULT CEIDFilter_CreateInstance(REFIID riid, void** ppv);

HINSTANCE g_hinst = nullptr;   // global dll hinstance

class CClassFactory : public IClassFactory
{
  public:
    // IUnknown
    STDMETHOD_(ULONG, AddRef)()
    {
        return InterlockedIncrement(&_cRef);
    }

    STDMETHOD_(ULONG, Release)()
    {
        LONG cRef = InterlockedDecrement(&_cRef);
        if (cRef == 0)
        {
            delete this;
        }
        return cRef;
    }

    STDMETHOD (QueryInterface)(REFIID riid, void** ppv) 
    {
        HRESULT hr;
        if (ppv != nullptr)
        {
            if (IID_IClassFactory == riid || IID_IUnknown == riid)
            {
                *ppv = static_cast<IUnknown*>(this);
                reinterpret_cast<IUnknown*>(*ppv)->AddRef();
                hr = S_OK;
            }
            else
            {
                *ppv = nullptr;
                hr = E_NOINTERFACE;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
        return hr;
    }

    // IClassFactory
    STDMETHOD (CreateInstance)(IUnknown* pUnkOuter, REFIID riid, void** ppv)
    {
        HRESULT hr;
        if (!pUnkOuter)
        {
            if (IID_ICredentialProviderFilter == riid)
			{
				hr = CEIDFilter_CreateInstance(riid, ppv);
			}
			else
			{
				hr = CEIDProvider_CreateInstance(riid, ppv);
			}
        }
        else
        {
            hr = CLASS_E_NOAGGREGATION;
        }
        return hr;
    }

    STDMETHOD (LockServer)(BOOL bLock)
    {
        if (bLock)
        {
            DllAddRef();
        }
        else
        {
            DllRelease();
        }
        return S_OK;
    }

  private:
     CClassFactory() : _cRef(1) {}
    ~CClassFactory() = default;

  private:
    LONG _cRef;

    friend HRESULT CClassFactory_CreateInstance(REFCLSID rclsid, REFIID riid, void** ppv);
};

HRESULT CClassFactory_CreateInstance(REFCLSID rclsid, REFIID riid, void** ppv)
{
    HRESULT hr;
    if (CLSID_CEIDProvider == rclsid )
    {
        CClassFactory* pcf = new CClassFactory;
        if (pcf)
        {
            hr = pcf->QueryInterface(riid, ppv);
            pcf->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = CLASS_E_CLASSNOTAVAILABLE;
    }
    return hr;
}

// DLL Functions ///////////////////////////////////////////////////////////////////////

BOOL WINAPI DllMain(
    HINSTANCE hinstDll,
    DWORD dwReason,
    LPVOID pReserved
    )
{
    UNREFERENCED_PARAMETER(pReserved);

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDll);
        break;
    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    default:
        break;
    }

    g_hinst = hinstDll;
    return TRUE;

}

void DllAddRef()
{
    InterlockedIncrement(&g_cRef);
}

void DllRelease()
{
    InterlockedDecrement(&g_cRef);
}

// DLL entry point.
STDAPI DllCanUnloadNow()
{
    HRESULT hr;

    if (g_cRef > 0)
    {
        hr = S_FALSE;   // cocreated objects still exist, don't unload
    }
    else
    {
        hr = S_OK;      // refcount is zero, ok to unload
    }

    return hr;
}

// DLL entry point.
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    return CClassFactory_CreateInstance(rclsid, riid, ppv);
}

