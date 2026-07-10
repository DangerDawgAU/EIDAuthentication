# Diagnostics Trace Logging Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an EIDLogManager option that captures the credential provider's free-text diagnostic ETW traces (including the `EVID:` tile traces) to a dedicated `diagnostics.log`.

**Architecture:** Persist two new settings (`DiagnosticsEnabled`, `DiagnosticsLevel`) in the existing config store. The always-running `EIDTraceConsumer` service, when diagnostics is enabled, creates its real-time ETW session and writes the provider's free-text traces to `diagnostics.log`; it no longer writes `events.csv` (owned by the in-process LSA logger), which removes a latent CSV schema-collision. EIDLogManager gets a checkbox + Info/Verbose level and restarts the consumer on Apply.

**Tech Stack:** C++17, Win32 (ETW/TDH, registry, dialog resources), MSBuild via `build.ps1`.

## Global Constraints

- Build ONLY via `.\build.ps1 Release x64` (never raw msbuild on one project). Verified in memory `build-with-build-ps1`.
- Platform is **x64 only**.
- This codebase has **no automated test framework**. Each task's gate is: `build.ps1` succeeds (0 failed). The final task is manual on-hardware verification using the ETW-trace method.
- LSASS-safety convention: stack/C-style buffers, no exceptions across the ETW callback. Follow the surrounding file's existing style and NOSONAR comments.
- Registry config key: `HKLM\SOFTWARE\EIDAuthentication\LogManager`. JSON config: `C:\ProgramData\EIDAuthentication\logging.json`.
- Diagnostics level semantics: capture an event when `EventDescriptor.Level <= dwDiagnosticsLevel`, where `WINEVENT_LEVEL_INFO = 4`, `WINEVENT_LEVEL_VERBOSE = 5`.

---

### Task 1: Config model + persistence for diagnostics settings

**Files:**
- Modify: `EIDCardLibrary/CSVConfig.h` (struct `EID_CSV_CONFIG`, ~line 185-206)
- Modify: `EIDCardLibrary/CSVConfig.cpp` (JSON at ~36-112, registry at ~190-345)

**Interfaces:**
- Produces: `EID_CSV_CONFIG::fDiagnosticsEnabled` (BOOL), `EID_CSV_CONFIG::dwDiagnosticsLevel` (DWORD). Consumed by Tasks 2 and 3.
- Registry values `DiagnosticsEnabled` (REG_DWORD), `DiagnosticsLevel` (REG_DWORD) under the LogManager key.
- JSON keys `diagnosticsEnabled` (bool), `diagnosticsLevel` (int).

- [ ] **Step 1: Add the two fields + defaults to the struct**

In `CSVConfig.h`, inside `struct EID_CSV_CONFIG`, after `BOOL fVerboseEvents;`:

```cpp
    BOOL                  fDiagnosticsEnabled; // Capture free-text provider diagnostic traces
    DWORD                 dwDiagnosticsLevel;  // WINEVENT level ceiling for diagnostics (4=INFO, 5=VERBOSE)
```

In the same struct's constructor, after `fVerboseEvents = FALSE;`:

```cpp
        fDiagnosticsEnabled = FALSE;
        dwDiagnosticsLevel = 4; // WINEVENT_LEVEL_INFO
```

- [ ] **Step 2: Add JSON serialization**

In `CSVConfig.cpp` `EID_CSV_ConfigToJson`, after `builder.add("verboseEvents", config.fVerboseEvents != FALSE);`:

```cpp
    builder.add("diagnosticsEnabled", config.fDiagnosticsEnabled != FALSE);
    builder.add("diagnosticsLevel", static_cast<int>(config.dwDiagnosticsLevel));
```

In `EID_CSV_JsonToConfig`, after the `verboseEvents` block:

```cpp
    // Read diagnostics flags
    if (root.has("diagnosticsEnabled"))
        config.fDiagnosticsEnabled = root["diagnosticsEnabled"]->asBool() ? TRUE : FALSE;
    if (root.has("diagnosticsLevel"))
        config.dwDiagnosticsLevel = static_cast<DWORD>(root["diagnosticsLevel"]->asNumber());
```

- [ ] **Step 3: Add registry read**

In `EID_CSV_LoadConfigFromRegistry`, after the `CSVVerbose` read block (before the closing `__finally`):

