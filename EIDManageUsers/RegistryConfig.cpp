// RegistryConfig.cpp - Settings persistence
#include "RegistryConfig.h"
#include <vector>

// Registry value names
static const LPCWSTR REG_EID_GROUPS = L"EIDGroups";
static const LPCWSTR REG_FILTER_MODE = L"FilterMode";
static const LPCWSTR REG_CONFIRM_ACTIONS = L"ConfirmActions";
static const LPCWSTR REG_SHOW_WARNINGS = L"ShowWarnings";

// Open registry key
HKEY OpenRegKey(BOOL fWrite)
{
    HKEY hKey = nullptr;
    DWORD dwDisposition;

    LONG lResult = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
        EIDMANAGE_REG_KEY,
        0,
        nullptr,
        REG_OPTION_NON_VOLATILE,
        fWrite ? KEY_WRITE : KEY_READ,
        nullptr,
        &hKey,
        &dwDisposition);

    return (lResult == ERROR_SUCCESS) ? hKey : nullptr;
}

// Close registry key
void CloseRegKey(HKEY hKey)
{
    if (hKey)
        RegCloseKey(hKey);
}

// Load all settings
void LoadSettings()
{
    // Load EID groups
    LoadEIDGroups(g_appState.EIDGroups);

    // Load filter mode
    g_appState.currentFilter = LoadFilterMode();

    // Load confirm actions
    g_appState.fConfirmActions = LoadConfirmActions();

    // Load show warnings
    HKEY hKey = OpenRegKey(FALSE);
    if (hKey)
    {
        DWORD dwValue = 0;
        DWORD dwSize = sizeof(DWORD);
        DWORD dwType = REG_DWORD;

        if (RegQueryValueExW(hKey, REG_SHOW_WARNINGS, nullptr,
            &dwType, reinterpret_cast<LPBYTE>(&dwValue), &dwSize) == ERROR_SUCCESS)
        {
            g_appState.fShowWarnings = (dwValue != 0);
        }

        CloseRegKey(hKey);
    }
}

// Save all settings
void SaveSettings()
{
    SaveEIDGroups(g_appState.EIDGroups);
    SaveFilterMode(g_appState.currentFilter);
    SaveConfirmActions(g_appState.fConfirmActions);

    HKEY hKey = OpenRegKey(TRUE);
    if (hKey)
    {
        DWORD dwValue = g_appState.fShowWarnings ? 1 : 0;
        RegSetValueExW(hKey, REG_SHOW_WARNINGS, 0, REG_DWORD,
            reinterpret_cast<const BYTE*>(&dwValue), sizeof(DWORD));
        CloseRegKey(hKey);
    }
}

// Load EID groups
void LoadEIDGroups(_Inout_ std::vector<std::wstring>& groups)
{
    // Keep defaults if registry read fails
    HKEY hKey = OpenRegKey(FALSE);
    if (!hKey)
        return;

    DWORD dwType = REG_MULTI_SZ;
    DWORD dwSize = 0;

    // First call to get size
    if (RegQueryValueExW(hKey, REG_EID_GROUPS, nullptr,
        &dwType, nullptr, &dwSize) != ERROR_SUCCESS || dwSize == 0)
    {
        CloseRegKey(hKey);
        return;
    }

    // Allocate buffer and read value
    std::vector<WCHAR> buffer(dwSize / sizeof(WCHAR));
    if (RegQueryValueExW(hKey, REG_EID_GROUPS, nullptr,
        &dwType, reinterpret_cast<LPBYTE>(buffer.data()), &dwSize) == ERROR_SUCCESS)
    {
        // Parse REG_MULTI_SZ
        groups.clear();
        LPCWSTR pwsz = buffer.data();
        while (*pwsz)
        {
            groups.push_back(pwsz);
            pwsz += wcslen(pwsz) + 1;
        }
    }

    CloseRegKey(hKey);
}

// Save EID groups
void SaveEIDGroups(_In_ const std::vector<std::wstring>& groups)
{
    HKEY hKey = OpenRegKey(TRUE);
    if (!hKey)
        return;

    if (groups.empty())
    {
        // Delete value if no groups
        RegDeleteValueW(hKey, REG_EID_GROUPS);
    }
    else
    {
        // Build REG_MULTI_SZ string
        DWORD dwTotalSize = 0;
        for (const auto& group : groups)
        {
            dwTotalSize += static_cast<DWORD>(group.length() + 1);
        }
        dwTotalSize += 1;  // Double null terminator

        std::vector<WCHAR> buffer(dwTotalSize);
        WCHAR* pwsz = buffer.data();
        for (const auto& group : groups)
        {
            wcscpy_s(pwsz, buffer.size() - (pwsz - buffer.data()), group.c_str());
            pwsz += group.length() + 1;
        }
        *pwsz = L'\0';  // Double null terminator

        RegSetValueExW(hKey, REG_EID_GROUPS, 0, REG_MULTI_SZ,
            reinterpret_cast<const BYTE*>(buffer.data()),
            static_cast<DWORD>(buffer.size() * sizeof(WCHAR)));
    }

    CloseRegKey(hKey);
}

// Load filter mode
FilterMode LoadFilterMode()
{
    HKEY hKey = OpenRegKey(FALSE);
    if (!hKey)
        return FILTER_EID_ONLY;

    DWORD dwValue = FILTER_EID_ONLY;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;

    if (RegQueryValueExW(hKey, REG_FILTER_MODE, nullptr,
        &dwType, reinterpret_cast<LPBYTE>(&dwValue), &dwSize) == ERROR_SUCCESS)
    {
        if (dwValue <= FILTER_ALL_USERS)
            dwValue = static_cast<DWORD>(FILTER_EID_ONLY);
    }

    CloseRegKey(hKey);
    return static_cast<FilterMode>(dwValue);
}

// Save filter mode
void SaveFilterMode(FilterMode filter)
{
    HKEY hKey = OpenRegKey(TRUE);
    if (!hKey)
        return;

    DWORD dwValue = static_cast<DWORD>(filter);
    RegSetValueExW(hKey, REG_FILTER_MODE, 0, REG_DWORD,
        reinterpret_cast<const BYTE*>(&dwValue), sizeof(DWORD));
    CloseRegKey(hKey);
}

// Load confirm actions
BOOL LoadConfirmActions()
{
    HKEY hKey = OpenRegKey(FALSE);
    if (!hKey)
        return TRUE;

    DWORD dwValue = TRUE;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;

    if (RegQueryValueExW(hKey, REG_CONFIRM_ACTIONS, nullptr,
        &dwType, reinterpret_cast<LPBYTE>(&dwValue), &dwSize) == ERROR_SUCCESS)
    {
        CloseRegKey(hKey);
        return (dwValue != 0);
    }

    CloseRegKey(hKey);
    return TRUE;
}

// Save confirm actions
void SaveConfirmActions(BOOL fConfirm)
{
    HKEY hKey = OpenRegKey(TRUE);
    if (!hKey)
        return;

    DWORD dwValue = fConfirm ? 1 : 0;
    RegSetValueExW(hKey, REG_CONFIRM_ACTIONS, 0, REG_DWORD,
        reinterpret_cast<const BYTE*>(&dwValue), sizeof(DWORD));
    CloseRegKey(hKey);
}
