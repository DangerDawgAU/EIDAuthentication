// RegistryConfig.h - Settings persistence to registry
#pragma once
#include "EIDManageUsers.h"

// Load all settings from registry
void LoadSettings();

// Save all settings to registry
void SaveSettings();

// Load EID-related groups list
void LoadEIDGroups(_Inout_ std::vector<std::wstring>& groups);

// Save EID-related groups list
void SaveEIDGroups(_In_ const std::vector<std::wstring>& groups);

// Load filter setting
FilterMode LoadFilterMode();

// Save filter setting
void SaveFilterMode(FilterMode filter);

// Load confirm actions setting
BOOL LoadConfirmActions();

// Save confirm actions setting
void SaveConfirmActions(BOOL fConfirm);

// Helper: Registry key open/close
HKEY OpenRegKey(BOOL fWrite = FALSE);
void CloseRegKey(HKEY hKey);
