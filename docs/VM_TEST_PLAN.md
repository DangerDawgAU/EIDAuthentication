# EIDAuthentication — VM Test Plan (security-uplift)

**Purpose:** Verify the `security-uplift` branch on a clean VM before merging to
`quality-fixes`. This is the explicit gate from `SECURITY_REVIEW.md`: every High/Medium
finding is code-complete, but nothing has been exercised end-to-end on a real machine.

**Branch under test:** `security-uplift` @ `43721a5`
**Installer:** `Installer\EIDInstallx64.exe` (rebuilt from this branch — confirm timestamp is
newer than commit `43721a5`, i.e. after 2026-07-15 22:20)
**Cards on hand:** MyEID Aventra (decrypt-capable), YubiKey PIV (decrypt-capable)

Work top to bottom. Tick each box; record the observed result in the Notes column when it
differs from Expected. A single ❌ on any **Core regression** or **must-block** row is a
merge blocker.

---

## Part A — Environment setup

| # | Step | Expected | ✓ | Notes |
|---|------|----------|---|-------|
| A1 | Fresh Windows 11 VM (matching target build), **local account** only, not domain-joined. | Clean baseline. | ☐ | |
| A2 | Take a VM snapshot named `baseline-preinstall`. | Restore point exists. | ☐ | |
| A3 | Install the smart-card minidriver(s) for your card(s) if not already present (the installer bundles them under the Complete install type). | Card visible in `certutil -scinfo`. | ☐ | |
| A4 | Copy `EIDInstallx64.exe` to the VM. Verify its SHA-256 against `Installer\SHA256SUMS.txt`. | Hash matches. | ☐ | |
| A5 | Run the installer elevated → Complete. Reboot. | Installs with no errors; reboots clean. | ☐ | |
| A6 | Confirm files: `EIDAuthenticationPackage.dll`, `EIDCredentialProvider.dll` registered; `EIDConfigurationWizard.exe`, `EIDMigrate.exe`, `EIDMigrateUI.exe`, `EIDManageUsers.exe`, `EIDTraceConsumer.exe` present. **`EIDLogManager.exe` must be absent** (removed on this branch). | All present except EIDLogManager. | ☐ | |
| A7 | Take snapshot `installed-clean`. | Restore point exists. | ☐ | |

---

## Part B — Core regression (must still work)

These prove the security hardening didn't break the product. Any ❌ blocks merge.

| # | Step | Expected | ✓ | Notes |
|---|------|----------|---|-------|
| B1 | Launch **EIDConfigurationWizard**, enroll the local account with the card (Option 2: real Windows password as backup). | Enrollment completes; cert bound to account. | ☐ | |
| B2 | Sign out. At the logon screen, select the smart-card tile, enter PIN. | Logs on from certificate only (no password typed). | ☐ | |
| B3 | Lock the workstation (Win+L), unlock with card + PIN. | Unlocks. | ☐ | |
| B4 | Wrong PIN at logon. | Rejected gracefully, no crash, retry allowed. | ☐ | |
| B5 | Remove card / no card present at logon. | Message tile shown; cannot log on with card. No secure-desktop wizard/reset dialog appears (M4). | ☐ | |
| B6 | Post-logon, confirm DPAPI-protected resource opens (proves stored password backup works). | Accessible. | ☐ | |
| B7 | Enroll a **second** account using the YubiKey (Option 1: blank password + blank-password GPO set). Log on with it. | Smart-card-only logon works. | ☐ | |
| B8 | Reboot; LSASS stable across several logon/lock cycles. | No LSASS crash, no event-log faults. | ☐ | |

---

## Part C — Security fix verification

Each row cites the finding. "must-block" rows are attacks the fix should now **prevent** —
a ❌ (i.e. the attack succeeds) is a merge blocker.

### H1 / H2 / M7 / M8 — LSASS memory safety

