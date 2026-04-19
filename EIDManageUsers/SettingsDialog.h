// SettingsDialog.h - Settings dialog
#pragma once
#include "EIDManageUsers.h"

// Initialize settings dialog
void InitializeSettingsDialog(HWND hwndDlg);

// Save settings from dialog
void SaveSettingsDialog(HWND hwndDlg);

// Add a new EID-related group
void AddEIDGroup(HWND hwndDlg);

// Remove selected EID-related groups
void RemoveEIDGroup(HWND hwndDlg);

// Update groups listbox
void UpdateGroupsList(HWND hwndDlg);