```cpp
        // Read DiagnosticsEnabled
        dwSize = sizeof(DWORD);
        err = RegQueryValueExW(hKey, L"DiagnosticsEnabled", nullptr, &dwType,
            reinterpret_cast<LPBYTE>(&dwValue), &dwSize);
        if (err == ERROR_SUCCESS && dwType == REG_DWORD)
            config.fDiagnosticsEnabled = dwValue ? TRUE : FALSE;

        // Read DiagnosticsLevel
        dwSize = sizeof(DWORD);
        err = RegQueryValueExW(hKey, L"DiagnosticsLevel", nullptr, &dwType,
            reinterpret_cast<LPBYTE>(&config.dwDiagnosticsLevel), &dwSize);
        if (err != ERROR_SUCCESS || dwType != REG_DWORD)
            config.dwDiagnosticsLevel = 4; // WINEVENT_LEVEL_INFO
```

(Reuses the existing `dwValue` local declared for `CSVEnabled`/`CSVVerbose`.)

- [ ] **Step 4: Add registry write**

In `EID_CSV_SaveConfigToRegistry`, after the `CSVVerbose` write block (locate the last `RegSetValueExW(... L"CSVVerbose" ...)` and add after its error check):

```cpp
        dwValue = config.fDiagnosticsEnabled ? 1 : 0;
        err = RegSetValueExW(hKey, L"DiagnosticsEnabled", 0, REG_DWORD,
            reinterpret_cast<const BYTE*>(&dwValue), sizeof(DWORD));
        if (err != ERROR_SUCCESS) { hr = HRESULT_FROM_WIN32(err); __leave; }

        err = RegSetValueExW(hKey, L"DiagnosticsLevel", 0, REG_DWORD,
            reinterpret_cast<const BYTE*>(&config.dwDiagnosticsLevel), sizeof(DWORD));
        if (err != ERROR_SUCCESS) { hr = HRESULT_FROM_WIN32(err); __leave; }
```

(If `CSVVerbose` is not currently written in `SaveConfigToRegistry`, add a `CSVVerbose` write in the same style first so verbose persists too — check the tail of the function ~line 334-360.)

- [ ] **Step 5: Build and verify**

Run: `.\build.ps1 Release x64`
Expected: `Rebuild All: 11 succeeded, 0 failed` and `BUILD SUCCEEDED`.

- [ ] **Step 6: Commit**

```bash
git add EIDCardLibrary/CSVConfig.h EIDCardLibrary/CSVConfig.cpp
git commit -m "feat(logging): add diagnostics-capture config fields and persistence"
```

---

### Task 2: EIDTraceConsumer writes free-text traces to diagnostics.log

**Files:**
- Modify: `EIDTraceConsumer/EIDTraceConsumer.cpp`

**Interfaces:**
- Consumes: registry values `DiagnosticsEnabled`, `DiagnosticsLevel` (Task 1).
- Produces: `diagnostics.log` next to the configured CSV path.

- [ ] **Step 1: Add diagnostics globals**

Near the other globals (~line 60-69), after `BOOL g_fCsvEnabled = FALSE;`:

```cpp
// Diagnostics (free-text provider traces) capture
HANDLE g_hDiagFile = INVALID_HANDLE_VALUE;
WCHAR  g_szDiagPath[MAX_PATH] = {0};
DWORD  g_dwDiagFileSize = 0;
BOOL   g_fDiagnosticsEnabled = FALSE;
DWORD  g_dwDiagnosticsLevel = 4; // WINEVENT_LEVEL_INFO
```

Add forward declarations near the other ones (~line 88-94):

```cpp
BOOL EnsureDiagFileOpen();
void WriteDiagnosticLine(const WCHAR* timestamp, const WCHAR* severity, const WCHAR* message);
void RotateDiagFile();
void CloseDiagFile();
```

- [ ] **Step 2: Read the new registry values and derive the diag path in `LoadCsvConfiguration`**

After the block that reads `CSVFileCount` and before `RegCloseKey(hKey);` (~line 145):

```cpp
    // Read diagnostics settings
    DWORD dwDiag = 0;
    dwSize = sizeof(dwDiag);
    if (RegQueryValueExW(hKey, L"DiagnosticsEnabled", nullptr, nullptr,
            reinterpret_cast<LPBYTE>(&dwDiag), &dwSize) == ERROR_SUCCESS)
        g_fDiagnosticsEnabled = (dwDiag != 0);

    dwSize = sizeof(g_dwDiagnosticsLevel);
    if (RegQueryValueExW(hKey, L"DiagnosticsLevel", nullptr, nullptr,
            reinterpret_cast<LPBYTE>(&g_dwDiagnosticsLevel), &dwSize) != ERROR_SUCCESS)
        g_dwDiagnosticsLevel = 4;
```

