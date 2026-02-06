#pragma once

#include <Windows.h>
#include <NTSecAPI.h>
#include <string>
#include <vector>
#include <sstream>

namespace EID {
    // LoadString wrapper - loads string from resource
    std::wstring LoadStringW(HINSTANCE hInst, UINT uID);

    // Window text helpers
    std::wstring GetWindowTextW(HWND hWnd);
    BOOL SetWindowTextW(HWND hWnd, const std::wstring& text);

    // Format string helper (replaces _stprintf_s)
    std::wstring Format(const wchar_t* format, ...);

    // Container name building (for smart cards)
    std::wstring BuildContainerNameFromReader(const std::wstring& readerName);

    // Safe conversion with bounds checking
    std::wstring SafeConvert(const char* src, size_t maxLen);
    std::wstring SafeConvert(const wchar_t* src, size_t maxLen);

    // Registry helpers
    std::wstring RegQueryStringW(HKEY hKey, const std::wstring& valueName);

    // UNICODE_STRING conversion helper
    void ConvertToUnicodeString(const std::wstring& src, PUNICODE_STRING dst, PVOID buffer, ULONG bufferSize);

    // Password/sensitive data handling - secure zero
    void SecureZeroString(std::wstring& str);

    // Helper for converting between char arrays and wstring
    std::wstring ConvertCharToWString(const char* src);
    std::string ConvertWStringToChar(const std::wstring& src);
}
