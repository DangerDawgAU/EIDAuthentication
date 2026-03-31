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
 *  CSV Logger Implementation
 *
 *  Thread-safe CSV logger with LSASS-safe design.
 */

#include "CSVLogger.h"
#include "../EIDMigrate/Utils.h"
#include <string>
#include <cmath>

// ================================================================
// Static Member Initialization
// ================================================================
CRITICAL_SECTION EIDCSVLogger::s_csLogger;
BOOL EIDCSVLogger::s_csInitialized = FALSE;
HANDLE EIDCSVLogger::s_hLogFile = INVALID_HANDLE_VALUE;
EID_CSV_CONFIG EIDCSVLogger::s_config;
DWORD EIDCSVLogger::s_dwCurrentFileSize = 0;
WCHAR EIDCSVLogger::s_szCurrentLogPath[MAX_PATH] = {0};
BOOL EIDCSVLogger::s_fHeaderWritten = FALSE;

// ================================================================
// Helper: Format Timestamp as ISO 8601
// ================================================================
void EIDCSVLogger::FormatTimestamp(WCHAR* pszBuffer, size_t cchBuffer)
{
    SYSTEMTIME st;
    GetSystemTime(&st);  // UTC time

    swprintf_s(pszBuffer, cchBuffer,
        L"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

// ================================================================
// Helper: Check if event should be logged
// ================================================================
BOOL EIDCSVLogger::IsEventEnabled(EID_EVENT_ID eventId, EID_SEVERITY severity)
{
    // Check if logger is enabled
    if (!s_config.fEnabled)
        return FALSE;

    // Check verbose filter
    if (severity == EID_SEVERITY::VERBOSE && !s_config.fVerboseEvents)
        return FALSE;

    // Check category filter
    EID_EVENT_CATEGORY category = GetEventCategory(eventId);
    if (!IsCategoryEnabled(s_config.dwCategoryFilter, category))
        return FALSE;

    return TRUE;
}

// ================================================================
// Helper: Write CSV Header
// ================================================================
void EIDCSVLogger::WriteCSVHeader()
{
    if (s_fHeaderWritten)
        return;

    WCHAR szHeader[2048];
    size_t offset = 0;
    EID_CSV_COLUMN cols = s_config.dwColumns;

    // Helper lambda to append field name
    auto appendField = [&szHeader, &offset](PCWSTR pwszName)
    {
        if (offset > 0)
            szHeader[offset++] = L',';
        size_t len = wcslen(pwszName);
        wcsncpy_s(szHeader + offset, _countof(szHeader) - offset, pwszName, _TRUNCATE);
        offset += len;
    };

    if (cols & EID_CSV_COLUMN::TIMESTAMP)    appendField(L"Timestamp");
    if (cols & EID_CSV_COLUMN::EVENT_ID)     appendField(L"EventID");
    if (cols & EID_CSV_COLUMN::CATEGORY)     appendField(L"Category");
    if (cols & EID_CSV_COLUMN::SEVERITY)     appendField(L"Severity");
    if (cols & EID_CSV_COLUMN::OUTCOME)      appendField(L"Outcome");
    if (cols & EID_CSV_COLUMN::USERNAME)     appendField(L"Username");
    if (cols & EID_CSV_COLUMN::DOMAIN)       appendField(L"Domain");
    if (cols & EID_CSV_COLUMN::CLIENT_IP)    appendField(L"SourceIP");
    if (cols & EID_CSV_COLUMN::ACTION)       appendField(L"Action");
    if (cols & EID_CSV_COLUMN::TARGET)     appendField(L"Resource");
    if (cols & EID_CSV_COLUMN::REASON)       appendField(L"Reason");
    if (cols & EID_CSV_COLUMN::LOGIN_SESSION)   appendField(L"SessionID");
    if (cols & EID_CSV_COLUMN::PROCESS_ID)   appendField(L"ProcessID");
    if (cols & EID_CSV_COLUMN::MESSAGE)      appendField(L"Message");

    szHeader[offset++] = L'\r';
    szHeader[offset++] = L'\n';
    szHeader[offset] = L'\0';

    // Write header
    DWORD dwWritten = 0;
    WriteFile(s_hLogFile, szHeader, static_cast<DWORD>(offset * sizeof(WCHAR)), &dwWritten, nullptr);
    s_dwCurrentFileSize += dwWritten;
    s_fHeaderWritten = TRUE;
}

// ================================================================
// Helper: Write Escaped CSV Field
// ================================================================
void EIDCSVLogger::WriteEscapedCSVField(PCWSTR pwszValue)
{
    if (!pwszValue || !pwszValue[0])
        return;

    // Check if field needs quoting
    BOOL fNeedsQuotes = FALSE;
    for (PCWSTR p = pwszValue; *p; p++)
    {
        if (*p == L',' || *p == L'"' || *p == L'\n' || *p == L'\r')
        {
            fNeedsQuotes = TRUE;
            break;
        }
    }

    if (!fNeedsQuotes)
    {
        // No escaping needed, write as-is
        DWORD dwLen = static_cast<DWORD>(wcslen(pwszValue));
        DWORD dwWritten = 0;
        WriteFile(s_hLogFile, pwszValue, dwLen * sizeof(WCHAR), &dwWritten, nullptr);
        s_dwCurrentFileSize += dwWritten;
        return;
    }

    // Needs escaping: wrap in quotes, double inner quotes
    DWORD dwWritten = 0;
    WriteFile(s_hLogFile, L"\"", 1 * sizeof(WCHAR), &dwWritten, nullptr);
    s_dwCurrentFileSize += dwWritten;

    for (PCWSTR p = pwszValue; *p; p++)
    {
        if (*p == L'"')
        {
            WriteFile(s_hLogFile, L"\"\"", 2 * sizeof(WCHAR), &dwWritten, nullptr);
        }
        else
        {
            WriteFile(s_hLogFile, p, 1 * sizeof(WCHAR), &dwWritten, nullptr);
        }
        s_dwCurrentFileSize += dwWritten;
    }

    WriteFile(s_hLogFile, L"\"", 1 * sizeof(WCHAR), &dwWritten, nullptr);
    s_dwCurrentFileSize += dwWritten;
}

// ================================================================
// Helper: Rotate Log File
// ================================================================
void EIDCSVLogger::RotateLogFile()
{
    // Close current file
    if (s_hLogFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(s_hLogFile);
        s_hLogFile = INVALID_HANDLE_VALUE;
    }

    // Determine rotation number
    DWORD dwRotation = 1;
    WCHAR szRotatedPath[MAX_PATH];

    // Find next available rotation slot
    for (DWORD i = 1; i <= s_config.dwFileCount; i++)
    {
        swprintf_s(szRotatedPath, L"%s.%03u", s_szCurrentLogPath, i);
        if (GetFileAttributesW(szRotatedPath) == INVALID_FILE_ATTRIBUTES)
        {
            dwRotation = i;
            break;
        }
    }

    // If all slots filled, rotate all files
    if (dwRotation > s_config.dwFileCount)
    {
        // Delete oldest file
        swprintf_s(szRotatedPath, L"%s.%03u", s_szCurrentLogPath, s_config.dwFileCount);
        DeleteFileW(szRotatedPath);

        // Rename remaining files
        for (DWORD i = s_config.dwFileCount - 1; i >= 1; i--)
        {
            WCHAR szOldPath[MAX_PATH];
            swprintf_s(szOldPath, L"%s.%03u", s_szCurrentLogPath, i);
            swprintf_s(szRotatedPath, L"%s.%03u", s_szCurrentLogPath, i + 1);
            MoveFileExW(szOldPath, szRotatedPath, MOVEFILE_REPLACE_EXISTING);
        }
        dwRotation = 1;
    }

    // Rename current file to rotated
    swprintf_s(szRotatedPath, L"%s.%03u", s_szCurrentLogPath, dwRotation);
    MoveFileExW(s_szCurrentLogPath, szRotatedPath, MOVEFILE_REPLACE_EXISTING);

    // Reset state
    s_dwCurrentFileSize = 0;
    s_fHeaderWritten = FALSE;

    // Open new file
    EnsureLogFileOpen();
}

// ================================================================
// Helper: Ensure Log File is Open
// ================================================================
BOOL EIDCSVLogger::EnsureLogFileOpen()
{
    if (s_hLogFile != INVALID_HANDLE_VALUE)
        return TRUE;

    // Create directory if needed
    WCHAR szDir[MAX_PATH];
    wcscpy_s(szDir, s_szCurrentLogPath);
    WCHAR* pLastSlash = wcsrchr(szDir, L'\\');
    if (pLastSlash)
    {
        *pLastSlash = L'\0';
        CreateDirectoryW(szDir, nullptr);
    }

    // Open file for append (UTF-16 LE encoding)
    s_hLogFile = CreateFileW(
        s_szCurrentLogPath,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (s_hLogFile == INVALID_HANDLE_VALUE)
        return FALSE;

    // Get current file size
    LARGE_INTEGER liSize;
    if (GetFileSizeEx(s_hLogFile, &liSize))
    {
        s_dwCurrentFileSize = static_cast<DWORD>(liSize.QuadPart);
    }

    // Seek to end for append
    SetFilePointer(s_hLogFile, 0, nullptr, FILE_END);

    // Check if file is empty and needs header
    if (s_dwCurrentFileSize == 0)
    {
        // Write UTF-16 LE BOM
        DWORD dwWritten = 0;
        WORD wBOM = 0xFEFF;
        WriteFile(s_hLogFile, &wBOM, sizeof(WORD), &dwWritten, nullptr);
        s_dwCurrentFileSize += dwWritten;

        WriteCSVHeader();
    }

    return TRUE;
}

// ================================================================
// Initialize Logger
// ================================================================
HRESULT EIDCSVLogger::Initialize(const EID_CSV_CONFIG& config)
{
    // Initialize critical section if needed
    if (!s_csInitialized)
    {
        InitializeCriticalSection(&s_csLogger);
        s_csInitialized = TRUE;
    }

    EnterCriticalSection(&s_csLogger);

    __try
    {
        // Store configuration
        s_config = config;

        // Close existing file if open
        if (s_hLogFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(s_hLogFile);
            s_hLogFile = INVALID_HANDLE_VALUE;
        }

        // If logging disabled, we're done
        if (!config.fEnabled)
            return S_OK;

        // Set log path
        if (config.szLogPath[0] != L'\0')
        {
            wcscpy_s(s_szCurrentLogPath, config.szLogPath);
        }
        else
        {
            wcscpy_s(s_szCurrentLogPath, EID_CSV_DEFAULT_LOG_PATH);
        }

        // Reset state
        s_dwCurrentFileSize = 0;
        s_fHeaderWritten = FALSE;

        // Open log file
        if (!EnsureLogFileOpen())
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        return S_OK;
    }
    __finally
    {
        LeaveCriticalSection(&s_csLogger);
    }
}

// ================================================================
// Shutdown Logger
// ================================================================
void EIDCSVLogger::Shutdown()
{
    EnterCriticalSection(&s_csLogger);

    __try
    {
        if (s_hLogFile != INVALID_HANDLE_VALUE)
        {
            FlushFileBuffers(s_hLogFile);
            CloseHandle(s_hLogFile);
            s_hLogFile = INVALID_HANDLE_VALUE;
        }

        s_dwCurrentFileSize = 0;
        s_fHeaderWritten = FALSE;
    }
    __finally
    {
        LeaveCriticalSection(&s_csLogger);
    }
}

// ================================================================
// Update Configuration
// ================================================================
HRESULT EIDCSVLogger::UpdateConfig(const EID_CSV_CONFIG& config)
{
    EnterCriticalSection(&s_csLogger);

    __try
    {
        s_config = config;

        // If logging was disabled and now enabled, open file
        if (config.fEnabled && s_hLogFile == INVALID_HANDLE_VALUE)
        {
            wcscpy_s(s_szCurrentLogPath, config.szLogPath[0] ? config.szLogPath : EID_CSV_DEFAULT_LOG_PATH);
            s_dwCurrentFileSize = 0;
            s_fHeaderWritten = FALSE;
            if (!EnsureLogFileOpen())
                return HRESULT_FROM_WIN32(GetLastError());
        }
        // If logging was enabled and now disabled, close file
        else if (!config.fEnabled && s_hLogFile != INVALID_HANDLE_VALUE)
        {
            FlushFileBuffers(s_hLogFile);
            CloseHandle(s_hLogFile);
            s_hLogFile = INVALID_HANDLE_VALUE;
        }

        return S_OK;
    }
    __finally
    {
        LeaveCriticalSection(&s_csLogger);
    }
}

// ================================================================
// Get Current Configuration
// ================================================================
EID_CSV_CONFIG EIDCSVLogger::GetConfig()
{
    EnterCriticalSection(&s_csLogger);
    EID_CSV_CONFIG config = s_config;
    LeaveCriticalSection(&s_csLogger);
    return config;
}

// ================================================================
// Check if Enabled
// ================================================================
BOOL EIDCSVLogger::IsEnabled()
{
    return s_config.fEnabled && s_hLogFile != INVALID_HANDLE_VALUE;
}

// ================================================================
// Core Logging Function
// ================================================================
void EIDCSVLogger::LogEvent(
    EID_EVENT_ID eventId,
    EID_SEVERITY severity,
    EID_OUTCOME outcome,
    PCWSTR pwszUsername,
    PCWSTR pwszAction,
    PCWSTR pwszMessage,
    PCWSTR pwszDomain,
    PCWSTR pwszSourceIP,
    DWORD dwProcessID,
    DWORD dwThreadID,
    DWORD dwSessionID,
    PCWSTR pwszResource,
    PCWSTR pwszReason)
{
    // Check if event should be logged
    if (!IsEventEnabled(eventId, severity))
        return;

    EnterCriticalSection(&s_csLogger);

    __try
    {
        // Ensure file is open
        if (!EnsureLogFileOpen())
            __leave;

        // Check file rotation
        DWORD dwMaxBytes = s_config.dwMaxFileSizeMB * 1024 * 1024;
        if (s_dwCurrentFileSize >= dwMaxBytes)
        {
            RotateLogFile();
            if (!EnsureLogFileOpen())
                __leave;
        }

        // Stack-allocated buffer for building log line
        WCHAR szBuffer[4096];
        DWORD dwWritten = 0;
        EID_CSV_COLUMN cols = s_config.dwColumns;
        BOOL fFirstField = TRUE;

        // Helper lambda to write field with comma separator
        auto writeField = [&szBuffer, &dwWritten, &fFirstField](PCWSTR pwszValue)
        {
            if (!fFirstField)
                szBuffer[dwWritten++] = L',';
            else
                fFirstField = FALSE;
            return dwWritten;
        };

        // Timestamp
        if (cols & EID_CSV_COLUMN::TIMESTAMP)
        {
            writeField(L"");
            FormatTimestamp(szBuffer + dwWritten, _countof(szBuffer) - dwWritten);
            dwWritten += static_cast<DWORD>(wcslen(szBuffer + dwWritten));
        }

        // Event ID
        if (cols & EID_CSV_COLUMN::EVENT_ID)
        {
            writeField(L"");
            dwWritten += swprintf_s(szBuffer + dwWritten, _countof(szBuffer) - dwWritten, L"%u", static_cast<DWORD>(eventId));
        }

        // Category
        if (cols & EID_CSV_COLUMN::CATEGORY)
        {
            writeField(L"");
            PCWSTR pwszCategory = GetCategoryName(GetEventCategory(eventId));
            dwWritten += swprintf_s(szBuffer + dwWritten, _countof(szBuffer) - dwWritten, L"%s", pwszCategory);
        }

        // Severity
        if (cols & EID_CSV_COLUMN::SEVERITY)
        {
            writeField(L"");
            dwWritten += swprintf_s(szBuffer + dwWritten, _countof(szBuffer) - dwWritten, L"%s", GetSeverityName(static_cast<UCHAR>(severity)));
        }

        // Outcome
        if (cols & EID_CSV_COLUMN::OUTCOME)
        {
            writeField(L"");
            dwWritten += swprintf_s(szBuffer + dwWritten, _countof(szBuffer) - dwWritten, L"%s", GetOutcomeName(static_cast<UCHAR>(outcome)));
        }

        // Username
        if (cols & EID_CSV_COLUMN::USERNAME)
        {
            writeField(L"");
            WriteFile(s_hLogFile, szBuffer, dwWritten * sizeof(WCHAR), &dwWritten, nullptr);
            s_dwCurrentFileSize += dwWritten;
            WriteEscapedCSVField(pwszUsername ? pwszUsername : L"");
            dwWritten = 0;
            fFirstField = FALSE;
        }

        // Domain
        if (cols & EID_CSV_COLUMN::DOMAIN)
        {
            writeField(L"");
            WriteFile(s_hLogFile, szBuffer, dwWritten * sizeof(WCHAR), &dwWritten, nullptr);
            s_dwCurrentFileSize += dwWritten;
            WriteEscapedCSVField(pwszDomain);
            dwWritten = 0;
            fFirstField = FALSE;
        }

        // Source IP
        if (cols & EID_CSV_COLUMN::CLIENT_IP)
        {
            writeField(L"");
            WriteFile(s_hLogFile, szBuffer, dwWritten * sizeof(WCHAR), &dwWritten, nullptr);
            s_dwCurrentFileSize += dwWritten;
            WriteEscapedCSVField(pwszSourceIP);
            dwWritten = 0;
            fFirstField = FALSE;
        }

        // Action
        if (cols & EID_CSV_COLUMN::ACTION)
        {
            writeField(L"");
            WriteFile(s_hLogFile, szBuffer, dwWritten * sizeof(WCHAR), &dwWritten, nullptr);
            s_dwCurrentFileSize += dwWritten;
            WriteEscapedCSVField(pwszAction ? pwszAction : L"");
            dwWritten = 0;
            fFirstField = FALSE;
        }

        // Resource
        if (cols & EID_CSV_COLUMN::TARGET)
        {
            writeField(L"");
            WriteFile(s_hLogFile, szBuffer, dwWritten * sizeof(WCHAR), &dwWritten, nullptr);
            s_dwCurrentFileSize += dwWritten;
            WriteEscapedCSVField(pwszResource);
            dwWritten = 0;
            fFirstField = FALSE;
        }

        // Reason
        if (cols & EID_CSV_COLUMN::REASON)
        {
            writeField(L"");
            WriteFile(s_hLogFile, szBuffer, dwWritten * sizeof(WCHAR), &dwWritten, nullptr);
            s_dwCurrentFileSize += dwWritten;
            WriteEscapedCSVField(pwszReason);
            dwWritten = 0;
            fFirstField = FALSE;
        }

        // Session ID
        if (cols & EID_CSV_COLUMN::LOGIN_SESSION)
        {
            writeField(L"");
            dwWritten += swprintf_s(szBuffer + dwWritten, _countof(szBuffer) - dwWritten, L"%u", dwSessionID);
        }

        // Process ID
        if (cols & EID_CSV_COLUMN::PROCESS_ID)
        {
            writeField(L"");
            dwWritten += swprintf_s(szBuffer + dwWritten, _countof(szBuffer) - dwWritten, L"%u", dwProcessID ? dwProcessID : GetCurrentProcessId());
        }

        // Message
        if (cols & EID_CSV_COLUMN::MESSAGE)
        {
            writeField(L"");
            WriteFile(s_hLogFile, szBuffer, dwWritten * sizeof(WCHAR), &dwWritten, nullptr);
            s_dwCurrentFileSize += dwWritten;
            WriteEscapedCSVField(pwszMessage ? pwszMessage : L"");
            dwWritten = 0;
            fFirstField = FALSE;
        }

        // Write line ending
        if (dwWritten > 0 || !fFirstField)
        {
            WriteFile(s_hLogFile, szBuffer, dwWritten * sizeof(WCHAR), &dwWritten, nullptr);
            s_dwCurrentFileSize += dwWritten;
        }
        WriteFile(s_hLogFile, L"\r\n", 2 * sizeof(WCHAR), &dwWritten, nullptr);
        s_dwCurrentFileSize += dwWritten;

        // Flush for immediate visibility (optional, can be disabled for performance)
        // FlushFileBuffers(s_hLogFile);
    }
    __finally
    {
        LeaveCriticalSection(&s_csLogger);
    }
}

// ================================================================
// Convenience: Log Success
// ================================================================
void EIDCSVLogger::LogSuccess(EID_EVENT_ID eventId, PCWSTR pwszAction, PCWSTR pwszMessage)
{
    LogEvent(eventId, EID_SEVERITY::INFO, EID_OUTCOME::SUCCESS,
        nullptr, pwszAction, pwszMessage);
}

// ================================================================
// Convenience: Log Failure
// Note: Uses WARNING severity by default. For actual errors, call LogEvent directly with ERROR.
// ================================================================
void EIDCSVLogger::LogFailure(EID_EVENT_ID eventId, PCWSTR pwszAction, PCWSTR pwszReason)
{
    LogEvent(eventId, EID_SEVERITY::WARNING, EID_OUTCOME::FAILURE,
        nullptr, pwszAction, L"Failed", nullptr, nullptr, 0, 0, 0, nullptr, pwszReason);
}

// ================================================================
// Convenience: Log Auth Success
// ================================================================
void EIDCSVLogger::LogAuthSuccess(PCWSTR pwszUsername, PCWSTR pwszMessage)
{
    LogEvent(EID_EVENT_ID::AUTH_SUCCESS, EID_SEVERITY::INFO, EID_OUTCOME::SUCCESS,
        pwszUsername, L"Authentication", pwszMessage);
}

// ================================================================
// Convenience: Log Auth Failure
// Note: Authentication failures are logged at WARNING level, not ERROR.
// User entering wrong credentials is expected behavior, not a system error.
// ================================================================
void EIDCSVLogger::LogAuthFailure(PCWSTR pwszUsername, PCWSTR pwszReason)
{
    LogEvent(EID_EVENT_ID::AUTH_FAILURE, EID_SEVERITY::WARNING, EID_OUTCOME::FAILURE,
        pwszUsername, L"Authentication", L"Failed", nullptr, nullptr, 0, 0, 0, nullptr, pwszReason);
}

// ================================================================
// Flush Output
// ================================================================
void EIDCSVLogger::Flush()
{
    EnterCriticalSection(&s_csLogger);

    __try
    {
        if (s_hLogFile != INVALID_HANDLE_VALUE)
        {
            FlushFileBuffers(s_hLogFile);
        }
    }
    __finally
    {
        LeaveCriticalSection(&s_csLogger);
    }
}

// ================================================================
// Get Current File Size
// ================================================================
DWORD EIDCSVLogger::GetCurrentFileSize()
{
    EnterCriticalSection(&s_csLogger);
    DWORD dwSize = s_dwCurrentFileSize;
    LeaveCriticalSection(&s_csLogger);
    return dwSize;
}
