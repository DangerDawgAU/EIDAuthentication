/*
    EID Authentication - Smart card authentication for Windows
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
 *  CSV Configuration Implementation
 *
 *  JSON and Registry persistence for CSV logging configuration.
 */

#include "CSVConfig.h"
#include "../EIDMigrate/JsonHelper.h"
#include "../EIDMigrate/Utils.h"
#include <fstream>
#include <filesystem>

namespace fs = std::experimental::filesystem;

// ================================================================
// Helper: Convert EID_CSV_CONFIG to JSON string
// ================================================================
std::string EID_CSV_ConfigToJson(const EID_CSV_CONFIG& config)
{
    JsonBuilder builder;

    builder.add("enabled", config.fEnabled != FALSE);

    // Convert wide strings to UTF-8
    std::string logPath = WideToUtf8(config.szLogPath);
    builder.add("logPath", logPath);

    builder.add("maxFileSizeMB", static_cast<int>(config.dwMaxFileSizeMB));
    builder.add("fileCount", static_cast<int>(config.dwFileCount));

    // Store column bitmask as integer
    builder.add("columns", static_cast<int>(config.dwColumns));

    // Store category filter as integer
    builder.add("categoryFilter", static_cast<int>(config.dwCategoryFilter));

    builder.add("verboseEvents", config.fVerboseEvents != FALSE);

    return builder.build();
}

// ================================================================
// Helper: Convert JSON string to EID_CSV_CONFIG
// ================================================================
HRESULT EID_CSV_JsonToConfig(const std::string& json, EID_CSV_CONFIG& config)
{
    JsonParser parser(json);
    auto pRoot = parser.parse();

    if (!pRoot || !pRoot->isObject())
        return E_INVALIDARG;

    const JsonObject& root = pRoot->asObject();

    // Initialize with defaults
    config = EID_CSV_CONFIG();

    // Read enabled flag
    if (root.has("enabled"))
        config.fEnabled = root["enabled"]->asBool() ? TRUE : FALSE;

    // Read log path
    if (root.has("logPath"))
    {
        std::string utf8Path = root["logPath"]->asString();
        std::wstring wpath = Utf8ToWide(utf8Path);
        if (wpath.length() < MAX_PATH)
        {
            wcscpy_s(config.szLogPath, wpath.c_str());
        }
    }

    // Read max file size
    if (root.has("maxFileSizeMB"))
        config.dwMaxFileSizeMB = static_cast<DWORD>(root["maxFileSizeMB"]->asNumber());

    // Read file count
    if (root.has("fileCount"))
        config.dwFileCount = static_cast<DWORD>(root["fileCount"]->asNumber());

    // Read columns bitmask
    if (root.has("columns"))
        config.dwColumns = static_cast<EID_CSV_COLUMN>(static_cast<DWORD>(root["columns"]->asNumber()));

    // Read category filter bitmask
    if (root.has("categoryFilter"))
        config.dwCategoryFilter = static_cast<DWORD>(root["categoryFilter"]->asNumber());

    // Read verbose events flag
    if (root.has("verboseEvents"))
        config.fVerboseEvents = root["verboseEvents"]->asBool() ? TRUE : FALSE;

    return S_OK;
}

// ================================================================
// Load configuration from file
// ================================================================
HRESULT EID_CSV_LoadConfigFromFile(PCWSTR pwszPath, EID_CSV_CONFIG& config)
{
    // Check if file exists
    DWORD dwAttrib = GetFileAttributesW(pwszPath);
    if (dwAttrib == INVALID_FILE_ATTRIBUTES)
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    // Read file content
    std::wstring wpath(pwszPath);
    std::string utf8Path = WideToUtf8(wpath);

    std::ifstream file(utf8Path, std::ios::binary);
    if (!file.is_open())
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);

    std::string content;
    try
    {
        content.assign(std::istreambuf_iterator<char>(file),
                      std::istreambuf_iterator<char>());
    }
    catch (...)
    {
        file.close();
        return E_FAIL;
    }
    file.close();

    // Parse JSON
    return EID_CSV_JsonToConfig(content, config);
}

