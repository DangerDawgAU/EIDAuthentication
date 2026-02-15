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

#include <Windows.h>
#include <tchar.h>
#include <wmistr.h>
#include <evntrace.h>

#include "TraceExport.h"

// Non-const string buffer for EVENT_TRACE_LOGFILE.LoggerName (requires LPWSTR)
static TCHAR s_szLoggerName[] = TEXT("EIDCredentialProvider");

static HANDLE g_hTraceOutputFile = nullptr;

static VOID WINAPI ProcessEvents(PEVENT_TRACE pEvent)
{
  {
	  if (pEvent->MofLength && pEvent->Header.Class.Level > 0)
	  {
		DWORD dwWritten;
		FILETIME ft;
		SYSTEMTIME st;
		ft.dwHighDateTime = pEvent->Header.TimeStamp.HighPart;
		ft.dwLowDateTime = pEvent->Header.TimeStamp.LowPart;
		FileTimeToSystemTime(&ft,&st);
		TCHAR szLocalDate[255], szLocalTime[255];
		_stprintf_s(szLocalDate, ARRAYSIZE(szLocalDate),TEXT("%04d-%02d-%02d"),st.wYear,st.wMonth,st.wDay);
		_stprintf_s(szLocalTime, ARRAYSIZE(szLocalTime),TEXT("%02d:%02d:%02d"),st.wHour,st.wMinute,st.wSecond);
		WriteFile ( g_hTraceOutputFile, szLocalDate, (DWORD)_tcslen(szLocalDate) * (DWORD)sizeof(TCHAR), &dwWritten, nullptr);
		WriteFile ( g_hTraceOutputFile, TEXT(";"), 1 * (DWORD)sizeof(TCHAR), &dwWritten, nullptr);
		WriteFile ( g_hTraceOutputFile, szLocalTime, (DWORD)_tcslen(szLocalTime) * (DWORD)sizeof(TCHAR), &dwWritten, nullptr);
		WriteFile ( g_hTraceOutputFile, TEXT(";"), 1 * (DWORD)sizeof(TCHAR), &dwWritten, nullptr);
		WriteFile ( g_hTraceOutputFile, pEvent->MofData, (DWORD)_tcslen((PTSTR) pEvent->MofData) * (DWORD)sizeof(TCHAR), &dwWritten, nullptr);
		WriteFile ( g_hTraceOutputFile, TEXT("\r\n"), 2 * (DWORD)sizeof(TCHAR), &dwWritten, nullptr);
	  }
  }
}

void ExportOneTraceFile(HANDLE hOutputFile, PTSTR szTraceFile)
{
	g_hTraceOutputFile = hOutputFile;
	ULONG rc;
	TRACEHANDLE handle = NULL;
	EVENT_TRACE_LOGFILE trace;
	memset(&trace,0, sizeof(EVENT_TRACE_LOGFILE));
	trace.LoggerName = s_szLoggerName;
	trace.LogFileName = szTraceFile;
	trace.EventCallback = reinterpret_cast<PEVENT_CALLBACK>(ProcessEvents);
	handle = OpenTrace(&trace);
	if ((TRACEHANDLE)INVALID_HANDLE_VALUE == handle)
	{
	}
	else
	{
		FILETIME now, start;
		SYSTEMTIME sysNow, sysstart;
		GetLocalTime(&sysNow);
		SystemTimeToFileTime(&sysNow, &now);
		memcpy(&sysstart, &sysNow, sizeof(SYSTEMTIME));
		sysstart.wYear -= 1;
		SystemTimeToFileTime(&sysstart, &start);
		DWORD dwWritten;
		TCHAR szBuffer[256];
		_tcscpy_s(szBuffer,ARRAYSIZE(szBuffer),TEXT("================================================\r\n"));
		WriteFile ( hOutputFile, szBuffer, (DWORD)_tcslen(szBuffer) * (DWORD)sizeof(TCHAR), &dwWritten, nullptr);
		WriteFile ( hOutputFile, szTraceFile, (DWORD)_tcslen(szTraceFile) * (DWORD)sizeof(TCHAR), &dwWritten, nullptr);
		_tcscpy_s(szBuffer,ARRAYSIZE(szBuffer),TEXT("\r\n"));
		WriteFile ( hOutputFile, szBuffer, (DWORD)_tcslen(szBuffer) * (DWORD)sizeof(TCHAR), &dwWritten, nullptr);
		_tcscpy_s(szBuffer,ARRAYSIZE(szBuffer),TEXT("================================================\r\n"));
		WriteFile ( hOutputFile, szBuffer, (DWORD)_tcslen(szBuffer) * (DWORD)sizeof(TCHAR), &dwWritten, nullptr);
		rc = ProcessTrace(&handle, 1, nullptr, nullptr);
		if (rc != ERROR_SUCCESS && rc != ERROR_CANCELLED)
		{
			if (rc ==  0x00001069)
			{
			}
			else
			{
			}
		}
		CloseTrace(handle);
	}
}
