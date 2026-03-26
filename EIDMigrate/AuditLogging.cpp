// File: EIDMigrate/AuditLogging.cpp
// Audit logging to Windows Event Log and file

#include "AuditLogging.h"
#include "Tracing.h"
#include "Utils.h"
#include <fstream>
#include <sstream>

// Undefine Windows macros that conflict with our enum values
// (must come after includes that pull in Windows.h)
#ifdef ERROR
#undef ERROR // NOSONAR - Must undef Windows macro that conflicts with enum value
#endif
#ifdef WARNING
#undef WARNING // NOSONAR - Must undef Windows macro that conflicts with enum value
#endif

static HANDLE g_hEventLog = nullptr;
static std::wstring g_wsLogFilePath;

HRESULT InitializeAuditLogging()
{
    g_hEventLog = RegisterEventSourceW(nullptr, L"EIDMigrate");
    return (g_hEventLog != nullptr) ? S_OK : E_FAIL;
}

void LogAuditEvent(_In_ EID_AUDIT_EVENT_TYPE eventType, _In_opt_ PCWSTR pwszUsername, _In_opt_ PCWSTR pwszDetails)
{
    if (g_hEventLog)
    {
        WORD wType = GetEventLogLevel(eventType);
        DWORD dwEventID = static_cast<DWORD>(eventType);

        WCHAR szMessage[512];
        SecureZeroMemory(szMessage, sizeof(szMessage));

        switch (static_cast<DWORD>(eventType)) // NOSONAR - static_cast used for enum to underlying type; std::to_underlying is C++23, project uses earlier standard
        {
        case 1:  // EID_AUDIT_EVENT_TYPE::EXPORT_SUCCESS
            swprintf_s(szMessage, L"Successfully exported credentials. User: %s",
                pwszUsername ? pwszUsername : L"(system)");
            break;

        case 2:  // EID_AUDIT_EVENT_TYPE::EXPORT_FAILURE
            swprintf_s(szMessage, L"Failed to export credentials. User: %s, Details: %s",
                pwszUsername ? pwszUsername : L"(system)",
                pwszDetails ? pwszDetails : L"(none)");
            break;

        case 3:  // EID_AUDIT_EVENT_TYPE::IMPORT_SUCCESS
            swprintf_s(szMessage, L"Successfully imported credentials. User: %s",
                pwszUsername ? pwszUsername : L"(system)");
            break;

        case 4:  // EID_AUDIT_EVENT_TYPE::IMPORT_FAILURE
            swprintf_s(szMessage, L"Failed to import credentials. User: %s, Details: %s",
                pwszUsername ? pwszUsername : L"(system)",
                pwszDetails ? pwszDetails : L"(none)");
            break;

        case 5:  // EID_AUDIT_EVENT_TYPE::ACCESS_DENIED
            swprintf_s(szMessage, L"Access denied to LSA operations. User: %s",
                pwszUsername ? pwszUsername : L"(system)");
            break;

        case 9:  // EID_AUDIT_EVENT_TYPE::WARNING
            swprintf_s(szMessage, L"Warning: %s", pwszDetails ? pwszDetails : L"(none)");
            break;

        default:
            swprintf_s(szMessage, L"EIDMigrate event: %s",
                pwszDetails ? pwszDetails : L"(none)");
            break;
        }

        PCWSTR pwszStrings[1] = { szMessage }; // NOSONAR - C-style array required for Windows ReportEventW API which expects PCWSTR*
        ReportEventW(g_hEventLog, wType, 1, dwEventID, nullptr, 1, 0, pwszStrings, nullptr);
    }
}

void LogAuditEventToFile(_In_ EID_AUDIT_EVENT_TYPE eventType, _In_opt_ PCWSTR pwszUsername, _In_opt_ PCWSTR pwszDetails)
{
    if (!g_AppState.pLogFile && g_wsLogFilePath.empty())
        return;

    FILE* pLogFile = g_AppState.pLogFile;
    if (!pLogFile && !g_wsLogFilePath.empty())
    {
        if (_wfopen_s(&pLogFile, g_wsLogFilePath.c_str(), L"a, ccs=UTF-8") != 0)
            return;

        g_AppState.pLogFile = pLogFile;
    }

    if (!pLogFile)
        return;

    // Write timestamp
    fwprintf(pLogFile, L"[%ls] ", FormatCurrentTimestamp().c_str());

    // Write event type
    fwprintf(pLogFile, L"%ls ", GetEventTypeString(eventType));

    // Write username if provided
    if (pwszUsername)
        fwprintf(pLogFile, L"User: %ls ", pwszUsername);

    // Write details if provided
    if (pwszDetails)
        fwprintf(pLogFile, L"Details: %ls", pwszDetails);

    fwprintf(pLogFile, L"\n");
    fflush(pLogFile);
}