At the very end of `LoadCsvConfiguration`, before `return g_fCsvEnabled;`, derive the diagnostics path from the CSV path's directory:

```cpp
    // diagnostics.log lives in the same directory as the CSV log
    wcscpy_s(g_szDiagPath, g_szCsvPath);
    WCHAR* pSlash = wcsrchr(g_szDiagPath, L'\\');
    if (pSlash) { *(pSlash + 1) = L'\0'; wcscat_s(g_szDiagPath, L"diagnostics.log"); }
    else        { wcscpy_s(g_szDiagPath, L"C:\\ProgramData\\EIDAuthentication\\logs\\diagnostics.log"); }
```

Note: `LoadCsvConfiguration` currently `return g_fCsvEnabled;` — leave that return as-is; callers are updated in Step 4.

- [ ] **Step 3: Add the diagnostics file writer functions**

Add after `CloseCsvFile()` (~line 327):

```cpp
BOOL EnsureDiagFileOpen()
{
    if (g_hDiagFile != INVALID_HANDLE_VALUE)
        return TRUE;

    WCHAR szDir[MAX_PATH];
    wcscpy_s(szDir, g_szDiagPath);
    WCHAR* pLastSlash = wcsrchr(szDir, L'\\');
    if (pLastSlash) { *pLastSlash = L'\0'; CreateDirectoryW(szDir, nullptr); }

    g_hDiagFile = CreateFileW(g_szDiagPath, GENERIC_WRITE, FILE_SHARE_READ,
        nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (g_hDiagFile == INVALID_HANDLE_VALUE)
        return FALSE;

    LARGE_INTEGER liSize;
    if (GetFileSizeEx(g_hDiagFile, &liSize))
        g_dwDiagFileSize = static_cast<DWORD>(liSize.QuadPart);

    SetFilePointer(g_hDiagFile, 0, nullptr, FILE_END);

    if (g_dwDiagFileSize == 0)
    {
        DWORD dwWritten = 0;
        const BYTE bom[] = {0xEF, 0xBB, 0xBF};
        WriteFile(g_hDiagFile, bom, 3, &dwWritten, nullptr);
        g_dwDiagFileSize += dwWritten;
    }
    return TRUE;
}

void RotateDiagFile()
{
    if (g_hDiagFile != INVALID_HANDLE_VALUE)
    {
        FlushFileBuffers(g_hDiagFile);
        CloseHandle(g_hDiagFile);
        g_hDiagFile = INVALID_HANDLE_VALUE;
    }

    WCHAR szRotated[MAX_PATH];
    swprintf_s(szRotated, L"%s.001", g_szDiagPath);
    MoveFileExW(g_szDiagPath, szRotated, MOVEFILE_REPLACE_EXISTING);
    g_dwDiagFileSize = 0;
}

void WriteDiagnosticLine(const WCHAR* timestamp, const WCHAR* severity, const WCHAR* message)
{
    if (!g_fDiagnosticsEnabled || !EnsureDiagFileOpen())
        return;

    DWORD dwMaxBytes = g_dwMaxFileSizeMB * 1024 * 1024;
    if (g_dwDiagFileSize >= dwMaxBytes)
    {
        RotateDiagFile();
        if (!EnsureDiagFileOpen())
            return;
    }

    char szTs[64] = {0};
    char szSev[32] = {0};
    char szMsg[3072] = {0};
    WideCharToMultiByte(CP_UTF8, 0, timestamp, -1, szTs, sizeof(szTs), nullptr, nullptr);
    WideCharToMultiByte(CP_UTF8, 0, severity, -1, szSev, sizeof(szSev), nullptr, nullptr);
    WideCharToMultiByte(CP_UTF8, 0, message, -1, szMsg, sizeof(szMsg), nullptr, nullptr);

    char szLine[4096];
    int len = sprintf_s(szLine, sizeof(szLine), "%s %s %s\r\n", szTs, szSev, szMsg);
    if (len > 0)
    {
        DWORD dwWritten = 0;
        WriteFile(g_hDiagFile, szLine, len, &dwWritten, nullptr);
        g_dwDiagFileSize += dwWritten;
    }
}

void CloseDiagFile()
{
    if (g_hDiagFile != INVALID_HANDLE_VALUE)
    {
        FlushFileBuffers(g_hDiagFile);
        CloseHandle(g_hDiagFile);
        g_hDiagFile = INVALID_HANDLE_VALUE;
    }
    g_dwDiagFileSize = 0;
}
```

