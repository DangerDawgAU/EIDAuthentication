// WorkerThread.h - Background thread worker for async operations
// Copyright (c) 2026

#pragma once

#include <Windows.h>
#include <string>
#include <functional>

// Worker thread context
struct WORKER_CONTEXT {
    HWND hwndParent;           // Parent window to send messages to
    UINT uProgressMsg;         // Message for progress updates
    UINT uCompleteMsg;         // Message for completion
    UINT uErrorMsg;            // Message for errors

    // Operation-specific data
    std::wstring* pwszOutputFile;
    std::wstring* pwszInputFile;
    std::wstring* pwszPassword;
    BOOL* pfValidateCerts;
    BOOL* pfIncludeGroups;
    BOOL* pfDryRun;
    BOOL* pfCreateUsers;
    BOOL* pfContinueOnError;
    DWORD* pdwResultCount;
    HRESULT* phrResult;

    // User passwords from prompts (username -> password map)
    std::vector<std::pair<std::wstring, std::wstring>>* pUserPasswords;

    WORKER_CONTEXT() :
        hwndParent(nullptr),
        uProgressMsg(0),
        uCompleteMsg(0),
        uErrorMsg(0),
        pwszOutputFile(nullptr),
        pwszInputFile(nullptr),
        pwszPassword(nullptr),
        pfValidateCerts(nullptr),
        pfIncludeGroups(nullptr),
        pfDryRun(nullptr),
        pfCreateUsers(nullptr),
        pfContinueOnError(nullptr),
        pdwResultCount(nullptr),
        phrResult(nullptr),
        pUserPasswords(nullptr)
    {}
};

// Progress data sent with WM_USER_PROGRESS
struct PROGRESS_DATA {
    DWORD dwCurrent;
    DWORD dwTotal;
    std::wstring wsStatus;

    PROGRESS_DATA() : dwCurrent(0), dwTotal(0) {}
    PROGRESS_DATA(DWORD c, DWORD t, const std::wstring& s)
        : dwCurrent(c), dwTotal(t), wsStatus(s) {}
};

// Completion data sent with WM_USER_COMPLETE
struct COMPLETE_DATA {
    HRESULT hrResult;
    DWORD dwItemCount;
    std::wstring wsMessage;

    COMPLETE_DATA() : hrResult(S_OK), dwItemCount(0) {}
    COMPLETE_DATA(HRESULT hr, DWORD c, const std::wstring& m)
        : hrResult(hr), dwItemCount(c), wsMessage(m) {}
};

// Error data sent with WM_USER_ERROR
struct ERROR_DATA {
    HRESULT hrResult;
    DWORD dwErrorCode;
    std::wstring wsMessage;

    ERROR_DATA() : hrResult(E_FAIL), dwErrorCode(0) {}
    ERROR_DATA(HRESULT hr, DWORD ec, const std::wstring& m)
        : hrResult(hr), dwErrorCode(ec), wsMessage(m) {}
};

// Worker thread entry points
DWORD WINAPI ExportWorker(LPVOID lpParam); // NOSONAR - Windows API requires LPVOID (void*) for thread functions
DWORD WINAPI ImportWorker(LPVOID lpParam); // NOSONAR - Windows API requires LPVOID (void*) for thread functions
DWORD WINAPI EnumerateWorker(LPVOID lpParam); // NOSONAR - Windows API requires LPVOID (void*) for thread functions
DWORD WINAPI ValidateFileWorker(LPVOID lpParam); // NOSONAR - Windows API requires LPVOID (void*) for thread functions

// Helper to send progress to parent window
void SendProgress(HWND hwnd, UINT uMsg, DWORD dwCurrent, DWORD dwTotal, const std::wstring& wsStatus = L"");

// Helper to send completion to parent window
void SendComplete(HWND hwnd, UINT uMsg, HRESULT hr, DWORD dwCount, const std::wstring& wsMessage);

// Helper to send error to parent window
void SendError(HWND hwnd, UINT uMsg, HRESULT hr, DWORD dwErrorCode, const std::wstring& wsMessage);
