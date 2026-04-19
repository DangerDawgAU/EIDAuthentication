// EventLogReader.h - Read last login time from Windows Security Event Log
#pragma once
#include <Windows.h>
#include <string>
#include <vector>

// Get the last EID/smart card login time for a user from Security Event Log
// Returns S_OK with ftLastLogin set, or S_FALSE if no logins found
HRESULT GetLastEIDLoginTime(_In_ const std::wstring& wsUsername, _Out_ FILETIME& ftLastLogin);

// Get the last login time (any type) for a user
HRESULT GetLastLoginTime(_In_ const std::wstring& wsUsername, _Out_ FILETIME& ftLastLogin);

// Parse event log for EID authentication events
HRESULT EnumerateEIDLogonEvents(_In_ const std::wstring& wsUsername,
    _Out_ std::vector<FILETIME>& eventTimes);

// Helper: Convert FILETIME to UTC system time
BOOL FileTimeToSystemTimeUTC(_In_ const FILETIME& ft, _Out_ SYSTEMTIME& st);