- [ ] **Step 4: Gate the session on diagnostics, and close the diag file on stop**

In `StartTraceSession` (~line 517-526), change the enablement guard from CSV to diagnostics. Replace:

```cpp
    if (!g_fCsvEnabled)
    {
        wprintf(L"CSV logging disabled, skipping trace session start\n");
        return TRUE; // Not an error - just disabled
    }
```

with:

```cpp
    if (!g_fDiagnosticsEnabled)
    {
        wprintf(L"Diagnostics capture disabled, skipping trace session start\n");
        return TRUE; // Not an error - just disabled
    }
```

In `StopTraceSession` (~line 566), after `CloseCsvFile();` add:

```cpp
    CloseDiagFile();
```

- [ ] **Step 5: Route events in `EventCallback` to diagnostics.log; stop writing events.csv**

In `EventCallback`, replace the guard and the final `WriteCsvEvent(...)` call. Change the top guard (~line 387) from:

```cpp
    if (!pEvent || !g_fCsvEnabled)
        return;
```

to:

```cpp
    if (!pEvent || !g_fDiagnosticsEnabled)
        return;
```

Then replace the tail of the function — the `ParseEventIdFromMessage` / `GetEventCategory` / `WriteCsvEvent` block (~line 427-437) — with:

```cpp
    // Structured audit events carry an "[EID:NNNN]" prefix and are written to events.csv
    // by the in-process LSA logger. The consumer must NOT touch events.csv (its column
    // schema differs and would corrupt the file). Only free-text diagnostic traces are ours.
    if (wcsstr(szMessage, L"[EID:") != nullptr)
        return;

    UCHAR level = pEvent->EventHeader.EventDescriptor.Level;
    if (level == 0)
        level = 4; // EventWriteString with level 0 -> treat as INFO
    if (level > static_cast<UCHAR>(g_dwDiagnosticsLevel))
        return; // more verbose than the configured ceiling

    WriteDiagnosticLine(szTimestamp, GetSeverityName(level),
                        szMessage[0] ? szMessage : L"(no message)");
```

(The `eventId` / `szCategory` locals and `ParseEventIdFromMessage`/`GetEventCategory` calls are no longer used in `EventCallback`; remove those two locals and two calls to avoid unused-variable churn. `GetEventCategory`/`ParseEventIdFromMessage`/`WriteCsvEvent`/`EnsureCsvFileOpen`/`WriteCsvHeader`/`RotateCsvFile`/`CloseCsvFile` remain defined but unused — they are non-static file-scope functions, so this produces no warning; leave them to minimize churn.)

- [ ] **Step 6: Build and verify**

Run: `.\build.ps1 Release x64`
Expected: `Rebuild All: 11 succeeded, 0 failed` and `BUILD SUCCEEDED`.

- [ ] **Step 7: Commit**

```bash
git add EIDTraceConsumer/EIDTraceConsumer.cpp
git commit -m "feat(consumer): write provider diagnostic traces to diagnostics.log; stop writing events.csv"
```

---

### Task 3: EIDLogManager UI toggle + level + restart-on-Apply

**Files:**
- Modify: `EIDLogManager/resource.h`
- Modify: `EIDLogManager/EIDLogManager.rc`
- Modify: `EIDLogManager/EIDLogManager.cpp`

**Interfaces:**
- Consumes: `EID_CSV_CONFIG::fDiagnosticsEnabled`, `dwDiagnosticsLevel` (Task 1); service name `EIDTraceConsumer` (Task 2 target).
- Control IDs produced: `IDC_DIAG_GROUP` 252, `IDC_DIAG_ENABLE_CHECK` 253, `IDC_DIAG_LEVEL_LABEL` 254, `IDC_DIAG_LEVEL` 255.

- [ ] **Step 1: Add control IDs to `resource.h`**

After `#define IDC_CSV_OPTIONS_GROUP 251`:

