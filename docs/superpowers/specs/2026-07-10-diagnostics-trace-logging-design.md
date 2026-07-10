# Diagnostics Trace Logging — Design

Date: 2026-07-10
Status: Approved (design)

## Problem

The credential provider emits free-text diagnostic traces (via `EIDCardLibraryTrace`
→ `EventWriteString` on ETW provider `{B4866A0A-DB08-4835-A26F-414B46F3244C}`),
including the tile-lifecycle `EVID:` traces used to debug the recent smart-card tile
work. Today the only way to capture them is a manual `logman` + `tracerpt` session.
There is no supported, user-facing way to turn this capture on and read the result.

We want an option in **EIDLogManager** that, when enabled, captures the provider's
diagnostic traces into a readable log file.

## Key finding that shapes the design

`events.csv` is currently written **in-process** by the LSA CSV logger
(`EIDCSVLogger`, 14-column schema). The `EIDTraceConsumer` service has its own writer
that emits a **different 5-column schema**. If the consumer's capture session became
active while the in-process logger was also writing, it would append mismatched rows
and corrupt `events.csv`. This is a latent bug and it sits directly in this feature's
path (the feature needs the consumer's session running).

Therefore the consumer is given a single, well-bounded job: **capture free-text
diagnostic traces to a dedicated `diagnostics.log`**, and stop touching `events.csv`
(the in-process logger owns it).

## Scope

- Capture **all** of the provider's free-text diagnostic traces (not only `EVID:`
  lines). The `EVID:` prefix was a temporary tag.
- Write them to a **separate** `diagnostics.log`, not into `events.csv`.
- Toggle + level are controlled from EIDLogManager and persisted like other settings.

Out of scope: changing how structured audit events reach `events.csv`; reconciling the
broader overlap between the in-process logger and the consumer beyond the corruption
fix above.

## Components

### 1. Configuration (`EIDCardLibrary/CSVConfig.h` + CSVConfig.cpp)

Add to `EID_CSV_CONFIG`:

- `BOOL fDiagnosticsEnabled;` — default `FALSE`.
- `DWORD dwDiagnosticsLevel;` — WINEVENT level ceiling, default `WINEVENT_LEVEL_INFO`
  (4). Events with `level <= dwDiagnosticsLevel` are captured (INFO captures INFO and
  more severe; VERBOSE (5) captures everything including memory-dump traces).

Persistence (mirror the existing pattern for every other field):

- Registry key `SOFTWARE\EIDAuthentication\LogManager`: values `DiagnosticsEnabled`
  (REG_DWORD) and `DiagnosticsLevel` (REG_DWORD), in
  `EID_CSV_LoadConfigFromRegistry` / `EID_CSV_SaveConfigToRegistry`.
- JSON `logging.json`: keys `diagnosticsEnabled`, `diagnosticsLevel`, in
  `EID_CSV_ConfigToJson` / `EID_CSV_JsonToConfig`.

The diagnostics file path is **derived**, not separately configured: the directory of
`szLogPath` + `diagnostics.log` (e.g. `C:\ProgramData\EIDAuthentication\logs\diagnostics.log`).

### 2. `EIDTraceConsumer` (EIDTraceConsumer.cpp)

- `LoadCsvConfiguration`: also read `DiagnosticsEnabled` and `DiagnosticsLevel` into
  new globals `g_fDiagnosticsEnabled`, `g_dwDiagnosticsLevel`.
- `StartTraceSession`: create the real-time session when **`g_fDiagnosticsEnabled`**
  is set. (Today it gates on `g_fCsvEnabled`; since the consumer no longer writes
  `events.csv`, CSV-enabled alone gives the consumer nothing to do, so the gate moves
  to the diagnostics flag.)
- `EventCallback`: classify each event by whether its message begins with the
  `[EID:` prefix.
  - **Has `[EID:` prefix** (structured audit event): the consumer does **nothing** —
    the in-process logger owns `events.csv`. (Removes the corruption risk.)
  - **No prefix** (free-text diagnostic): if `g_fDiagnosticsEnabled` and
    `EventDescriptor.Level <= g_dwDiagnosticsLevel`, write one line to
    `diagnostics.log`: `<ISO8601 timestamp> <SEVERITY> <message>`.
- File writing: repurpose the existing open/append/rotation helpers to target
  `diagnostics.log` (plain text, UTF-8 + BOM, size-based rotation using the existing
  `CSVMaxFileSize` / `CSVFileCount`). The old `events.csv` writer path in the consumer
  is removed.

Effect matrix (the consumer's behavior now depends only on the diagnostics flag; CSV
enablement is irrelevant to the consumer because the in-process logger owns
`events.csv`):

| Diagnostics enabled | Consumer session created | events.csv written by consumer | diagnostics.log |
|---|---|---|---|
| off | no  | no | no |
| on  | yes | no | yes |

### 3. `EIDLogManager` UI (EIDLogManager.rc, resource.h, EIDLogManager.cpp)

- New "Diagnostics" group box in the settings dialog near the CSV controls:
  - Checkbox `IDC_DIAG_ENABLE_CHECK` — "Capture diagnostic traces (diagnostics.log)".
  - Level dropdown `IDC_DIAG_LEVEL` — two items: "Info" (4) and "Verbose" (5),
    default Info. Disabled when the checkbox is unchecked.
- Load (`LoadCsvSettingsToDialog` equivalent): set checkbox and dropdown from
  `config.fDiagnosticsEnabled` / `config.dwDiagnosticsLevel`.
- Save (`SaveCsvSettingsFromDialog` equivalent): read them back into the config before
  `EID_CSV_SaveConfig`.
- Enable/disable the dropdown from the checkbox in the `WM_COMMAND` handler (mirror
  `IDC_CSV_ENABLE_CHECK` → `EnableDisableCSVControls`).

### 4. Applying changes

The consumer reads config at session start, so a config change needs the service to
re-read. On **Apply** (`IDC_LOG_APPLY` / CSV apply path), after
`EID_CSV_SaveConfig` succeeds, the log manager restarts the consumer by launching the
consumer's own service wrappers:

- `EIDTraceConsumer.exe -stop` then `EIDTraceConsumer.exe -start` via `CreateProcess`
  (or `ShellExecute` with the installed path), waiting briefly for each.

This reuses the consumer's existing `-stop`/`-start` handling instead of embedding SCM
API code in the log manager. Requires elevation (the log manager already writes HKLM
and configures logging, so it runs elevated). If the restart fails (e.g. service not
installed), surface a non-fatal message; the new settings still apply on the service's
next start.

## Error handling

- Missing/failed registry reads fall back to defaults (`fDiagnosticsEnabled = FALSE`).
- `diagnostics.log` open failure: the consumer logs to its console/debug output and
  skips diagnostic writes (does not crash; CSV/other behavior unaffected).
- Consumer restart failure on Apply: non-fatal warning in the log manager.

## Testing / verification

Manual, on the target machine (same method used to validate the tile fix):

1. Enable "Capture diagnostic traces" (Info) in EIDLogManager, Apply.
2. At the lock screen, insert/remove the card.
3. Confirm `diagnostics.log` contains the provider traces (Creation, Scenario,
   Connect, `EVID:` tile lines) and that `events.csv` is unchanged and uncorrupted.
4. Toggle the option off, Apply, repeat — confirm no new diagnostic lines are written.
5. Verify Verbose level additionally captures VERBOSE traces.

## Notes

- This feature makes the temporary `EVID:` traces useful as a shipped capability, so
  they no longer need to be stripped from the code — they become part of the general
  diagnostic stream captured on demand.
