// EIDMigrateUI.cpp - Main entry point for EID Credential Migration Tool UI
// Copyright (c) 2026

#include "EIDMigrateUI.h"
#include "WorkerThread.h"
#include "Page14_GroupSelect.h"
#include "../EIDMigrate/EIDMigrate.h"

// Link with Common Controls v6 for Aero Wizard support
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Global application state
WIZARD_DATA g_wizardData;
HINSTANCE g_hinst = nullptr;
HWND g_hwndWizard = nullptr;
WizardFlow g_currentFlow = FLOW_NONE;  // Tracks current wizard flow (export/import)

// Provide minimal APP_STATE for AuditLogging module
APP_STATE g_AppState;

// Hidden window class for owning the PropertySheet
static const wchar_t szHiddenWindowClass[] = L"EIDMigrateUI_HiddenWindow";

// Window procedure for hidden owner window
LRESULT CALLBACK HiddenWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Create hidden owner window
HWND CreateHiddenOwnerWindow(HINSTANCE hInstance)
{
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = HiddenWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = szHiddenWindowClass;

    if (!RegisterClassW(&wc)) {
        // Class might already be registered, ignore error
    }

    HWND hwnd = CreateWindowExW(0, szHiddenWindowClass, L"EIDMigrateUI_Owner",
        0, 0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);

    return hwnd;
}

// Load string resource
std::wstring LoadStringResource(UINT uID) {
    WCHAR szBuffer[512];
    if (LoadString(g_hinst, uID, szBuffer, ARRAYSIZE(szBuffer))) {
        return std::wstring(szBuffer);
    }
    return std::wstring();
}

// Set window icon (uses imageres.dll shield icon by default)
void SetWindowIcon(HWND hwnd, int iconId) {
    HMODULE hDll = LoadLibraryW(L"imageres.dll");
    if (hDll) {
        HICON hIcon = (HICON)LoadImage(hDll, MAKEINTRESOURCE(iconId ? iconId : 58),
            IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
            GetSystemMetrics(SM_CYSMICON), LR_SHARED);
        if (hIcon) {
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        }
        FreeLibrary(hDll);
    }
}

// Check if user is Administrator
BOOL IsUserAdmin() {
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup = nullptr;

    if (!AllocateAndInitializeSid(&NtAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0, &AdministratorsGroup)) {
        return FALSE;
    }

    BOOL bIsAdmin = FALSE;
    if (!CheckTokenMembership(nullptr, AdministratorsGroup, &bIsAdmin)) {
        bIsAdmin = FALSE;
    }

    FreeSid(AdministratorsGroup);
    return bIsAdmin;
}

// Get credential count from LSA
HRESULT GetCredentialCount(DWORD* pdwCount) {
    if (!pdwCount) return E_POINTER;

    std::vector<CredentialInfo> credentials;
    HRESULT hr = EnumerateLsaCredentials(credentials);
    if (SUCCEEDED(hr)) {
        *pdwCount = static_cast<DWORD>(credentials.size());
    }
    return hr;
}

// Format certificate hash for display (show first 16 chars + "...")
void FormatCredentialHash(const std::wstring& fullHash, std::wstring& truncated) {
    if (fullHash.length() > 16) {
        truncated = fullHash.substr(0, 16) + L"...";
    } else {
        truncated = fullHash;
    }
}

// Show error message and exit if condition fails
BOOL CheckPrerequisites() {
    // Note: Manifest handles elevation (requireAdministrator)
    // Note: We don't check IsEIDPackageAvailable() because this tool uses direct LSA access

    // Double-check admin (should always pass due to manifest)
    if (!IsUserAdmin()) {
        MessageBoxW(nullptr, L"This tool requires Administrator privileges.",
            EIDMIGRATEUI_APP_NAME, MB_ICONERROR);
        return FALSE;
    }

    return TRUE;
}

