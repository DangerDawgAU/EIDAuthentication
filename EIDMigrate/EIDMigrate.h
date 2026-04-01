#pragma once

// File: EIDMigrate/EIDMigrate.h
// Main application header

#include <Windows.h>
#include <string>
#include <vector>
#include "Tracing.h"

// Application information
#define EIDMIGRATE_APP_NAME        L"EIDMigrate"
#define EIDMIGRATE_APP_VERSION     L"1.0.0.0"
#define EIDMIGRATE_APP_COPYRIGHT   L"Copyright (C) 2026"

// Exit codes
#define EIDMIGRATE_EXIT_SUCCESS           0
#define EIDMIGRATE_EXIT_ERROR             1
#define EIDMIGRATE_EXIT_LSA_DENIED        2
#define EIDMIGRATE_EXIT_NO_CREDENTIALS    3
#define EIDMIGRATE_EXIT_FILE_WRITE        4
#define EIDMIGRATE_EXIT_INVALID_PASSPHRASE 5
#define EIDMIGRATE_EXIT_FILE_CORRUPTED    6
#define EIDMIGRATE_EXIT_NO_VALID_CREDS    7
#define EIDMIGRATE_EXIT_SOME_FAILED       8

// Command types
enum class COMMAND_TYPE
{
    NONE,
    EXPORT,
    IMPORT,
    LIST,
    VALIDATE,
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
    std::vector<std::wstring> SelectedGroups;  // Specific groups to export/import
    BOOL DryRun;
    BOOL Force;
    BOOL CreateUsers;
    BOOL ContinueOnError;
    BOOL ValidateCerts;
    BOOL ListLocal;
    VERBOSITY Verbosity;

    COMMAND_OPTIONS() :
        Command(COMMAND_TYPE::NONE),
        DryRun(FALSE),
        Force(FALSE),
        CreateUsers(FALSE),
        ContinueOnError(FALSE),
        ValidateCerts(FALSE),
        ListLocal(FALSE),
        Verbosity(VERBOSITY::NORMAL)
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
        hEventLog(nullptr),
        pLogFile(nullptr)
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
extern APP_STATE g_AppState;

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
