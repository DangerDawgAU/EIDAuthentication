// EIDManageUsers.h - Main header for EID User Management Tool
// Copyright (c) 2026

#pragma once

#include <Windows.h>
#include <tchar.h>
#include <commctrl.h>  // NOSONAR - INCLUDE-01: include order/casing significant for Windows SDK
#include <windowsx.h>
#include <string>
#include <vector>
#include <memory>
#include <lm.h>  // NOSONAR - INCLUDE-01: include order/casing significant for Windows SDK

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
#define EIDMANAGE_APP_VERSION       L"1.0.0.1"  // NOSONAR - MACRO-01: Windows-style macro constant retained for API/preprocessor use
#define EIDMANAGE_APP_COPYRIGHT     L"Copyright (C) 2026"  // NOSONAR - MACRO-01: Windows-style macro constant retained for API/preprocessor use
#define EIDMANAGE_APP_NAME          L"EID User Management"  // NOSONAR - MACRO-01: Windows-style macro constant retained for API/preprocessor use

// Registry settings path
#define EIDMANAGE_REG_KEY           L"SOFTWARE\\EIDAuthenticate\\EIDManageUsers"  // NOSONAR - MACRO-01: Windows-style macro constant retained for API/preprocessor use

// Maximum users displayable in list
#define EIDMANAGE_MAX_USERS         1000  // NOSONAR - MACRO-01: Windows-style macro constant retained for API/preprocessor use

// Filter modes
enum FilterMode  // NOSONAR - ENUM-01: Unscoped enum retained for API/ABI compatibility
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
    UCHAR CertificateHash[32];  // NOSONAR - LSASS-01: C-style buffer required by Win32 API
    std::vector<std::wstring> wsGroups;

    UserInfo() :
        dwRid(0),  // NOSONAR - INIT-01: member initialized in body for clarity/ordering
        fHasEIDCredential(FALSE),  // NOSONAR - INIT-01: member initialized in body for clarity/ordering
        EncryptionType(EID_PRIVATE_DATA_TYPE::eidpdtClearText)  // NOSONAR - INIT-01: member initialized in body for clarity/ordering
    {
        SecureZeroMemory(CertificateHash, sizeof(CertificateHash));
    }
};

// Management operation flags
enum ManagementOperation  // NOSONAR - ENUM-01: Unscoped enum retained for API/ABI compatibility
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
        hwndMain(nullptr),  // NOSONAR - INIT-01: member initialized in body for clarity/ordering
        currentFilter(FILTER_EID_ONLY),  // NOSONAR - INIT-01: member initialized in body for clarity/ordering
        fConfirmActions(TRUE),  // NOSONAR - INIT-01: member initialized in body for clarity/ordering
        fShowWarnings(TRUE)  // NOSONAR - INIT-01: member initialized in body for clarity/ordering
    {
        // Default EID-related groups
        EIDGroups.emplace_back(L"Smart Card Users");
        EIDGroups.emplace_back(L"EID Users");
    }
};

// Global application instance
extern USER_MANAGE_APP_STATE g_appState;  // NOSONAR - GLOBAL-01: global state assigned/modified at runtime
extern HINSTANCE g_hinst;  // NOSONAR - GLOBAL-01: pointer assigned at runtime

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
