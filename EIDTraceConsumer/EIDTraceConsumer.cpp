/*
    EID Authentication - Smart card authentication for Windows
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
 *  EID Trace Consumer Service
 *
 *  Windows service that consumes EID ETW events and writes them to CSV.
 */

#include <Windows.h>
#include <tdh.h>
#include <evntrace.h>
#include <evntprov.h>
#include <strsafe.h>
#include <stdio.h>

// Service name and display name
#define SERVICE_NAME             L"EIDTraceConsumer"
#define SERVICE_DISPLAY_NAME      L"EID Trace Consumer"
#define SERVICE_DESCRIPTION       L"Consumes EID authentication events and writes to CSV log files"

// ETW Provider GUID for EID Credential Provider
// {B4866A0A-DB08-4835-A26F-414B46F3244C}
static const GUID EID_PROVIDER_GUID =
    {0xB4866A0A, 0xDB08, 0x4835, {0xA2, 0x6F, 0x41, 0x4B, 0x46, 0xF3, 0x24, 0x4C}};

// Service status handle
SERVICE_STATUS_HANDLE g_StatusHandle = nullptr;
SERVICE_STATUS g_ServiceStatus = {0};

// Event trace session handle
TRACEHANDLE gTraceHandle = 0;
BOOL g_ServiceRunning = TRUE;
HANDLE g_StopEvent = nullptr;

// Registry key for configuration
#define EID_CSV_CONFIG_KEY L"SOFTWARE\\EIDAuthentication\\LogManager"

// CSV log file handle and state
HANDLE g_hCsvFile = INVALID_HANDLE_VALUE;
WCHAR g_szCsvPath[MAX_PATH] = {0};
DWORD g_dwCsvFileSize = 0;
BOOL g_fCsvHeaderWritten = FALSE;
BOOL g_fCsvEnabled = FALSE;

// Configuration
DWORD g_dwMaxFileSizeMB = 64;
DWORD g_dwFileCount = 5;

// Forward declarations
VOID WINAPI ServiceMain(DWORD dwArgc, LPWSTR *lpszArgv);
DWORD WINAPI ServiceCtrlHandlerEx(DWORD dwCtrl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
DWORD ServiceInit(DWORD dwArgc, LPWSTR *lpszArgv);
VOID ReportStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
DWORD ServiceWorkerThread(LPVOID lpParam);

BOOL InstallService();
BOOL UninstallService();
BOOL StartServiceWrapper();
BOOL StopServiceWrapper();

// Configuration and CSV management
BOOL LoadCsvConfiguration();
BOOL EnsureCsvFileOpen();
void WriteCsvHeader();
void WriteCsvEvent(const WCHAR* timestamp, DWORD eventId, const WCHAR* severity,
                   const WCHAR* message, const WCHAR* category);
void RotateCsvFile();
void CloseCsvFile();

// ================================================================
// Configuration Loading
// ================================================================
BOOL LoadCsvConfiguration()
{
    HKEY hKey = nullptr;
    LONG err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, EID_CSV_CONFIG_KEY, 0, KEY_READ, &hKey);
    if (err != ERROR_SUCCESS)
    {
        wprintf(L"CSV logging: Config key not found, using defaults\n");
        g_fCsvEnabled = FALSE;
        return FALSE;
    }

    // Read CSVEnabled flag
    DWORD dwEnabled = 0;
    DWORD dwSize = sizeof(dwEnabled);
    err = RegQueryValueExW(hKey, L"CSVEnabled", nullptr, nullptr,
                          reinterpret_cast<LPBYTE>(&dwEnabled), &dwSize);
    if (err == ERROR_SUCCESS)
    {
        g_fCsvEnabled = (dwEnabled != 0);
    }

    // Read CSV log path
    dwSize = sizeof(g_szCsvPath);
    err = RegQueryValueExW(hKey, L"CSVLogPath", nullptr, nullptr,
                          reinterpret_cast<LPBYTE>(g_szCsvPath), &dwSize);
    if (err != ERROR_SUCCESS || g_szCsvPath[0] == L'\0')
    {
        wcscpy_s(g_szCsvPath, L"C:\\ProgramData\\EIDAuthentication\\logs\\events.csv");
    }

    // Read max file size
    dwSize = sizeof(g_dwMaxFileSizeMB);
    err = RegQueryValueExW(hKey, L"CSVMaxFileSize", nullptr, nullptr,
                          reinterpret_cast<LPBYTE>(&g_dwMaxFileSizeMB), &dwSize);
    if (err != ERROR_SUCCESS)
    {
        g_dwMaxFileSizeMB = 64;
    }

    // Read file count
    dwSize = sizeof(g_dwFileCount);
    err = RegQueryValueExW(hKey, L"CSVFileCount", nullptr, nullptr,
                          reinterpret_cast<LPBYTE>(&g_dwFileCount), &dwSize);
    if (err != ERROR_SUCCESS)
    {
        g_dwFileCount = 5;
    }

    RegCloseKey(hKey);

    wprintf(L"CSV logging: %s, Path: %s, MaxSize: %u MB, Files: %u\n",
            g_fCsvEnabled ? L"Enabled" : L"Disabled",
            g_szCsvPath, g_dwMaxFileSizeMB, g_dwFileCount);

    return g_fCsvEnabled;
}

