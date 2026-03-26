#pragma once

// File: EIDMigrate/Tracing.h
// Trace and debug logging utilities

#include <Windows.h>
#include <evntrace.h>  // For winevent_level definitions

// Simplified winevent_level values if not available
#ifndef WINEVENT_LEVEL_MIN
#define WINEVENT_LEVEL_MIN  1
#define WINEVENT_LEVEL_INFO 2
#define WINEVENT_LEVEL_WARN 3
#define WINEVENT_LEVEL_ERROR 4
#define WINEVENT_LEVEL_MAX  5
#endif

// Verbosity levels for console output
enum class VERBOSITY : DWORD
{
    MINIMAL = 0,    // Only errors and final status
    NORMAL = 1,     // Progress and important messages
    VERBOSE = 2,    // Detailed information about each operation
    DEBUG = 3,      // All details including debug info
};

// Global verbosity setting (can be changed via command-line flags)
extern VERBOSITY g_Verbosity;

// Set verbosity level
void SetVerbosity(VERBOSITY level);

// Get current verbosity level
VERBOSITY GetVerbosity();

// Trace logging to console (respects verbosity level)
void EIDMigrateTrace(
    _In_ DWORD dwLevel,
    _In_ PCWSTR pwszFormat,
    ...
);

// Convenience macros
#define EIDM_TRACE_INFO(fmt, ...)  EIDMigrateTrace(WINEVENT_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define EIDM_TRACE_WARN(fmt, ...)  EIDMigrateTrace(WINEVENT_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define EIDM_TRACE_ERROR(fmt, ...) EIDMigrateTrace(WINEVENT_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define EIDM_TRACE_VERBOSE(fmt, ...) \
    do { if (GetVerbosity() >= VERBOSITY::VERBOSE) EIDMigrateTrace(WINEVENT_LEVEL_INFO, fmt, ##__VA_ARGS__); } while (0)
#define EIDM_TRACE_DEBUG(fmt, ...) \
    do { if (GetVerbosity() >= VERBOSITY::DEBUG) EIDMigrateTrace(WINEVENT_LEVEL_INFO, fmt, ##__VA_ARGS__); } while (0)

// Progress display (shows current operation and percentage)
void ShowProgress(
    _In_ PCWSTR pwszOperation,
    _In_ DWORD dwCurrent,
    _In_ DWORD dwTotal);

// Clear progress line
void ClearProgress();

// Error code to string conversion
PCWSTR GetErrorCodeString(_In_ HRESULT hr);
PCWSTR GetErrorCodeString(_In_ DWORD dwError);
