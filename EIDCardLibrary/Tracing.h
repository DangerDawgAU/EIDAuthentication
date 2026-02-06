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


/**
 *  Tracing function.
 */



/*
WINEVENT_LEVEL_CRITICAL Abnormal exit or termination events.
WINEVENT_LEVEL_ERROR Severe error events.
WINEVENT_LEVEL_WARNING Warning events such as allocation failures.
WINEVENT_LEVEL_INFO Non-error events such as entry or exit events.
WINEVENT_LEVEL_VERBOSE Detailed trace events.
*/

#pragma once

constexpr UCHAR WINEVENT_LEVEL_CRITICAL = 1;
constexpr UCHAR WINEVENT_LEVEL_ERROR    = 2;
constexpr UCHAR WINEVENT_LEVEL_WARNING  = 3;
constexpr UCHAR WINEVENT_LEVEL_INFO     = 4;
constexpr UCHAR WINEVENT_LEVEL_VERBOSE  = 5;

void EIDCardLibraryTracingRegister();
void EIDCardLibraryTracingUnRegister();

#define EIDCardLibraryTrace(dwLevel, ...) \
	EIDCardLibraryTraceEx(__FILE__,__LINE__,__FUNCTION__, dwLevel, __VA_ARGS__)

void EIDCardLibraryTraceEx(PCSTR szFile, DWORD dwLine, PCSTR szFunction, UCHAR dwLevel, PCWSTR szFormat,...);

#define EIDCardLibraryDumpMemory(memory, memorysize) \
	EIDCardLibraryDumpMemoryEx(__FILE__,__LINE__,__FUNCTION__, memory, memorysize)

void EIDCardLibraryDumpMemoryEx(LPCSTR szFile, DWORD dwLine, LPCSTR szFunction, PVOID memory, DWORD memorysize);

/**
 *  Display a messagebox giving an error code
 */
void MessageBoxWin32Ex2(DWORD status, HWND hWnd, LPCSTR szFile, DWORD dwLine);
#define MessageBoxWin32(status) MessageBoxWin32Ex2 (status, NULL, __FILE__,__LINE__)
#define MessageBoxWin32Ex(status, hwnd ) MessageBoxWin32Ex2 (status, hwnd, __FILE__,__LINE__)

BOOL LookUpErrorMessage(PWSTR buf, int cch, DWORD err);

LONG EIDExceptionHandler( PEXCEPTION_POINTERS pExceptPtrs );
LONG EIDExceptionHandlerDebug( PEXCEPTION_POINTERS pExceptPtrs, BOOL fMustCrash );

BOOL StartLogging();
BOOL StopLogging();

// Security audit logging for security-relevant events
// These events are logged with elevated visibility for security monitoring
constexpr UCHAR SECURITY_AUDIT_SUCCESS  = 0;
constexpr UCHAR SECURITY_AUDIT_FAILURE  = 1;
constexpr UCHAR SECURITY_AUDIT_WARNING  = 2;

#define EIDSecurityAudit(dwAuditType, ...) \
	EIDSecurityAuditEx(__FILE__,__LINE__,__FUNCTION__, dwAuditType, __VA_ARGS__)

void EIDSecurityAuditEx(PCSTR szFile, DWORD dwLine, PCSTR szFunction, UCHAR dwAuditType, PCWSTR szFormat,...);