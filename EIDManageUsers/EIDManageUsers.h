// EIDManageUsers.h - Main header for EID User Management Tool
// Copyright (c) 2026

#pragma once

#include <Windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <windowsx.h>
#include <string>
#include <vector>
#include <memory>
#include <lm.h>

// Resource IDs
#include "resource.h"

// EIDCardLibrary includes
#include "../EIDCardLibrary/EIDCardLibrary.h"
#include "../EIDCardLibrary/Package.h"
#include "../EIDCardLibrary/Tracing.h"

// EIDMigrate includes
#include "../EIDMigrate/EIDMigrate.h"
#include "../EIDMigrate/LsaClient.h"

// Version information
#define EIDMANAGE_APP_VERSION       L"1.0.0.1"
#define EIDMANAGE_APP_COPYRIGHT     L"Copyright (C) 2026"
#define EIDMANAGE_APP_NAME          L"EID User Management"

// Registry settings path
#define EIDMANAGE_REG_KEY           L"SOFTWARE\\EIDAuthenticate\\EIDManageUsers"

// Maximum users displayable in list
#define EIDMANAGE_MAX_USERS         1000

// Filter modes
enum FilterMode
{
    FILTER_EID_ONLY = 0,   // Show only users with EID credentials
    FILTER_ALL_USERS = 1   // Show all local users
};

// User information structure
struct UserInfo
{
    DWORD dwRid;
    std::wstring wsUsername;
    std::wstring wsSid;
    std::wstring wsLastLogin;
    BOOL fHasEIDCredential;
    EID_PRIVATE_DATA_TYPE EncryptionType;
    UCHAR CertificateHash[32];
    std::vector<std::wstring> wsGroups;

    UserInfo() :
        dwRid(0),
        fHasEIDCredential(FALSE),
        EncryptionType(EID_PRIVATE_DATA_TYPE::eidpdtClearText)
    {
        SecureZeroMemory(CertificateHash, sizeof(CertificateHash));
    }
};

// Management operation flags
enum ManagementOperation
{
    OP_REMOVE_CREDENTIAL       = 0x0001,
    OP_REMOVE_CERTIFICATES     = 0x0002,
    OP_REMOVE_FROM_GROUPS      = 0x0004,
    OP_DELETE_ACCOUNT          = 0x0008
};

// Global application state
struct USER_MANAGE_APP_STATE
{
    HWND hwndMain;
    FilterMode currentFilter;
    std::vector<UserInfo> users;
    std::vector<std::wstring> EIDGroups;
    BOOL fConfirmActions;
    BOOL fShowWarnings;

    USER_MANAGE_APP_STATE() :
        hwndMain(nullptr),
        currentFilter(FILTER_EID_ONLY),
        fConfirmActions(TRUE),
        fShowWarnings(TRUE)
    {
        // Default EID-related groups
        EIDGroups.push_back(L"Smart Card Users");
        EIDGroups.push_back(L"EID Users");
    }
};

// Global application instance
extern USER_MANAGE_APP_STATE g_appState;
extern HINSTANCE g_hinst;

// Forward declarations of dialog procedures
INT_PTR CALLBACK WndProc_Main(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK WndProc_Settings(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK WndProc_Manage(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK WndProc_Details(HWND, UINT, WPARAM, LPARAM);

// Utility functions
BOOL IsUserAdmin();
std::wstring LoadStringResource(UINT uID);
void SetWindowIcon(HWND hwnd, int iconId = 0);
HRESULT EnsureElevated();
std::wstring FormatFileTime(const FILETIME& ft);
std::wstring GetCurrentUserSid();

// MainPage functions
void InitializeMainPage(HWND hwndDlg);
HRESULT RefreshUserList(HWND hwndDlg);
void ShowDetailsDialog(HWND hwndParent);
void ShowManageDialog(HWND hwndParent);
void UpdateFilterControls(HWND hwndDlg, FilterMode filter);