// ================================================================
// CSV File Management
// ================================================================
BOOL EnsureCsvFileOpen()
{
    if (g_hCsvFile != INVALID_HANDLE_VALUE)
        return TRUE;

    // Create directory if needed
    WCHAR szDir[MAX_PATH];
    wcscpy_s(szDir, g_szCsvPath);
    WCHAR* pLastSlash = wcsrchr(szDir, L'\\');
    if (pLastSlash)
    {
        *pLastSlash = L'\0';
        CreateDirectoryW(szDir, nullptr);
    }

    // Open file for append (UTF-8 encoding)
    g_hCsvFile = CreateFileW(
        g_szCsvPath,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (g_hCsvFile == INVALID_HANDLE_VALUE)
        return FALSE;

    // Get current file size
    LARGE_INTEGER liSize;
    if (GetFileSizeEx(g_hCsvFile, &liSize))
    {
        g_dwCsvFileSize = static_cast<DWORD>(liSize.QuadPart);
    }

    // Seek to end for append
    SetFilePointer(g_hCsvFile, 0, nullptr, FILE_END);

    // Write header if file is empty
    if (g_dwCsvFileSize == 0)
    {
        // Write UTF-8 BOM
        DWORD dwWritten = 0;
        const BYTE bom[] = {0xEF, 0xBB, 0xBF};
        WriteFile(g_hCsvFile, bom, 3, &dwWritten, nullptr);
        g_dwCsvFileSize += dwWritten;

        WriteCsvHeader();
    }

    return TRUE;
}

void WriteCsvHeader()
{
    if (g_fCsvHeaderWritten)
        return;

    const char header[] =
        "Timestamp,EventID,Severity,Category,Message\r\n";

    DWORD dwWritten = 0;
    WriteFile(g_hCsvFile, header, static_cast<DWORD>(strlen(header)), &dwWritten, nullptr);
    g_dwCsvFileSize += dwWritten;
    g_fCsvHeaderWritten = TRUE;
}

void WriteCsvEvent(const WCHAR* timestamp, DWORD eventId, const WCHAR* severity,
                   const WCHAR* message, const WCHAR* category)
{
    if (!g_fCsvEnabled || !EnsureCsvFileOpen())
        return;

    // Check file rotation
    DWORD dwMaxBytes = g_dwMaxFileSizeMB * 1024 * 1024;
    if (g_dwCsvFileSize >= dwMaxBytes)
    {
        RotateCsvFile();
        if (!EnsureCsvFileOpen())
            return;
    }

    // Convert strings to UTF-8
    char szTimestamp[64] = {0};
    char szMessage[2048] = {0};
    char szCategory[64] = {0};
    char szSeverity[32] = {0};

    WideCharToMultiByte(CP_UTF8, 0, timestamp, -1, szTimestamp, sizeof(szTimestamp), nullptr, nullptr);
    WideCharToMultiByte(CP_UTF8, 0, message, -1, szMessage, sizeof(szMessage), nullptr, nullptr);
    WideCharToMultiByte(CP_UTF8, 0, category, -1, szCategory, sizeof(szCategory), nullptr, nullptr);
    WideCharToMultiByte(CP_UTF8, 0, severity, -1, szSeverity, sizeof(szSeverity), nullptr, nullptr);

    // Build CSV line (with escaping)
    char szLine[4096];
    int len = sprintf_s(szLine, sizeof(szLine),
        "%s,%u,%s,\"%s\",\"%s\"\r\n",
        szTimestamp, eventId, szSeverity, szCategory, szMessage);

    if (len > 0)
    {
        DWORD dwWritten = 0;
        WriteFile(g_hCsvFile, szLine, len, &dwWritten, nullptr);
        g_dwCsvFileSize += dwWritten;
    }
}

void RotateCsvFile()
{
    if (g_hCsvFile != INVALID_HANDLE_VALUE)
    {
        FlushFileBuffers(g_hCsvFile);
        CloseHandle(g_hCsvFile);
        g_hCsvFile = INVALID_HANDLE_VALUE;
    }

    // Determine rotation number
    DWORD dwRotation = 1;
    WCHAR szRotatedPath[MAX_PATH];

    for (DWORD i = 1; i <= g_dwFileCount; i++)
    {
        swprintf_s(szRotatedPath, L"%s.%03u", g_szCsvPath, i);
        if (GetFileAttributesW(szRotatedPath) == INVALID_FILE_ATTRIBUTES)
        {
            dwRotation = i;
            break;
        }
    }

    // If all slots filled, rotate all files
    if (dwRotation > g_dwFileCount)
    {
        // Delete oldest file
        swprintf_s(szRotatedPath, L"%s.%03u", g_szCsvPath, g_dwFileCount);
        DeleteFileW(szRotatedPath);

        // Rename remaining files
        for (DWORD i = g_dwFileCount - 1; i >= 1; i--)
        {
            WCHAR szOldPath[MAX_PATH];
            swprintf_s(szOldPath, L"%s.%03u", g_szCsvPath, i);
            swprintf_s(szRotatedPath, L"%s.%03u", g_szCsvPath, i + 1);
            MoveFileExW(szOldPath, szRotatedPath, MOVEFILE_REPLACE_EXISTING);
        }
        dwRotation = 1;
    }

    // Rename current file
    swprintf_s(szRotatedPath, L"%s.%03u", g_szCsvPath, dwRotation);
    MoveFileExW(g_szCsvPath, szRotatedPath, MOVEFILE_REPLACE_EXISTING);

    // Reset state
    g_dwCsvFileSize = 0;
    g_fCsvHeaderWritten = FALSE;
}

void CloseCsvFile()
{
    if (g_hCsvFile != INVALID_HANDLE_VALUE)
    {
        FlushFileBuffers(g_hCsvFile);
        CloseHandle(g_hCsvFile);
        g_hCsvFile = INVALID_HANDLE_VALUE;
    }
    g_dwCsvFileSize = 0;
    g_fCsvHeaderWritten = FALSE;
}

// ================================================================
// ETW Event Parsing
// ================================================================

// Parse event ID from message format: "[EID:XXXX] Message"
void ParseEventIdFromMessage(const WCHAR* message, DWORD* eventId)
{
    *eventId = 0;
    const WCHAR* p = wcsstr(message, L"[EID:");
    if (p)
    {
        p += 5; // Skip "[EID:"
        *eventId = static_cast<DWORD>(wcstoul(p, nullptr, 10));
    }
}

// Get category name from event ID
void GetEventCategory(DWORD eventId, WCHAR* category, size_t cchCategory)
{
    if (eventId >= 1001 && eventId <= 1999)
        wcscpy_s(category, cchCategory, L"AUTHENTICATION");
    else if (eventId >= 2001 && eventId <= 2999)
        wcscpy_s(category, cchCategory, L"AUTHORIZATION");
    else if (eventId >= 3001 && eventId <= 3999)
        wcscpy_s(category, cchCategory, L"SESSION");
    else if (eventId >= 4001 && eventId <= 4999)
        wcscpy_s(category, cchCategory, L"CERTIFICATE");
    else if (eventId >= 5001 && eventId <= 5999)
        wcscpy_s(category, cchCategory, L"SMARTCARD");
    else if (eventId >= 6001 && eventId <= 6999)
        wcscpy_s(category, cchCategory, L"LSA");
    else if (eventId >= 7001 && eventId <= 7999)
        wcscpy_s(category, cchCategory, L"CONFIG");
    else if (eventId >= 8001 && eventId <= 8999)
        wcscpy_s(category, cchCategory, L"AUDIT");
    else
        wcscpy_s(category, cchCategory, L"GENERAL");
}

// Get severity name from level
const WCHAR* GetSeverityName(UCHAR level)
{
    switch (level)
    {
        case 1: return L"CRITICAL";
        case 2: return L"ERROR";
        case 3: return L"WARNING";
        case 4: return L"INFO";
        case 5: return L"VERBOSE";
        default: return L"UNKNOWN";
    }
}

// ================================================================
// ETW Callback for Events
// ================================================================
VOID WINAPI EventCallback(PEVENT_RECORD pEvent)
{
    if (!pEvent || !g_fCsvEnabled)
        return;

    WCHAR szTimestamp[64] = {0};
    WCHAR szMessage[1024] = {0};
    DWORD eventId = 0;
    WCHAR szCategory[64] = {0};

    // Format timestamp - convert FILETIME to SYSTEMTIME
    FILETIME ft;
    ft.dwLowDateTime = pEvent->EventHeader.TimeStamp.LowPart;
    ft.dwHighDateTime = pEvent->EventHeader.TimeStamp.HighPart;

    SYSTEMTIME st;
    FileTimeToSystemTime(&ft, &st);

    swprintf_s(szTimestamp, ARRAYSIZE(szTimestamp),
        L"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    // Try to get message from UserData
    if (pEvent->UserData && pEvent->UserDataLength > 0)
    {
        ULONG cchData = pEvent->UserDataLength / sizeof(WCHAR);
        if (cchData > 0 && cchData < ARRAYSIZE(szMessage))
        {
            wcsncpy_s(szMessage, ARRAYSIZE(szMessage),
                     static_cast<WCHAR*>(pEvent->UserData), _TRUNCATE);
            szMessage[cchData] = L'\0';
        }
    }

    // If no user data, create a generic message
    if (szMessage[0] == L'\0')
    {
        swprintf_s(szMessage, ARRAYSIZE(szMessage),
            L"Event 0x%X from provider", pEvent->EventHeader.ProviderId.Data1);
    }

    // Parse event ID from message
    ParseEventIdFromMessage(szMessage, &eventId);

    // Get category
    GetEventCategory(eventId, szCategory, ARRAYSIZE(szCategory));

    // Write to CSV
    WriteCsvEvent(szTimestamp, eventId,
                  GetSeverityName(pEvent->EventHeader.EventDescriptor.Level),
                  szMessage[0] ? szMessage : L"(no message)",
                  szCategory);
}

// ================================================================
// Trace Session Management
// ================================================================
BOOL StartTraceSession()
{
    // Reload configuration to check if CSV logging is enabled
    LoadCsvConfiguration();

    if (!g_fCsvEnabled)
    {
        wprintf(L"CSV logging disabled, skipping trace session start\n");
        return TRUE; // Not an error - just disabled
    }

    EVENT_TRACE_LOGFILE logfile = {0};
    logfile.LogFileName = nullptr;
    logfile.LoggerName = const_cast<LPWSTR>(L"EIDTraceConsumer");
    logfile.LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
    logfile.EventRecordCallback = EventCallback;
    logfile.Context = nullptr;

    gTraceHandle = OpenTrace(&logfile);
    if (gTraceHandle == INVALID_PROCESSTRACE_HANDLE)
    {
        wprintf(L"OpenTrace failed: %d\n", GetLastError());
        return FALSE;
    }

    wprintf(L"Trace session started successfully\n");
    return TRUE;
}

BOOL StopTraceSession()
{
    if (gTraceHandle != INVALID_PROCESSTRACE_HANDLE)
    {
        CloseTrace(gTraceHandle);
        gTraceHandle = INVALID_PROCESSTRACE_HANDLE;
    }

    CloseCsvFile();
    wprintf(L"Trace session stopped\n");
    return TRUE;
}

// ================================================================
// Service Worker Thread
// ================================================================
DWORD ServiceWorkerThread(LPVOID lpParam)
{
    UNREFERENCED_PARAMETER(lpParam);

    wprintf(L"Service worker thread started\n");

    // Start the trace session
    if (!StartTraceSession())
    {
        ReportStatus(SERVICE_STOPPED, 1, 0);
        return 1;
    }

    // Main service loop - process events
    while (g_ServiceRunning)
    {
        // Wait for stop event with a short timeout
        DWORD dwWait = WaitForSingleObject(g_StopEvent, 1000);
        if (dwWait == WAIT_OBJECT_0)
        {
            // Stop event signaled
            break;
        }

        // Process trace events (with timeout for responsiveness)
        ULONG status = ProcessTrace(&gTraceHandle, 1, nullptr, nullptr);
        if (status != ERROR_SUCCESS && status != ERROR_CANCELLED)
        {
            wprintf(L"ProcessTrace failed: %u\n", status);
            // Try to restart trace session
            StopTraceSession();
            Sleep(5000);
            if (!StartTraceSession())
            {
                break;
            }
        }
    }

    // Stop the trace session
    StopTraceSession();

    ReportStatus(SERVICE_STOPPED, 0, 0);
    wprintf(L"Service worker thread exiting\n");
    return 0;
}

// ================================================================
// Service Main Entry Point
// ================================================================
VOID WINAPI ServiceMain(DWORD dwArgc, LPWSTR *lpszArgv)
{
    UNREFERENCED_PARAMETER(dwArgc);
    UNREFERENCED_PARAMETER(lpszArgv);

    g_StatusHandle = RegisterServiceCtrlHandlerExW(
        SERVICE_NAME,
        ServiceCtrlHandlerEx,
        nullptr
    );

    if (!g_StatusHandle)
    {
        wprintf(L"RegisterServiceCtrlHandlerEx failed: %d\n", GetLastError());
        return;
    }

    ReportStatus(SERVICE_START_PENDING, 0, 1000);

    // Create stop event
    g_StopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_StopEvent)
    {
        wprintf(L"CreateEvent failed: %d\n", GetLastError());
        ReportStatus(SERVICE_STOPPED, 1, 0);
        return;
    }

    // Load configuration
    LoadCsvConfiguration();

    // Create worker thread
    DWORD dwThreadId = 0;
    HANDLE hThread = CreateThread(
        nullptr,
        0,
        ServiceWorkerThread,
        nullptr,
        0,
        &dwThreadId
    );

    if (!hThread)
    {
        wprintf(L"CreateThread failed: %d\n", GetLastError());
        CloseHandle(g_StopEvent);
        ReportStatus(SERVICE_STOPPED, 1, 0);
        return;
    }

    ReportStatus(SERVICE_RUNNING, 0, 0);

    // Wait for worker thread to complete
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    CloseHandle(g_StopEvent);

    ReportStatus(SERVICE_STOPPED, 0, 0);
    wprintf(L"Service exiting\n");
}

// ================================================================
// Service Control Handler
// ================================================================
DWORD WINAPI ServiceCtrlHandlerEx(DWORD dwCtrl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
    UNREFERENCED_PARAMETER(dwEventType);
    UNREFERENCED_PARAMETER(lpEventData);
    UNREFERENCED_PARAMETER(lpContext);

    switch (dwCtrl)
    {
    case SERVICE_CONTROL_STOP:
        ReportStatus(SERVICE_STOP_PENDING, 0, 1000);
        g_ServiceRunning = FALSE;
        // Cancel the ProcessTrace call
        if (gTraceHandle != INVALID_PROCESSTRACE_HANDLE)
        {
            CloseTrace(gTraceHandle);
            gTraceHandle = INVALID_PROCESSTRACE_HANDLE;
        }
        SetEvent(g_StopEvent);
        ReportStatus(SERVICE_STOP_PENDING, 0, 1000);
        break;

    case SERVICE_CONTROL_SHUTDOWN:
        ReportStatus(SERVICE_STOP_PENDING, 0, 1000);
        g_ServiceRunning = FALSE;
        if (gTraceHandle != INVALID_PROCESSTRACE_HANDLE)
        {
            CloseTrace(gTraceHandle);
            gTraceHandle = INVALID_PROCESSTRACE_HANDLE;
        }
        SetEvent(g_StopEvent);
        ReportStatus(SERVICE_STOP_PENDING, 0, 1000);
        break;

    case SERVICE_CONTROL_INTERROGATE:
        ReportStatus(g_ServiceStatus.dwCurrentState, 0, 0);
        break;

    default:
        break;
    }
    return 0;
}

// ================================================================
// Service Status Reporting
// ================================================================
VOID ReportStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwCurrentState = dwCurrentState;
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    g_ServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
    g_ServiceStatus.dwWaitHint = dwWaitHint;

    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

// ================================================================
// Service Installation/Removal
// ================================================================
BOOL InstallService()
{
    SC_HANDLE hSCManager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
    if (!hSCManager)
    {
        wprintf(L"OpenSCManager failed: %d\n", GetLastError());
        return FALSE;
    }

    // Get the path to the executable
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(nullptr, szPath, MAX_PATH);

    SC_HANDLE hService = CreateServiceW(
        hSCManager,
        SERVICE_NAME,
        SERVICE_DISPLAY_NAME,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        szPath,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    );

    if (!hService)
    {
        DWORD dwError = GetLastError();
        CloseServiceHandle(hSCManager);

        if (dwError == ERROR_SERVICE_EXISTS)
        {
            wprintf(L"Service already exists\n");
            return TRUE;
        }

        wprintf(L"CreateService failed: %d\n", dwError);
        return FALSE;
    }

    // Set service description
    SERVICE_DESCRIPTIONW desc;
    desc.lpDescription = const_cast<LPWSTR>(SERVICE_DESCRIPTION);

    ChangeServiceConfig2W(
        hService,
        SERVICE_CONFIG_DESCRIPTION,
        &desc
    );

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    wprintf(L"Service installed successfully\n");
    return TRUE;
}

BOOL UninstallService()
{
    SC_HANDLE hSCManager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!hSCManager)
    {
        wprintf(L"OpenSCManager failed: %d\n", GetLastError());
        return FALSE;
    }

    SC_HANDLE hService = OpenServiceW(
        hSCManager,
        SERVICE_NAME,
        DELETE | SERVICE_STOP
    );

    if (!hService)
    {
        DWORD dwError = GetLastError();
        CloseServiceHandle(hSCManager);

        if (dwError == ERROR_SERVICE_DOES_NOT_EXIST)
        {
            wprintf(L"Service does not exist\n");
            return TRUE;
        }

        wprintf(L"OpenService failed: %d\n", dwError);
        return FALSE;
    }

    // Stop the service
    SERVICE_STATUS status;
    QueryServiceStatus(hService, &status);
    if (status.dwCurrentState != SERVICE_STOPPED)
    {
        wprintf(L"Stopping service...\n");
        ControlService(hService, SERVICE_CONTROL_STOP, &status);
        Sleep(1000);
        QueryServiceStatus(hService, &status);
    }

    // Delete the service
    if (!DeleteService(hService))
    {
        wprintf(L"DeleteService failed: %d\n", GetLastError());
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
        return FALSE;
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    wprintf(L"Service uninstalled successfully\n");
    return TRUE;
}

BOOL StartServiceWrapper()
{
    SC_HANDLE hSCManager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!hSCManager)
    {
        wprintf(L"OpenSCManager failed: %d\n", GetLastError());
        return FALSE;
    }

    SC_HANDLE hService = OpenServiceW(
        hSCManager,
        SERVICE_NAME,
        SERVICE_START | SERVICE_QUERY_STATUS
    );

    if (!hService)
    {
        wprintf(L"OpenService failed: %d\n", GetLastError());
        CloseServiceHandle(hSCManager);
        return FALSE;
    }

    BOOL result = StartService(hService, 0, nullptr);
    if (!result)
    {
        DWORD dwError = GetLastError();
        if (dwError == ERROR_SERVICE_ALREADY_RUNNING)
        {
            wprintf(L"Service is already running\n");
            result = TRUE;
        }
        else
        {
            wprintf(L"StartService failed: %d\n", dwError);
        }
    }
    else
    {
        wprintf(L"Service started successfully\n");
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return result;
}

BOOL StopServiceWrapper()
{
    SC_HANDLE hSCManager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!hSCManager)
    {
        wprintf(L"OpenSCManager failed: %d\n", GetLastError());
        return FALSE;
    }

    SC_HANDLE hService = OpenServiceW(
        hSCManager,
        SERVICE_NAME,
        SERVICE_STOP | SERVICE_QUERY_STATUS
    );

    if (!hService)
    {
        wprintf(L"OpenService failed: %d\n", GetLastError());
        CloseServiceHandle(hSCManager);
        return FALSE;
    }

    SERVICE_STATUS status;
    QueryServiceStatus(hService, &status);
    if (status.dwCurrentState == SERVICE_STOPPED)
    {
        wprintf(L"Service is already stopped\n");
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
        return TRUE;
    }

    wprintf(L"Stopping service...\n");
    BOOL result = ControlService(hService, SERVICE_CONTROL_STOP, &status);

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    if (result)
    {
        wprintf(L"Service stop requested\n");
    }
    else
    {
        wprintf(L"ControlService failed: %d\n", GetLastError());
    }

    return result;
}

