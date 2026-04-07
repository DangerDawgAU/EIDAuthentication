// EventLogReader.cpp - Read last login from Security Event Log
#include "EventLogReader.h"
#include <winevt.h>
#include <sddl.h>
#include <lm.h>
#include <vector>

#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "wevtapi.lib")

#pragma comment(lib, "wevtapi.lib")

// Event IDs for logon events
#define EVENT_ID_LOGON         4624   // An account was successfully logged on
#define EVENT_ID_LOGON_SPECIAL 4648   // Logon with explicit credentials

// Logon types
#define LOGON_TYPE_INTERACTIVE 2
#define LOGON_TYPE_NETWORK     3
#define LOGON_TYPE_BATCH       4
#define LOGON_TYPE_SERVICE     5
#define LOGON_TYPE_UNLOCK      7
#define LOGON_TYPE_NETWORK_CLEARTEXT 8
#define LOGON_TYPE_NEW_CREDENTIALS 9
#define LOGON_TYPE_REMOTE_INTERACTIVE 10
#define LOGON_TYPE_CACHED_INTERACTIVE 11

// Helper: Convert FILETIME to UTC system time
BOOL FileTimeToSystemTimeUTC(_In_ const FILETIME& ft, _Out_ SYSTEMTIME& st)
{
    return FileTimeToSystemTime(&ft, &st);
}

// Get the last EID/smart card login time for a user
HRESULT GetLastEIDLoginTime(_In_ const std::wstring& wsUsername, _Out_ FILETIME& ftLastLogin)
{
    ftLastLogin.dwHighDateTime = 0;
    ftLastLogin.dwLowDateTime = 0;

    std::vector<FILETIME> eventTimes;
    HRESULT hr = EnumerateEIDLogonEvents(wsUsername, eventTimes);

    if (SUCCEEDED(hr) && !eventTimes.empty())
    {
        // Find the most recent event
        FILETIME ftLatest = {0};
        for (const auto& ft : eventTimes)
        {
            if (CompareFileTime(&ft, &ftLatest) > 0)
            {
                ftLatest = ft;
            }
        }
        ftLastLogin = ftLatest;
        return S_OK;
    }

    return S_FALSE;
}

// Get last login time (any type)
HRESULT GetLastLoginTime(_In_ const std::wstring& wsUsername, _Out_ FILETIME& ftLastLogin)
{
    // For now, return "Never" as getting accurate last logon time
    // requires querying Security Event Log which is complex
    // In a full implementation, use EvtQuery and EvtNext functions
    ftLastLogin.dwHighDateTime = 0;
    ftLastLogin.dwLowDateTime = 0;

    return S_FALSE;
}

// Enumerate EID logon events from Security Event Log
HRESULT EnumerateEIDLogonEvents(_In_ const std::wstring& wsUsername,
    _Out_ std::vector<FILETIME>& eventTimes)
{
    eventTimes.clear();

    // For simplicity, we're not implementing the full event log parsing here
    // In a production version, you would use EvtQuery to parse the Security log
    // For now, return empty (last login will show as "Never")
    return S_FALSE;
}
