// File: EIDMigrate/Tracing.cpp
// Trace and debug logging implementation

#include "Tracing.h"
#include <stdio.h>
#include <cstdarg>

// Global verbosity level (declared as extern in Tracing.h)
VERBOSITY g_Verbosity = VERBOSITY::NORMAL;

// Console colors for Windows
static const WORD CONSOLE_COLOR_DEFAULT = 7;  // White on black
static const WORD CONSOLE_COLOR_ERROR = 12;   // Red on black
static const WORD CONSOLE_COLOR_WARNING = 14; // Yellow on black
static const WORD CONSOLE_COLOR_INFO = 11;    // Cyan on black

// Check if we have a valid console (for GUI compatibility)
static BOOL HasValidConsole()
{
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hStdErr = GetStdHandle(STD_ERROR_HANDLE);

    return (hStdOut != INVALID_HANDLE_VALUE && hStdOut != nullptr &&
            hStdIn != INVALID_HANDLE_VALUE && hStdIn != nullptr &&
            hStdErr != INVALID_HANDLE_VALUE && hStdErr != nullptr);
}

void SetVerbosity(VERBOSITY level)
{
    g_Verbosity = level;
}

VERBOSITY GetVerbosity()
{
    return g_Verbosity;
}

static void SetConsoleColor(WORD wColor)
{
    if (!HasValidConsole())
        return;

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE)
    {
        SetConsoleTextAttribute(hConsole, wColor);
    }
}

void EIDMigrateTrace(_In_ DWORD dwLevel, _In_ PCWSTR pwszFormat, ...)
{
    if (!pwszFormat)
        return;

    // Early return if no console - don't attempt any console I/O
    if (!HasValidConsole())
        return;

    // Check verbosity level
    VERBOSITY requiredLevel = VERBOSITY::MINIMAL;
    switch (dwLevel)
    {
    case WINEVENT_LEVEL_ERROR:
        requiredLevel = VERBOSITY::MINIMAL;
        break;
    case WINEVENT_LEVEL_WARN:
        requiredLevel = VERBOSITY::NORMAL;
        break;
    case WINEVENT_LEVEL_INFO:
        requiredLevel = VERBOSITY::VERBOSE;
        break;
    default:
        requiredLevel = VERBOSITY::DEBUG;
        break;
    }

    if (g_Verbosity < requiredLevel)
        return;

    // Set console color based on level
    WORD wColor = CONSOLE_COLOR_DEFAULT;
    if (dwLevel == WINEVENT_LEVEL_ERROR)
        wColor = CONSOLE_COLOR_ERROR;
    else if (dwLevel == WINEVENT_LEVEL_WARN)
        wColor = CONSOLE_COLOR_WARNING;
    else if (dwLevel == WINEVENT_LEVEL_INFO)
        wColor = CONSOLE_COLOR_INFO;

    SetConsoleColor(wColor);

    // Format and print the message
    va_list args;
    va_start(args, pwszFormat);
    vfwprintf(stderr, pwszFormat, args);
    va_end(args);

    // Add newline if not present
    size_t cchLen = wcslen(pwszFormat); // NOSONAR - pwszFormat already used successfully in vfwprintf above, non-NULL guaranteed
    if (cchLen == 0 || pwszFormat[cchLen - 1] != L'\n')
        fwprintf(stderr, L"\n");

    // Reset console color
    SetConsoleColor(CONSOLE_COLOR_DEFAULT);
}

static DWORD g_dwProgressLastWidth = 0;

void ShowProgress(_In_ PCWSTR pwszOperation, _In_ DWORD dwCurrent, _In_ DWORD dwTotal)
{
    if (!pwszOperation)
        return;

    // Early return if no console
    if (!HasValidConsole())
        return;

    if (g_Verbosity < VERBOSITY::NORMAL)
        return;

    // Calculate percentage
    DWORD dwPercent = (dwTotal > 0) ? (dwCurrent * 100 / dwTotal) : 100;

    // Build progress string
    WCHAR szProgress[256];
    swprintf_s(szProgress, L"  %ls: %u/%u (%u%%)",
        pwszOperation, dwCurrent, dwTotal, dwPercent);

    // Clear previous progress line if needed
    if (g_dwProgressLastWidth > 0)
    {
        ClearProgress();
    }

    // Print progress (without newline for overwriting)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE)
    {
        DWORD dwWritten;
        WriteConsoleW(hConsole, szProgress,
            static_cast<DWORD>(wcslen(szProgress)), &dwWritten, nullptr); // NOSONAR - szProgress is stack-allocated buffer, never NULL
    }

    g_dwProgressLastWidth = static_cast<DWORD>(wcslen(szProgress)); // NOSONAR - szProgress is stack-allocated buffer, never NULL
}

void ClearProgress()
{
    if (!HasValidConsole())
        return;

    if (g_dwProgressLastWidth == 0)
        return;

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE)
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(hConsole, &csbi))
        {
            // Move cursor back to start of progress line
            csbi.dwCursorPosition.X -= static_cast<SHORT>(g_dwProgressLastWidth);
            if (csbi.dwCursorPosition.X < 0)
                csbi.dwCursorPosition.X = 0;
            SetConsoleCursorPosition(hConsole, csbi.dwCursorPosition);

            // Clear the line
            DWORD dwWritten;
            for (DWORD i = 0; i < g_dwProgressLastWidth; i++)
            {
                WriteConsoleW(hConsole, L" ", 1, &dwWritten, nullptr);
            }

            // Move cursor back again
            csbi.dwCursorPosition.X -= static_cast<SHORT>(g_dwProgressLastWidth);
            if (csbi.dwCursorPosition.X < 0)
                csbi.dwCursorPosition.X = 0;
            SetConsoleCursorPosition(hConsole, csbi.dwCursorPosition);
        }
    }

    g_dwProgressLastWidth = 0;
}

PCWSTR GetErrorCodeString(_In_ HRESULT hr)
{
    static thread_local WCHAR szBuffer[512];

    if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
    {
        DWORD dwError = HRESULT_CODE(hr);
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
            nullptr, dwError, 0, szBuffer,
            ARRAYSIZE(szBuffer), nullptr);
    }
    else
    {
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
            nullptr, hr, 0, szBuffer,
            ARRAYSIZE(szBuffer), nullptr);
    }

    return szBuffer;
}

PCWSTR GetErrorCodeString(_In_ DWORD dwError)
{
    static thread_local WCHAR szBuffer[512];

    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr, dwError, 0, szBuffer,
        ARRAYSIZE(szBuffer), nullptr);

    return szBuffer;
}