// ================================================================
// Console Entry Point (for testing/standalone execution)
// ================================================================
int wmain(int argc, WCHAR* argv[])
{
    if (argc > 1)
    {
        if (_wcsicmp(argv[1], L"-install") == 0)
        {
            return InstallService() ? 0 : 1;
        }
        else if (_wcsicmp(argv[1], L"-uninstall") == 0)
        {
            return UninstallService() ? 0 : 1;
        }
        else if (_wcsicmp(argv[1], L"-start") == 0)
        {
            return StartServiceWrapper() ? 0 : 1;
        }
        else if (_wcsicmp(argv[1], L"-stop") == 0)
        {
            return StopServiceWrapper() ? 0 : 1;
        }
        else if (_wcsicmp(argv[1], L"-console") == 0)
        {
            // Run in console mode for testing
            wprintf(L"Running in console mode...\n");
            LoadCsvConfiguration();
            if (!StartTraceSession())
            {
                wprintf(L"Failed to start trace session\n");
                return 1;
            }

            wprintf(L"Press Enter to stop...\n");
            getchar();

            StopTraceSession();
            return 0;
        }
        else
        {
            wprintf(L"Usage:\n");
            wprintf(L"  EIDTraceConsumer -install   Install service\n");
            wprintf(L"  EIDTraceConsumer -uninstall Uninstall service\n");
            wprintf(L"  EIDTraceConsumer -start     Start service\n");
            wprintf(L"  EIDTraceConsumer -stop      Stop service\n");
            wprintf(L"  EIDTraceConsumer -console   Run in console (testing)\n");
            return 1;
        }
    }
    else
    {
        // Run as service
        wprintf(L"Starting service...\n");

        SERVICE_TABLE_ENTRY dispatchTable[] =
        {
            { const_cast<LPWSTR>(SERVICE_NAME), (LPSERVICE_MAIN_FUNCTION)ServiceMain },
            { nullptr, nullptr }
        };

        if (!StartServiceCtrlDispatcherW(dispatchTable))
        {
            wprintf(L"StartServiceCtrlDispatcherW failed: %d\n", GetLastError());
            wprintf(L"This program must be run as a service or with command-line arguments\n");
            return 1;
        }
    }

    return 0;
}
