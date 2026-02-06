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
//

#pragma once
#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>

#include "helpers.h"
#include "dll.h"
#include "EIDCredentialProvider.h"

enum CMessageCredentialStatus
{
	Idle,
	Reading,
	EndReading,
	Error,
};

/**
  * Used to display message when no credential is available
  */
class CMessageCredential : public ICredentialProviderCredential
{
public:
	CMessageCredential(const CMessageCredential&) = delete;
	CMessageCredential& operator=(const CMessageCredential&) = delete;
    // IUnknown
    IMPL_IUNKNOWN_ADDREF_RELEASE()
    IMPL_CREDENTIAL_QUERYINTERFACE()
  public:
    // ICredentialProviderCredential
    IFACEMETHODIMP Advise(ICredentialProviderCredentialEvents* pcpce) override;
    IFACEMETHODIMP UnAdvise() override;

    IFACEMETHODIMP SetSelected(BOOL* pbAutoLogon) override;
    IFACEMETHODIMP SetDeselected() override;

    IFACEMETHODIMP GetFieldState(DWORD dwFieldID,
                                 CREDENTIAL_PROVIDER_FIELD_STATE* pcpfs,
                                 CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE* pcpfis) override;

    IFACEMETHODIMP GetStringValue(DWORD dwFieldID, PWSTR* ppwsz) override;
    IFACEMETHODIMP GetBitmapValue(DWORD dwFieldID, HBITMAP* phbmp) override;
    IFACEMETHODIMP GetCheckboxValue(DWORD dwFieldID, BOOL* pbChecked, PWSTR* ppwszLabel) override;
    IFACEMETHODIMP GetComboBoxValueCount(DWORD dwFieldID, DWORD* pcItems, DWORD* pdwSelectedItem) override;
    IFACEMETHODIMP GetComboBoxValueAt(DWORD dwFieldID, DWORD dwItem, PWSTR* ppwszItem) override;
    IFACEMETHODIMP GetSubmitButtonValue(DWORD dwFieldID, DWORD* pdwAdjacentTo) override;

    IFACEMETHODIMP SetStringValue(DWORD dwFieldID, PCWSTR pwz) override;
    IFACEMETHODIMP SetCheckboxValue(DWORD dwFieldID, BOOL bChecked) override;
    IFACEMETHODIMP SetComboBoxSelectedValue(DWORD dwFieldID, DWORD dwSelectedItem) override;
    IFACEMETHODIMP CommandLinkClicked(DWORD dwFieldID) override;

    IFACEMETHODIMP GetSerialization(CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr,
                                    CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs,
                                    PWSTR* ppwszOptionalStatusText,
                                    CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon) override;
    IFACEMETHODIMP ReportResult(NTSTATUS ntsStatus,
                                NTSTATUS ntsSubstatus,
                                PWSTR* ppwszOptionalStatusText,
                                CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon) override;

  public:
    HRESULT Initialize(const CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR* rgcpfd,
                       const FIELD_STATE_PAIR* rgfsp,
                       PWSTR szMessage);
    CMessageCredential();

    virtual ~CMessageCredential();
	void IncreaseSmartCardCount()
	{
		_dwSmartCardCount++;
	}
	void DecreaseSmartCardCount()
	{
		_dwSmartCardCount--;
	}
	void SetStatus(DWORD dwStatus) 
	{
		if (dwStatus == EndReading)
		{
			if (_dwSmartCardCount)
			{
				_dwStatus = Error;
			}
			else
			{
				_dwStatus = Idle;
			}
		}
		else
		{
			_dwStatus = dwStatus;
		}
	}
	void SetUsageScenario(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,DWORD dwFlags)
	{
		_cpus = cpus;
		_dwFlags = dwFlags;
	}

	DWORD GetStatus()
	{
		return _dwStatus;
	}

  private:
    LONG                                    _cRef;
    
    CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR    _rgCredProvFieldDescriptors[SMFI_NUM_FIELDS];   // An array holding the 
                                                                                            // type and name of each 
                                                                                            // field in the tile.
    
    FIELD_STATE_PAIR                        _rgFieldStatePairs[SMFI_NUM_FIELDS];            // An array holding the 
                                                                                            // state of each field in 
                                                                                            // the tile.

    PWSTR                                   _rgFieldStrings[SMFI_NUM_FIELDS];               // An array holding the 
                                                                                            // string value of each 
                                                                                            // field. This is different 
                                                                                            // from the name of the 
                                                                                            // field held in 
                                                                                            // _rgCredProvFieldDescriptors.
	ICredentialProviderCredentialEvents*	_pCredProvCredentialEvents;    
	DWORD									_dwSmartCardCount;
	DWORD									_dwStatus;
	DWORD									_dwOldStatus;
	CREDENTIAL_PROVIDER_USAGE_SCENARIO    _cpus; // The usage scenario for which we were enumerated.
	DWORD								  _dwFlags;
};

