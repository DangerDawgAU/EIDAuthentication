#pragma once

// File: EIDMigrate/EIDMigrate.h
// Main application header

#include <Windows.h>
#include <string>
#include <vector>
#include "Tracing.h"

// Application information
#define EIDMIGRATE_APP_NAME        L"EIDMigrate"  // NOSONAR - MACRO-01: Windows-style macro constant retained for API/preprocessor use
#define EIDMIGRATE_APP_VERSION     L"1.0.0.0"  // NOSONAR - MACRO-01: Windows-style macro constant retained for API/preprocessor use
#define EIDMIGRATE_APP_COPYRIGHT   L"Copyright (C) 2026"  // NOSONAR - MACRO-01: Windows-style macro constant retained for API/preprocessor use

// Exit codes
#define EIDMIGRATE_EXIT_SUCCESS           0  // NOSONAR - MACRO-01: Windows-style macro constant retained for API/preprocessor use
#define EIDMIGRATE_EXIT_ERROR             1  // NOSONAR - MACRO-01: Windows-style macro constant retained for API/preprocessor use
#define EIDMIGRATE_EXIT_LSA_DENIED        2  // NOSONAR - MACRO-01: Windows-style macro constant retained for API/preprocessor use
#define EIDMIGRATE_EXIT_NO_CREDENTIALS    3  // NOSONAR - MACRO-01: Windows-style macro constant retained for API/preprocessor use
#define EIDMIGRATE_EXIT_FILE_WRITE        4  // NOSONAR - MACRO-01: Windows-style macro constant retained for API/preprocessor use
#define EIDMIGRATE_EXIT_INVALID_PASSPHRASE 5  // NOSONAR - MACRO-01: Windows-style macro constant retained for API/preprocessor use
#define EIDMIGRATE_EXIT_FILE_CORRUPTED    6  // NOSONAR - MACRO-01: Windows-style macro constant retained for API/preprocessor use
#define EIDMIGRATE_EXIT_NO_VALID_CREDS    7  // NOSONAR - MACRO-01: Windows-style macro constant retained for API/preprocessor use
#define EIDMIGRATE_EXIT_SOME_FAILED       8  // NOSONAR - MACRO-01: Windows-style macro constant retained for API/preprocessor use

// Command types
enum class COMMAND_TYPE
{
    NONE,  // NOSONAR - ENUM-01: enum kept for Win32/ABI compatibility
    EXPORT,
    IMPORT,
    LIST,
    VALIDATE,
    IMPORT_CRL,
    HELP,
    VERSION,
};

// Command options structure
struct COMMAND_OPTIONS
{
    COMMAND_TYPE Command;
    std::wstring InputFile;
    std::wstring OutputFile;
    std::wstring LogFile;
    std::wstring Password;
    std::wstring ExpectedSource;  // import: refuse a file not stamped by this issuing machine
    std::vector<std::wstring> SelectedGroups;  // Specific groups to export/import
    BOOL DryRun;
    BOOL Force;
    BOOL CreateUsers;
    BOOL ContinueOnError;
    BOOL ValidateCerts;
    BOOL ListLocal;
    VERBOSITY Verbosity;

    COMMAND_OPTIONS() :
        Command(COMMAND_TYPE::NONE),  // NOSONAR - INIT-01: constructor initializer list retained for clarity/ordering
        DryRun(FALSE),  // NOSONAR - INIT-01: constructor initializer list retained for clarity/ordering
        Force(FALSE),  // NOSONAR - INIT-01: constructor initializer list retained for clarity/ordering
        CreateUsers(FALSE),  // NOSONAR - INIT-01: constructor initializer list retained for clarity/ordering
        ContinueOnError(FALSE),  // NOSONAR - INIT-01: constructor initializer list retained for clarity/ordering
        ValidateCerts(FALSE),  // NOSONAR - INIT-01: constructor initializer list retained for clarity/ordering
        ListLocal(FALSE),  // NOSONAR - INIT-01: constructor initializer list retained for clarity/ordering
        Verbosity(VERBOSITY::NORMAL)  // NOSONAR - INIT-01: constructor initializer list retained for clarity/ordering
    {}
};

// Application state
struct APP_STATE
{
    COMMAND_OPTIONS Options;
    HANDLE hEventLog;
    std::wstring LogFilePath;
    FILE* pLogFile;

    APP_STATE() :
        hEventLog(nullptr),  // NOSONAR - INIT-01: constructor initializer list retained for clarity/ordering
        pLogFile(nullptr)  // NOSONAR - INIT-01: constructor initializer list retained for clarity/ordering
    {}

    // Delete copy/move operations - this is a singleton
    APP_STATE(const APP_STATE&) = delete;
    APP_STATE& operator=(const APP_STATE&) = delete;
    APP_STATE(APP_STATE&&) = delete;
    APP_STATE& operator=(APP_STATE&&) = delete;

    ~APP_STATE()
    {
        if (pLogFile)
        {
            fclose(pLogFile);
        }
        if (hEventLog)
        {
            DeregisterEventSource(hEventLog);
        }
    }
};

// Global application state
extern APP_STATE g_AppState;  // NOSONAR - GLOBAL-01: global application state mutated at runtime

// Main entry point
int wmain(_In_ int argc, _In_ PWSTR argv[]);

// Command parsing
BOOL ParseCommandLine(_In_ int argc, _In_ PWSTR argv[], _Out_ COMMAND_OPTIONS& options);
void ShowUsage();
void ShowVersion();

// Command handlers
HRESULT CommandExport(_In_ const COMMAND_OPTIONS& options);
HRESULT CommandImport(_In_ const COMMAND_OPTIONS& options);
HRESULT CommandList(_In_ const COMMAND_OPTIONS& options);
HRESULT CommandValidate(_In_ const COMMAND_OPTIONS& options);
HRESULT CommandImportCrl(_In_ const COMMAND_OPTIONS& options);