// Entry point
int APIENTRY wWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPWSTR lpCmdLine,
                     int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    // Initialize common controls
    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&iccex);

    g_hinst = hInstance;

    // CRITICAL: Create hidden owner window BEFORE any MessageBox or PropertySheet
    // This window stays alive and owns the wizard, preventing Windows from
    // suspending the process when creating windows.
    HWND hwndOwner = CreateHiddenOwnerWindow(hInstance);
    if (!hwndOwner) {
        MessageBoxW(nullptr, L"Failed to create owner window", EIDMIGRATEUI_APP_NAME, MB_ICONERROR);
        return 1;
    }

    // Use the owner window for MessageBox
    if (!CheckPrerequisites()) {
        DestroyWindow(hwndOwner);
        return 1;
    }

    // Initialize COM for file dialogs
    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hrInit)) {
        MessageBoxW(hwndOwner, L"Failed to initialize COM", EIDMIGRATEUI_APP_NAME, MB_ICONERROR);
        DestroyWindow(hwndOwner);
        return 1;
    }

    // Create property sheet pages
    HPROPSHEETPAGE ahpsp[EIDMIGRATE_PAGE_COUNT];
    int pageCount = 0;

    PROPSHEETPAGE psp = {0};
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.hInstance = hInstance;
    psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.lParam = (LPARAM)&g_wizardData;

    // Page 01: Welcome
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE_WELCOME);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_01_WELCOME);
    psp.pfnDlgProc = WndProc_01_Welcome;
    ahpsp[pageCount++] = CreatePropertySheetPage(&psp);

    // Page 02: Export Select
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE_EXPORT_SELECT);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_02_EXPORT_SELECT);
    psp.pfnDlgProc = WndProc_02_ExportSelect;
    ahpsp[pageCount++] = CreatePropertySheetPage(&psp);

    // Page 03: Export Groups
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE_GROUP_SELECT);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_14_GROUP_SELECT);
    psp.pfnDlgProc = WndProc_14_GroupSelect;
    ahpsp[pageCount++] = CreatePropertySheetPage(&psp);

    // Page 04: Export Confirm
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE_EXPORT_CONFIRM);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_03_EXPORT_CONFIRM);
    psp.pfnDlgProc = WndProc_03_ExportConfirm;
    ahpsp[pageCount++] = CreatePropertySheetPage(&psp);

    // Page 05: Export Progress
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE_EXPORT_SELECT);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_04_EXPORT_PROGRESS);
    psp.pfnDlgProc = WndProc_04_ExportProgress;
    ahpsp[pageCount++] = CreatePropertySheetPage(&psp);

    // Page 06: Export Complete
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE_EXPORT_COMPLETE);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_05_EXPORT_COMPLETE);
    psp.pfnDlgProc = WndProc_05_ExportComplete;
    ahpsp[pageCount++] = CreatePropertySheetPage(&psp);

    // Page 07: Import Select
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE_IMPORT_SELECT);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_06_IMPORT_SELECT);
    psp.pfnDlgProc = WndProc_06_ImportSelect;
    ahpsp[pageCount++] = CreatePropertySheetPage(&psp);

    // Page 08: Import Options
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE_IMPORT_OPTIONS);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_07_IMPORT_OPTIONS);
    psp.pfnDlgProc = WndProc_07_ImportOptions;
    ahpsp[pageCount++] = CreatePropertySheetPage(&psp);

    // Page 09: Import Groups
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE_GROUP_SELECT);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_14_GROUP_SELECT);
    psp.pfnDlgProc = WndProc_14_GroupSelect;
    ahpsp[pageCount++] = CreatePropertySheetPage(&psp);

    // Page 10: Import Preview
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE_IMPORT_PREVIEW);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_08_IMPORT_PREVIEW);
    psp.pfnDlgProc = WndProc_08_ImportPreview;
    ahpsp[pageCount++] = CreatePropertySheetPage(&psp);

    // Page 11: Import Progress
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE_IMPORT_PREVIEW);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_09_IMPORT_PROGRESS);
    psp.pfnDlgProc = WndProc_09_ImportProgress;
    ahpsp[pageCount++] = CreatePropertySheetPage(&psp);

    // Page 12: Import Complete
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE_IMPORT_COMPLETE);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_10_IMPORT_COMPLETE);
    psp.pfnDlgProc = WndProc_10_ImportComplete;
    ahpsp[pageCount++] = CreatePropertySheetPage(&psp);

    // Page 13: List Credentials
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE_LIST);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_11_LIST_CREDENTIALS);
    psp.pfnDlgProc = WndProc_11_ListCredentials;
    ahpsp[pageCount++] = CreatePropertySheetPage(&psp);

    // Page 14: Validate File
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TITLE_VALIDATE);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_12_VALIDATE_FILE);
    psp.pfnDlgProc = WndProc_12_ValidateFile;
    ahpsp[pageCount++] = CreatePropertySheetPage(&psp);

    // Configure property sheet header
    PROPSHEETHEADER psh = {0};
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_WIZARD | PSH_AEROWIZARD | PSH_USEHICON;
    psh.hInstance = hInstance;
    psh.hwndParent = hwndOwner;  // Use hidden owner window
    psh.phpage = ahpsp;
    psh.nPages = pageCount;
    psh.nStartPage = 0;
    psh.pszCaption = MAKEINTRESOURCE(IDS_CAPTION);
    psh.pszbmWatermark = nullptr;
    psh.pszbmHeader = nullptr;

    // Load icon
    HMODULE hDll = LoadLibraryW(L"imageres.dll");
    if (hDll) {
        psh.hIcon = LoadIcon(hDll, MAKEINTRESOURCE(58));
        FreeLibrary(hDll);
    }

    // Display wizard
    INT_PTR rc = PropertySheet(&psh);

    // Cleanup
    CoUninitialize();
    DestroyWindow(hwndOwner);

    if (rc == -1) {
        MessageBoxW(nullptr, L"PropertySheet failed to create", EIDMIGRATEUI_APP_NAME, MB_ICONERROR);
        return 1;
    } else if (rc == 0) {
        return 0;
    }

    return 0;
}
