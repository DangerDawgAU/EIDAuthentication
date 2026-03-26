#pragma once

// File: EIDMigrate/Utils.h
// General utility functions

#include <Windows.h>
#include <string>
#include <vector>
#include <functional>
#include "SecureMemory.h"

// String conversion utilities
std::string WideToUtf8(_In_ const std::wstring& ws);
std::wstring Utf8ToWide(_In_ const std::string& s);
std::wstring AnsiToWide(_In_ const std::string& s);

// SID conversion utilities
std::wstring SidToString(_In_ PSID pSid);
BOOL StringToSid(_In_ const std::wstring& wsSid, _Out_ PSID* ppSid);
DWORD GetRidFromSid(_In_ PSID pSid);

// Computer name
std::wstring GetComputerName();
std::wstring GetUserName();

// File path utilities
std::wstring GetFileExtension(_In_ const std::wstring& wsPath);
std::wstring GetFileName(_In_ const std::wstring& wsPath);
std::wstring GetDirectoryPath(_In_ const std::wstring& wsPath);
bool FileExists(_In_ const std::wstring& wsPath);
bool IsFileReadable(_In_ const std::wstring& wsPath);
bool IsFileWritable(_In_ const std::wstring& wsPath);

// Time formatting
std::wstring FormatTimestamp(_In_ const FILETIME& ft);
std::wstring FormatCurrentTimestamp();

// Prompt user for input
BOOL PromptYesNo(_In_ PCWSTR pwszPrompt, _In_ BOOL fDefaultYes = TRUE);
std::wstring PromptForString(_In_ PCWSTR pwszPrompt);
SecureWString PromptForPassphrase(_In_ PCWSTR pwszPrompt, _In_ BOOL fConfirm = TRUE);

// Center window on screen
void CenterWindow(_In_ HWND hwnd);

// Version info
std::wstring GetFileVersion(_In_ PCWSTR pwszFilePath);

// Helper to run function with privilege elevation
BOOL RunWithPrivilege(
    _In_ PCWSTR pwszPrivilege,
    _In_ std::function<BOOL()> func);

// Check if running as administrator
BOOL IsRunningAsAdmin();

// Enable a privilege for the current process
BOOL EnablePrivilege(_In_ PCWSTR pwszPrivilege);

// Error formatting
std::wstring FormatErrorMessage(_In_ DWORD dwError);
std::wstring FormatHResult(_In_ HRESULT hr);

// Random password generation for user creation
std::wstring GenerateRandomPassword(_In_ DWORD dwLength = 32);