```cpp
// Diagnostics capture controls (252-255)
#define IDC_DIAG_GROUP                    252     // NOSONAR - RESOURCE-01: RC.exe requires #define macros
#define IDC_DIAG_ENABLE_CHECK             253     // NOSONAR - RESOURCE-01: RC.exe requires #define macros
#define IDC_DIAG_LEVEL_LABEL              254     // NOSONAR - RESOURCE-01: RC.exe requires #define macros
#define IDC_DIAG_LEVEL                    255     // NOSONAR - RESOURCE-01: RC.exe requires #define macros
```

Change `#define _APS_NEXT_CONTROL_VALUE 252` to `256`.

- [ ] **Step 2: Add the controls to the dialog in `EIDLogManager.rc`**

Inside the `IDD_EIDLOGMANAGER_DIALOG` dialog template, after the CSV options controls, add (adjust the x/y/width to sit below the CSV group in the existing layout; widen the dialog if needed):

```
    GROUPBOX        "Diagnostics",IDC_DIAG_GROUP,7,300,340,46
    CONTROL         "Capture diagnostic traces (diagnostics.log)",IDC_DIAG_ENABLE_CHECK,
                    "Button",BS_AUTOCHECKBOX | WS_TABSTOP,14,314,300,10
    LTEXT           "Level:",IDC_DIAG_LEVEL_LABEL,14,330,30,10
    COMBOBOX        IDC_DIAG_LEVEL,48,328,90,60,
                    CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP
```

- [ ] **Step 3: Populate the controls in `LoadCSVSettings`**

In `EIDLogManager.cpp` `LoadCSVSettings`, after the `IDC_CSV_VERBOSE_CHECK` block and before `EnableDisableCSVControls(hDlg, config.fEnabled);`:

```cpp
	// Diagnostics capture
	CheckDlgButton(hDlg, IDC_DIAG_ENABLE_CHECK,
		config.fDiagnosticsEnabled ? BST_CHECKED : BST_UNCHECKED);

	HWND hDiagLevel = GetDlgItem(hDlg, IDC_DIAG_LEVEL);
	SendMessage(hDiagLevel, CB_RESETCONTENT, 0, 0);
	SendMessage(hDiagLevel, CB_ADDSTRING, 0, (LPARAM)L"Info");     // index 0 -> level 4
	SendMessage(hDiagLevel, CB_ADDSTRING, 0, (LPARAM)L"Verbose");  // index 1 -> level 5
	SendMessage(hDiagLevel, CB_SETCURSEL, (config.dwDiagnosticsLevel >= 5) ? 1 : 0, 0);
	EnableWindow(hDiagLevel, config.fDiagnosticsEnabled);
	EnableWindow(GetDlgItem(hDlg, IDC_DIAG_LEVEL_LABEL), config.fDiagnosticsEnabled);
```

- [ ] **Step 4: Read the controls in `SaveCSVSettings`**

In `SaveCSVSettings`, after the verbose/category settings are read into `config` and before the `EID_CSV_SaveConfig(config)` call (locate the save call near the end of the function):

```cpp
	config.fDiagnosticsEnabled = IsDlgButtonChecked(hDlg, IDC_DIAG_ENABLE_CHECK) == BST_CHECKED;
	{
		LRESULT sel = SendMessage(GetDlgItem(hDlg, IDC_DIAG_LEVEL), CB_GETCURSEL, 0, 0);
		config.dwDiagnosticsLevel = (sel == 1) ? 5 : 4; // Verbose=5, Info=4
	}
```

- [ ] **Step 5: Wire the checkbox enable/disable + Apply restart in the `WM_COMMAND` switch**

In the `WM_COMMAND` switch (~line 146), add a case alongside the other CSV handlers:

```cpp
				case IDC_DIAG_ENABLE_CHECK:
				{
					BOOL fOn = IsDlgButtonChecked(hDlg, IDC_DIAG_ENABLE_CHECK) == BST_CHECKED;
					EnableWindow(GetDlgItem(hDlg, IDC_DIAG_LEVEL), fOn);
					EnableWindow(GetDlgItem(hDlg, IDC_DIAG_LEVEL_LABEL), fOn);
					return (INT_PTR)TRUE;
				}
```

In the existing `case IDC_LOG_APPLY:` block, after `SaveCSVSettings(hDlg);` and before the `MessageBox(...)`:

```cpp
					RestartTraceConsumer();
```

- [ ] **Step 6: Add the `RestartTraceConsumer` helper + forward declaration**

Add a forward declaration near the other prototypes (top of file, ~line 27-31):