// ================================================================
// Save configuration to file
// ================================================================
HRESULT EID_CSV_SaveConfigToFile(PCWSTR pwszPath, const EID_CSV_CONFIG& config)
{
    // Ensure directory exists
    std::wstring wpath(pwszPath);
    size_t lastSlash = wpath.find_last_of(L'\\');
    if (lastSlash != std::wstring::npos)
    {
        std::wstring dir = wpath.substr(0, lastSlash);
        // Try to create directory (ignore error if it exists)
        CreateDirectoryW(dir.c_str(), nullptr);
    }

    // Convert to JSON
    std::string json = EID_CSV_ConfigToJson(config);

    // Write to file
    std::string utf8Path = WideToUtf8(wpath);
    std::ofstream file(utf8Path, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);

    try
    {
        file.write(json.c_str(), json.size());
    }
    catch (...)
    {
        file.close();
        return E_FAIL;
    }
    file.close();

    return S_OK;
}

// ================================================================
// Load configuration from registry
// ================================================================
HRESULT EID_CSV_LoadConfigFromRegistry(EID_CSV_CONFIG& config)
{
    HKEY hKey = nullptr;
    LONG err = 0;
    HRESULT hr = S_OK;
    DWORD dwType = 0;
    DWORD dwSize = 0;

    // Initialize with defaults
    config = EID_CSV_CONFIG();

    __try
    {
        err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, EID_CSV_CONFIG_KEY, 0, KEY_READ, &hKey);
        if (err != ERROR_SUCCESS)
        {
            // Key doesn't exist, use defaults
            // Set default log path
            wcscpy_s(config.szLogPath, EID_CSV_DEFAULT_LOG_PATH);
            __leave;
        }

        // Read CSVEnabled
        dwSize = sizeof(DWORD);
        DWORD dwValue = 0;
        err = RegQueryValueExW(hKey, L"CSVEnabled", nullptr, &dwType,
            reinterpret_cast<LPBYTE>(&dwValue), &dwSize);
        if (err == ERROR_SUCCESS && dwType == REG_DWORD)
            config.fEnabled = dwValue ? TRUE : FALSE;

        // Read CSVLogPath
        dwSize = MAX_PATH * sizeof(WCHAR);
        err = RegQueryValueExW(hKey, L"CSVLogPath", nullptr, &dwType,
            reinterpret_cast<LPBYTE>(config.szLogPath), &dwSize);
        if (err != ERROR_SUCCESS || dwType != REG_SZ)
        {
            wcscpy_s(config.szLogPath, EID_CSV_DEFAULT_LOG_PATH);
        }

        // Read CSVMaxFileSize
        dwSize = sizeof(DWORD);
        err = RegQueryValueExW(hKey, L"CSVMaxFileSize", nullptr, &dwType,
            reinterpret_cast<LPBYTE>(&config.dwMaxFileSizeMB), &dwSize);
        if (err != ERROR_SUCCESS || dwType != REG_DWORD)
            config.dwMaxFileSizeMB = 64;

        // Read CSVFileCount
        dwSize = sizeof(DWORD);
        err = RegQueryValueExW(hKey, L"CSVFileCount", nullptr, &dwType,
            reinterpret_cast<LPBYTE>(&config.dwFileCount), &dwSize);
        if (err != ERROR_SUCCESS || dwType != REG_DWORD)
            config.dwFileCount = 5;

        // Read CSVColumns
        dwSize = sizeof(DWORD);
        err = RegQueryValueExW(hKey, L"CSVColumns", nullptr, &dwType,
            reinterpret_cast<LPBYTE>(&config.dwColumns), &dwSize);
        if (err != ERROR_SUCCESS || dwType != REG_DWORD)
            config.dwColumns = EID_CSV_PRESETS::STANDARD;

        // Read CSVCategoryFilter
        dwSize = sizeof(DWORD);
        err = RegQueryValueExW(hKey, L"CSVCategoryFilter", nullptr, &dwType,
            reinterpret_cast<LPBYTE>(&config.dwCategoryFilter), &dwSize);
        if (err != ERROR_SUCCESS || dwType != REG_DWORD)
            config.dwCategoryFilter = 0x0000FFFF;

        // Read CSVVerbose
        dwSize = sizeof(DWORD);
        err = RegQueryValueExW(hKey, L"CSVVerbose", nullptr, &dwType,
            reinterpret_cast<LPBYTE>(&dwValue), &dwSize);
        if (err == ERROR_SUCCESS && dwType == REG_DWORD)
            config.fVerboseEvents = dwValue ? TRUE : FALSE;
    }
    __finally
    {
        if (hKey != nullptr)
            RegCloseKey(hKey);
    }

    return hr;
}