| # | Step | Expected | ✓ | Notes |
|---|------|----------|---|-------|
| C-H1 | From an **unprivileged** user, exercise the credential-management IPC path (create/has/remove stored credential) via the normal UI flows, and with any available fuzz/malformed submit-buffer harness. | LSASS validates buffer; malformed input rejected with clean error, **no LSASS crash/read** (must-block). | ☐ | |
| C-H2 | Attempt enrollment / credential store with an over-length key blob (>32 bytes) if a test path allows. | Bounded copy; rejected, no overflow (must-block). | ☐ | |
| C-M7 | `EIDMigrate validate -i <crafted.eidm>` with an oversized/Forged `PayloadLength`. | Rejected with bounds error, no OOB/DoS (must-block). | ☐ | |
| C-M8 | Import a file with malformed `EID_PRIVATE_DATA` sizes. | Parse rejects; no underflow. | ☐ | |

### H3 — Card-bound stored credentials (`RequireCardBoundCredentials`)

Policy key: `HKLM\SOFTWARE\Policies\Microsoft\Windows\SmartCardCredentialProvider`,
DWORD `RequireCardBoundCredentials` (default 0/off).

| # | Step | Expected | ✓ | Notes |
|---|------|----------|---|-------|
| C-H3a | With policy **off** (default), confirm existing behavior unchanged (B1–B6 already cover this). | Baseline works. | ☐ | |
| C-H3b | Set `RequireCardBoundCredentials=1`. Re-enroll / log on with the decrypt-capable card. | Only card-wrapped (eidpdtCrypted) creds created & used; logon succeeds. | ☐ | |
| C-H3c | With policy **on**, attempt to create/import a DPAPI/ClearText (non-card-bound) credential. | Refused (must-block). | ☐ | |
| C-H3d | With policy **on**, confirm a previously stored DPAPI/ClearText cred is **not** usable at logon. | Rejected. | ☐ | |

### M1 — Offline certificate revocation

Policy key: same subkey, DWORD `RequireRevocationCheck` (default 0/off, fail-closed when 1).

| # | Step | Expected | ✓ | Notes |
|---|------|----------|---|-------|
| C-M1a | `EIDMigrate import-crl -i <valid-signed.crl>` (CLI). | CRL installed; signature verified. | ☐ | |
| C-M1b | `EIDMigrate import-crl -i <tampered-or-wrong-signer.crl>`. | Rejected — signature check fails (must-block). | ☐ | |
| C-M1c | **EIDMigrateUI → "Manage certificate revocation"** page: install a signed CRL + toggle `RequireRevocationCheck`. | GUI installs CRL and sets policy; no CLI needed. | ☐ | |
| C-M1d | With `RequireRevocationCheck=1` and a CRL that **revokes** the card's cert installed, attempt logon. | Logon denied — revoked cert rejected (must-block). | ☐ | |
| C-M1e | With `RequireRevocationCheck=1` but **no** CRL available (revocation "unknown"). | Fail-closed: logon denied. | ☐ | |
| C-M1f | Confirm the auth stack does **cache-only** checking — pull the network / stay air-gapped; a valid non-revoked cert with a cached CRL still logs on, and no outbound network attempt occurs. | Logon works offline; no network calls (Wireshark/loopback check optional). | ☐ | |

### M2 — Elevated trust-anchor install requires confirmation

