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

#include <ntstatus.h>
#define WIN32_NO_STATUS 1
#include <windows.h>
#include <Ntsecapi.h>


#include "../EIDCardLibrary/Tracing.h"
#include "../EIDCardLibrary/StoredCredentialManagement.h"
#include "../EIDCardLibrary/Registration.h"//#include "../EIDCardLibrary/XPCompatibility.h"

typedef NTSTATUS
(NTAPI LSA_IMPERSONATE_CLIENT) (
    VOID
    );
typedef LSA_IMPERSONATE_CLIENT * PLSA_IMPERSONATE_CLIENT;
void SetImpersonate(PLSA_IMPERSONATE_CLIENT Impersonate);

NTSTATUS NTAPI Impersonate (VOID)
{
	return STATUS_SUCCESS;
}

/*
The InitializeChangeNotify function is implemented by a password filter DLL.
This function initializes the DLL.
*/

BOOL WINAPI InitializeChangeNotify()
{
	EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Enter");
	// if we don't set this, the library thinks that we are in a test process
	// note : impersonation is not needed, that's why it is set to nothing
	SetImpersonate(Impersonate);
	return TRUE;
}

/*
The PasswordFilter function is implemented by a password filter DLL.
The value returned by this function determines whether the new password 
is accepted by the system. All of the password filters installed on a 
system must return TRUE for the password change to take effect.
*/

BOOL WINAPI PasswordFilter(
	PUNICODE_STRING AccountName,
	PUNICODE_STRING FullName,
	PUNICODE_STRING Password,
	BOOLEAN SetOperation
)
{
	UNREFERENCED_PARAMETER(AccountName);
	UNREFERENCED_PARAMETER(FullName);
	UNREFERENCED_PARAMETER(Password);
	UNREFERENCED_PARAMETER(SetOperation);
	EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Enter");
	return TRUE;
}

/*
The PasswordChangeNotify function is implemented by a password filter DLL.
It notifies the DLL that a password was changed.
*/

NTSTATUS WINAPI PasswordChangeNotify(
	PUNICODE_STRING UserName,
	ULONG RelativeId,
	PUNICODE_STRING NewPassword
)
{
	EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Enter");
	EIDCardLibraryTrace(WINEVENT_LEVEL_VERBOSE,L"Username %wZ RelativeId %d",UserName,RelativeId);
	CStoredCredentialManager* manager = CStoredCredentialManager::Instance();
	manager->UpdateCredential(RelativeId, NewPassword->Buffer, NewPassword->Length);
	return TRUE;
}