// ================================================================
// Save configuration to registry
// ================================================================
HRESULT EID_CSV_SaveConfigToRegistry(const EID_CSV_CONFIG& config)
{
    HKEY hKey = nullptr;
    LONG err = 0;
    HRESULT hr = S_OK;

    __try
    {
        err = RegCreateKeyExW(HKEY_LOCAL_MACHINE, EID_CSV_CONFIG_KEY, 0, nullptr,
            0, KEY_READ | KEY_WRITE, nullptr, &hKey, nullptr);
        if (err != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(err);
            __leave;
        }

        DWORD dwValue = config.fEnabled ? 1 : 0;
        err = RegSetValueExW(hKey, L"CSVEnabled", 0, REG_DWORD,
            reinterpret_cast<const BYTE*>(&dwValue), sizeof(DWORD));
        if (err != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(err);
            __leave;
        }

        err = RegSetValueExW(hKey, L"CSVLogPath", 0, REG_SZ,
            reinterpret_cast<const BYTE*>(config.szLogPath),
            static_cast<DWORD>((wcslen(config.szLogPath) + 1) * sizeof(WCHAR)));
        if (err != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(err);
            __leave;
        }

        err = RegSetValueExW(hKey, L"CSVMaxFileSize", 0, REG_DWORD,
            reinterpret_cast<const BYTE*>(&config.dwMaxFileSizeMB), sizeof(DWORD));
        if (err != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(err);
            __leave;
        }

        err = RegSetValueExW(hKey, L"CSVFileCount", 0, REG_DWORD,
            reinterpret_cast<const BYTE*>(&config.dwFileCount), sizeof(DWORD));
        if (err != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(err);
            __leave;
        }

        err = RegSetValueExW(hKey, L"CSVColumns", 0, REG_DWORD,
            reinterpret_cast<const BYTE*>(&config.dwColumns), sizeof(DWORD));
        if (err != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(err);
            __leave;
        }

        err = RegSetValueExW(hKey, L"CSVCategoryFilter", 0, REG_DWORD,
            reinterpret_cast<const BYTE*>(&config.dwCategoryFilter), sizeof(DWORD));
        if (err != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(err);
            __leave;
        }

        dwValue = config.fVerboseEvents ? 1 : 0;
        err = RegSetValueExW(hKey, L"CSVVerbose", 0, REG_DWORD,
            reinterpret_cast<const BYTE*>(&dwValue), sizeof(DWORD));
        if (err != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(err);
            __leave;
        }
    }
    __finally
    {
        if (hKey != nullptr)
            RegCloseKey(hKey);
    }

    return hr;
}

// ================================================================
// Load configuration (tries JSON, then registry, then defaults)
// ================================================================
HRESULT EID_CSV_LoadConfig(EID_CSV_CONFIG& config)
{
    // Try JSON file first
    HRESULT hr = EID_CSV_LoadConfigFromFile(EID_CSV_CONFIG_PATH, config);
    if (SUCCEEDED(hr))
        return S_OK;

    // Fall back to registry
    hr = EID_CSV_LoadConfigFromRegistry(config);
    if (SUCCEEDED(hr))
        return S_OK;

    // Use defaults
    config = EID_CSV_CONFIG();
    wcscpy_s(config.szLogPath, EID_CSV_DEFAULT_LOG_PATH);
    return S_FALSE;  // Indicate defaults were used
}

// ================================================================
// Save configuration (saves to both JSON and registry)
// ================================================================
HRESULT EID_CSV_SaveConfig(const EID_CSV_CONFIG& config)
{
    HRESULT hrJson = EID_CSV_SaveConfigToFile(EID_CSV_CONFIG_PATH, config);
    HRESULT hrReg = EID_CSV_SaveConfigToRegistry(config);

    // Return success if at least one succeeded
    return SUCCEEDED(hrJson) ? hrJson : hrReg;
}