void LogAuditEventBoth(_In_ EID_AUDIT_EVENT_TYPE eventType, _In_opt_ PCWSTR pwszUsername, _In_opt_ PCWSTR pwszDetails)
{
    LogAuditEvent(eventType, pwszUsername, pwszDetails);
    LogAuditEventToFile(eventType, pwszUsername, pwszDetails);
}

HRESULT SetLogFile(_In_ const std::wstring& wsFilePath)
{
    // Close existing log file
    CloseLogFile();

    FILE* pLogFile = nullptr;
    if (_wfopen_s(&pLogFile, wsFilePath.c_str(), L"a, ccs=UTF-8") != 0)
        return E_FAIL;

    g_wsLogFilePath = wsFilePath;
    g_AppState.LogFilePath = wsFilePath;
    g_AppState.pLogFile = pLogFile;

    return S_OK;
}

void CloseLogFile()
{
    if (g_AppState.pLogFile)
    {
        fclose(g_AppState.pLogFile);
        g_AppState.pLogFile = nullptr;
    }
}

void FlushLogFile()
{
    if (g_AppState.pLogFile)
    {
        fflush(g_AppState.pLogFile);
    }
}

PCWSTR GetEventTypeString(EID_AUDIT_EVENT_TYPE eventType)
{
    switch (static_cast<DWORD>(eventType)) // NOSONAR - static_cast used for enum to underlying type; std::to_underlying is C++23, project uses earlier standard
    {
    case 1: return L"EXPORT_SUCCESS";          // EID_AUDIT_EVENT_TYPE::EXPORT_SUCCESS
    case 2: return L"EXPORT_FAILURE";          // EID_AUDIT_EVENT_TYPE::EXPORT_FAILURE
    case 3: return L"IMPORT_SUCCESS";          // EID_AUDIT_EVENT_TYPE::IMPORT_SUCCESS
    case 4: return L"IMPORT_FAILURE";          // EID_AUDIT_EVENT_TYPE::IMPORT_FAILURE
    case 5: return L"ACCESS_DENIED";           // EID_AUDIT_EVENT_TYPE::ACCESS_DENIED
    case 6: return L"AUTH_FAILURE";            // EID_AUDIT_EVENT_TYPE::AUTH_FAILURE
    case 7: return L"VALIDATION_SUCCESS";      // EID_AUDIT_EVENT_TYPE::VALIDATION_SUCCESS
    case 8: return L"VALIDATION_FAILURE";      // EID_AUDIT_EVENT_TYPE::VALIDATION_FAILURE
    case 9: return L"WARNING";                 // EID_AUDIT_EVENT_TYPE::WARNING
    default: return L"UNKNOWN";
    }
}

WORD GetEventLogLevel(EID_AUDIT_EVENT_TYPE eventType)
{
    switch (static_cast<DWORD>(eventType)) // NOSONAR - static_cast used for enum to underlying type; std::to_underlying is C++23, project uses earlier standard
    {
    case 2:  // EID_AUDIT_EVENT_TYPE::EXPORT_FAILURE
    case 4:  // EID_AUDIT_EVENT_TYPE::IMPORT_FAILURE
    case 5:  // EID_AUDIT_EVENT_TYPE::ACCESS_DENIED
    case 6:  // EID_AUDIT_EVENT_TYPE::AUTH_FAILURE
    case 8:  // EID_AUDIT_EVENT_TYPE::VALIDATION_FAILURE
        return EVENTLOG_ERROR_TYPE;

    case 9:  // EID_AUDIT_EVENT_TYPE::WARNING
        return EVENTLOG_WARNING_TYPE;

    default:
        return EVENTLOG_INFORMATION_TYPE;
    }
}