```cpp
void RestartTraceConsumer();
```

Add the implementation (anywhere at file scope, e.g. after `SaveCSVSettings`):

```cpp
// Restart the EIDTraceConsumer service so it re-reads the diagnostics config.
// Uses sc.exe (always in System32, no path lookup needed). Non-fatal on failure -
// the new settings apply on the service's next start regardless. The log manager
// already runs elevated (it writes HKLM), so these calls succeed without a prompt.
void RestartTraceConsumer()
{
	auto runSc = [](LPCWSTR args)
	{
		WCHAR szCmd[128];
		swprintf_s(szCmd, L"sc.exe %s", args);
		STARTUPINFOW si = { sizeof(si) };
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
		PROCESS_INFORMATION pi = {0};
		if (CreateProcessW(nullptr, szCmd, nullptr, nullptr, FALSE,
				CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
		{
			WaitForSingleObject(pi.hProcess, 5000);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	};
	runSc(L"stop EIDTraceConsumer");
	Sleep(1500); // let the service reach STOPPED before starting again
	runSc(L"start EIDTraceConsumer");
}
```

- [ ] **Step 7: Build and verify**

Run: `.\build.ps1 Release x64`
Expected: `Rebuild All: 11 succeeded, 0 failed` and `BUILD SUCCEEDED`.

- [ ] **Step 8: Commit**

```bash
git add EIDLogManager/resource.h EIDLogManager/EIDLogManager.rc EIDLogManager/EIDLogManager.cpp
git commit -m "feat(logmanager): diagnostics capture toggle + level, restart consumer on Apply"
```

---

### Task 4: End-to-end manual verification on hardware

**Files:** none (verification only).

- [ ] **Step 1: Deploy the rebuilt binaries**

Elevated, copy the built consumer and log manager over the installed copies (or run `Installer\EIDInstallx64.exe`):

```
Copy-Item x64\Release\EIDTraceConsumer.exe "<installed consumer path>" -Force
Copy-Item x64\Release\EIDLogManager.exe    "<installed logmanager path>" -Force
```

Determine the installed consumer path via `sc qc EIDTraceConsumer` (BINARY_PATH_NAME).

- [ ] **Step 2: Enable diagnostics via the UI**

Launch EIDLogManager (elevated). Tick "Capture diagnostic traces", level "Info", click Apply. Confirm the consumer restarts (`sc query EIDTraceConsumer` → RUNNING) and that an ETW session now exists: `logman query -ets` lists `EIDTraceConsumer`.

- [ ] **Step 3: Generate traces**

Lock the workstation (Win+L), insert the smart card (tile appears), remove it without clicking, then unlock.

- [ ] **Step 4: Verify the output**

Confirm:
- `C:\ProgramData\EIDAuthentication\logs\diagnostics.log` exists and contains provider traces (`Creation`, `Scenario`, `Connect Reader`, `EVID:` tile lines), each line formatted `<timestamp> <SEVERITY> <message>`.
- `events.csv` still uses its 14-column schema and is NOT corrupted (no 3-field diagnostic lines mixed in).

- [ ] **Step 5: Verify the off state**

In EIDLogManager untick "Capture diagnostic traces", Apply. Repeat Step 3. Confirm no new lines are appended to `diagnostics.log`.

- [ ] **Step 6: Verify Verbose captures more**

Re-enable at level "Verbose", Apply, repeat Step 3, and confirm VERBOSE-level lines (e.g. memory-dump/verbose traces) now also appear.

- [ ] **Step 7: Commit any doc/status updates** (if notes were added; otherwise skip).

---

## Notes for the implementer

- **Deviation from spec (restart mechanism):** the spec proposed spawning `EIDTraceConsumer.exe -stop/-start`; this plan uses `sc.exe stop/start EIDTraceConsumer` instead. Same "external process controls the service" intent, but path-independent (no need to locate the installed service exe). If you prefer to match the spec exactly, replace `RestartTraceConsumer` with `CreateProcess` calls on the service's exe (resolve its path from `sc qc EIDTraceConsumer`).
- If `build.ps1` reports a resource compile error after Task 3 Step 2, the dialog coordinates likely overflow the template — increase the dialog height/width in `IDD_EIDLOGMANAGER_DIALOG` and re-run.
- Do not deploy the credential provider DLL in Task 4 — it is unchanged by this feature; only `EIDTraceConsumer.exe` and `EIDLogManager.exe` change.
