#include "StringConversion.h"
#include <stdarg.h>
#include <algorithm>
#include <cstring>

namespace EID {

    std::wstring LoadStringW(HINSTANCE hInst, UINT uID)
    {
        wchar_t buffer[512] = { 0 };
        int result = ::LoadStringW(hInst, uID, buffer, ARRAYSIZE(buffer) - 1);
        if (result > 0) {
            return std::wstring(buffer);
        }
        return std::wstring();
    }

    std::wstring GetWindowTextW(HWND hWnd)
    {
        if (!hWnd || !IsWindow(hWnd)) {
            return std::wstring();
        }

        int len = ::GetWindowTextLengthW(hWnd);
        if (len <= 0) {
            return std::wstring();
        }

        std::wstring result(len + 1, L'\0');
        ::GetWindowTextW(hWnd, &result[0], len + 1);
        result.resize(len);
        return result;
    }

    BOOL SetWindowTextW(HWND hWnd, const std::wstring& text)
    {
        if (!hWnd || !IsWindow(hWnd)) {
            return FALSE;
        }
        return ::SetWindowTextW(hWnd, text.c_str());
    }

    std::wstring Format(const wchar_t* format, ...)
    {
        if (!format) {
            return std::wstring();
        }

        va_list args;
        va_start(args, format);

        // First pass: get required size
        int size = _vscwprintf(format, args) + 1;
        if (size <= 1) {
            va_end(args);
            return std::wstring();
        }

        // Allocate and format
        std::vector<wchar_t> buffer(size);
        vswprintf_s(buffer.data(), buffer.size(), format, args);
        va_end(args);

        return std::wstring(buffer.data());
    }

    std::wstring BuildContainerNameFromReader(const std::wstring& readerName)
    {
        return L"\\\\.\\" + readerName + L"\\";
    }

    std::wstring SafeConvert(const char* src, size_t maxLen)
    {
        if (!src || maxLen == 0) {
            return std::wstring();
        }

        // Find actual length (null-terminated or max length)
        size_t len = strnlen(src, maxLen);

        // Convert from ANSI to Unicode
        int wideCharCount = MultiByteToWideChar(CP_ACP, 0, src, (int)len, NULL, 0);
        if (wideCharCount <= 0) {
            return std::wstring();
        }

        std::wstring result(wideCharCount, L'\0');
        MultiByteToWideChar(CP_ACP, 0, src, (int)len, &result[0], wideCharCount);
        return result;
    }

    std::wstring SafeConvert(const wchar_t* src, size_t maxLen)
    {
        if (!src || maxLen == 0) {
            return std::wstring();
        }

        size_t len = wcsnlen(src, maxLen);
        return std::wstring(src, len);
    }

    std::wstring RegQueryStringW(HKEY hKey, const std::wstring& valueName)
    {
        if (!hKey) {
            return std::wstring();
        }

        wchar_t buffer[1024] = { 0 };
        DWORD bufferSize = sizeof(buffer);
        DWORD dataType = 0;

        LONG result = RegQueryValueExW(hKey, valueName.c_str(), NULL, &dataType,
                                       (LPBYTE)buffer, &bufferSize);

        if (result != ERROR_SUCCESS || dataType != REG_SZ) {
            return std::wstring();
        }

        return std::wstring(buffer);
    }

    void ConvertToUnicodeString(const std::wstring& src, PUNICODE_STRING dst, PVOID buffer, ULONG bufferSize)
    {
        if (!dst || !buffer) {
            return;
        }

        size_t charCount = src.length() + 1;
        if (charCount * sizeof(wchar_t) > bufferSize) {
            // Buffer too small
            dst->Length = 0;
            dst->MaximumLength = 0;
            dst->Buffer = NULL;
            return;
        }

        wchar_t* bufPtr = (wchar_t*)buffer;
        errno_t err = wcscpy_s(bufPtr, bufferSize / sizeof(wchar_t), src.c_str());
        if (err == 0) {
            dst->Buffer = bufPtr;
            dst->Length = (USHORT)(src.length() * sizeof(wchar_t));
            dst->MaximumLength = (USHORT)bufferSize;
        } else {
            dst->Length = 0;
            dst->MaximumLength = 0;
            dst->Buffer = NULL;
        }
    }

    void SecureZeroString(std::wstring& str)
    {
        // Overwrite string memory with zeros before deallocation
        if (!str.empty()) {
            SecureZeroMemory(&str[0], str.length() * sizeof(wchar_t));
        }
        str.clear();
        str.shrink_to_fit();
    }

    std::wstring ConvertCharToWString(const char* src)
    {
        if (!src) {
            return std::wstring();
        }

        size_t len = strlen(src);
        return SafeConvert(src, len);
    }

    std::string ConvertWStringToChar(const std::wstring& src)
    {
        if (src.empty()) {
            return std::string();
        }

        int charCount = WideCharToMultiByte(CP_ACP, 0, src.c_str(), (int)src.length(), NULL, 0, NULL, NULL);
        if (charCount <= 0) {
            return std::string();
        }

        std::string result(charCount, '\0');
        WideCharToMultiByte(CP_ACP, 0, src.c_str(), (int)src.length(), &result[0], charCount, NULL, NULL);
        return result;
    }
}
