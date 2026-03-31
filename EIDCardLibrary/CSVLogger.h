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
 *  CSV Logger
 *
 *  Thread-safe CSV logger with LSASS-safe design.
 *  Stack-allocated buffers only, no dynamic memory allocation.
 *  Supports file rotation and configurable column output.
 */

#pragma once

#include "CSVConfig.h"
#include "EventDefinitions.h"
#include <Windows.h>

// ================================================================
// CSV Logger Class
// ================================================================
class EIDCSVLogger
{
private:
    // Thread synchronization
    static CRITICAL_SECTION s_csLogger;
    static BOOL s_csInitialized;

    // File handle
    static HANDLE s_hLogFile;

    // Current configuration
    static EID_CSV_CONFIG s_config;

    // File state
    static DWORD s_dwCurrentFileSize;
    static WCHAR s_szCurrentLogPath[MAX_PATH];
    static BOOL s_fHeaderWritten;

    // Private helper methods
    static BOOL EnsureLogFileOpen();
    static void RotateLogFile();
    static void WriteCSVHeader();
    static void WriteEscapedCSVField(PCWSTR pwszValue);
    static BOOL IsEventEnabled(EID_EVENT_ID eventId, EID_SEVERITY severity);
    static void FormatTimestamp(WCHAR* pszBuffer, size_t cchBuffer);

public:
    // ================================================================
    // Initialization and Cleanup
    // ================================================================

    // Initialize the logger with configuration
    static HRESULT Initialize(const EID_CSV_CONFIG& config);

    // Shutdown the logger and close file
    static void Shutdown();

    // Update configuration at runtime
    static HRESULT UpdateConfig(const EID_CSV_CONFIG& config);

    // Get current configuration
    static EID_CSV_CONFIG GetConfig();

    // Check if CSV logging is enabled
    static BOOL IsEnabled();

    // ================================================================
    // Core Logging Function
    // ================================================================

    // Log an event with full parameter set
    static void LogEvent(
        EID_EVENT_ID eventId,
        EID_SEVERITY severity,
        EID_OUTCOME outcome,
        PCWSTR pwszUsername,
        PCWSTR pwszAction,
        PCWSTR pwszMessage,
        // Optional extended fields
        PCWSTR pwszDomain = nullptr,
        PCWSTR pwszSourceIP = nullptr,
        DWORD dwProcessID = 0,
        DWORD dwThreadID = 0,
        DWORD dwSessionID = 0,
        PCWSTR pwszResource = nullptr,
        PCWSTR pwszReason = nullptr
    );

    // ================================================================
    // Convenience Logging Functions
    // ================================================================

    // Log successful event
    static void LogSuccess(EID_EVENT_ID eventId, PCWSTR pwszAction, PCWSTR pwszMessage);

    // Log failed event
    static void LogFailure(EID_EVENT_ID eventId, PCWSTR pwszAction, PCWSTR pwszReason);

    // Log authentication success
    static void LogAuthSuccess(PCWSTR pwszUsername, PCWSTR pwszMessage);

    // Log authentication failure
    static void LogAuthFailure(PCWSTR pwszUsername, PCWSTR pwszReason);

    // ================================================================
    // Flush and Status
    // ================================================================

    // Flush any buffered output to disk
    static void Flush();

    // Get current file size in bytes
    static DWORD GetCurrentFileSize();
};

// ================================================================
// Convenience Macros for Logging
// ================================================================
#define EID_CSV_LOG_SUCCESS(eventId, action, msg) \
    EIDCSVLogger::LogSuccess(eventId, action, msg)

#define EID_CSV_LOG_FAILURE(eventId, action, reason) \
    EIDCSVLogger::LogFailure(eventId, action, reason)

#define EID_CSV_LOG_AUTH_SUCCESS(user, msg) \
    EIDCSVLogger::LogAuthSuccess(user, msg)

#define EID_CSV_LOG_AUTH_FAILURE(user, reason) \
    EIDCSVLogger::LogAuthFailure(user, reason)