| # | Step | Expected | ✓ | Notes |
|---|------|----------|---|-------|
| C-M2a | Run `EIDConfigurationWizardElevated.exe TRUST <cert>` (or via the wizard's elevated path). | Per-cert confirmation dialog shows **subject / issuer / SHA-1**, defaults to **No**; declining aborts install (must-block: no silent machine-wide root install). | ☐ | |
| C-M2b | Run `EIDConfigurationWizardElevated.exe ENABLESIGNATUREONLY` (and `ENABLENOEKU`, `ENABLETIMEINVALID`). | Each prompts for confirmation before weakening the GPO; declining aborts. | ☐ | |
| C-M2c | Confirm the policy keys are admin-write only (unprivileged user cannot set them). | Access denied for standard user. | ☐ | |

### M3 — Import validates the certificate

| # | Step | Expected | ✓ | Notes |
|---|------|----------|---|-------|
| C-M3a | `EIDMigrate import` (with `-force`) of a file whose cert is **self-signed / untrusted / wrong-EKU / revoked**, in production mode. | Import rejects the credential (chain + EKU + offline revocation reused via `IsTrustedCertificate`) (must-block). | ☐ | |
| C-M3b | Import of a file with a **valid, trusted, correct-EKU** cert. | Import succeeds. | ☐ | |

### H4 — Migration file provenance

| # | Step | Expected | ✓ | Notes |
|---|------|----------|---|-------|
| C-H4a | `EIDMigrate export` on the issuing machine, then `EIDMigrate import`/`validate` on the VM. | CLI surfaces the provenance stamp (source **machine / operator / time**) held inside the AES-GCM+HMAC payload. | ☐ | |
| C-H4b | `EIDMigrate import -i <file> -expect-source <correct-machine>`. | Proceeds. | ☐ | |
| C-H4c | `EIDMigrate import -i <file> -expect-source <wrong-machine>`. | Refused before any account/password/group change (must-block). | ☐ | |
| C-H4d | Tamper with the `.eidm` bytes, then import with the correct passphrase. | AES-GCM/HMAC integrity fails; import aborts. | ☐ | |

### M5 / M6 — Install / service hardening

| # | Step | Expected | ✓ | Notes |
|---|------|----------|---|-------|
| C-M5 | Inspect ACLs on SYSTEM-written log dirs (CSV logger / trace consumer output). Attempt a junction/reparse redirect as a lower-priv writer. | Explicit DACL present; reparse redirect refused (must-block). | ☐ | |
| C-M6 | `sc qc <EID trace/consumer service>` — inspect `BINARY_PATH_NAME`. | Path is quoted (no unquoted-service-path CWE-428). | ☐ | |

### Audit / SIEM (supporting)

| # | Step | Expected | ✓ | Notes |
|---|------|----------|---|-------|
| C-AUD1 | After exercising H1/H2/H3 paths and a migration, inspect the structured audit output (events CSV) and the event pipeline. | Security-control events emitted; migration audit routed to pipeline; CSV is SIEM-parseable (header + rows). | ☐ | |

---

## Part D — Group Policy / logging via GPO (EIDLogManager removed)

| # | Step | Expected | ✓ | Notes |
|---|------|----------|---|-------|
| D1 | Load the ADMX/ADML (`Installer\PolicyDefinitions`) into the VM's local policy store; confirm the EID Authentication policy nodes appear (incl. ETW trace-session settings). | Policies visible in `gpedit.msc`. | ☐ | |
| D2 | Set trace/logging via GPO; confirm `EIDTraceConsumer` honors it (no EIDLogManager app needed). | Tracing controlled by policy. | ☐ | |
| D3 | Confirm no leftover EIDLogManager registration/shortcuts. | None. | ☐ | |

---

## Part E — Teardown

| # | Step | Expected | ✓ | Notes |
|---|------|----------|---|-------|
| E1 | Uninstall via the installer's uninstaller (includes certificate cleanup). | Clean removal; account reverts to password logon. | ☐ | |
| E2 | Reboot; confirm normal password logon restored. | Logs on. | ☐ | |
| E3 | Restore `baseline-preinstall` snapshot to release the VM. | Clean. | ☐ | |

---

## Sign-off

- [ ] All **Core regression** (Part B) passed.
- [ ] All **must-block** attack rows in Part C blocked as expected.
- [ ] No LSASS crash observed at any point.
- [ ] Result recorded → clear to merge `security-uplift` → `quality-fixes`.

**Tester:** ____________  **Date:** ____________  **VM build:** ____________
