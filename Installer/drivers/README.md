# Bundled Smart Card Minidrivers

This directory holds the signed vendor minidriver packages that ship with
the **Complete** install type. They are staged here at build time so the
resulting `EIDInstallx64.exe` is self-contained and does **not reach the
internet at install time** — suitable for air-gapped / isolated
deployments.

## Contents (expected by `build.ps1`)

| File                                    | Product                                            | SHA-256                                                                 | Bytes     |
|-----------------------------------------|----------------------------------------------------|-------------------------------------------------------------------------|-----------|
| `MyEID_Minidriver.zip`                  | Aventra MyEID Minidriver 3.0.1.2 (Certified)       | `E9789B800A1201BC7A7F8B72FB3C9C8B10CAAFD4541C013F1383F5BA2CF3A510`      |   616,068 |
| `YubiKey-Minidriver-5.0.4.273-x64.msi`  | YubiKey Smart Card Minidriver 5.0.4.273 (x64)      | `4C657C148C6DA60127094F8A64345317906A49DF25C3AAD0898C0FEFA6E07FB3`      | 2,994,176 |
| `WindowsUpdate_Minidriver.cab`          | Idemia IDOne PIV 2.4.3 Minidriver (Windows Update) | `F17279C3AA07497874D7DC8E3A4F1F500AA20E9BD273A8DD34BE7A37231356A9`      | 7,838,110 |

## How the build obtains them

`build.ps1` runs a staging step **before** the Visual Studio build:

1. If a file already exists and its SHA-256 matches the manifest above → reuse.
2. If a file is missing → download from the upstream URL (HTTPS + TLS 1.2).
3. If a hash mismatches → the file is re-downloaded; a second mismatch aborts the build.
4. If there is no internet **and** the file is missing → the build aborts with a clear message.

This means:

- **Build host with internet** → fully automatic, reproducible.
- **Fully offline build host** → pre-stage all three files here manually (copy from removable media, for example) and `build.ps1` will verify the hashes and proceed without reaching out.

## Upstream URLs

- MyEID: `https://aventra.fi/wp-content/uploads/2026/03/MyEID_Minidriver_3-0-1-2_Certified.zip`
- YubiKey: `https://downloads.yubico.com/support/YubiKey-Minidriver-5.0.4.273-x64.msi`
- Windows Update catalog: `https://catalog.s.download.windowsupdate.com/d/msdownload/update/driver/drvs/2025/10/33c66d53-0881-47f8-9714-081138ca4d26_00cfc05dd035d167c8d86691958366fe061faed5.cab`

## Git policy

These binaries are **not** tracked in git (see `.gitignore`). They are vendor
redistributables with their own terms; the authoritative copy is upstream.
The manifest (file + hash + size) lives in `build.ps1` and in this README.

## Redistribution notes

Before shipping an installer containing these files, confirm each vendor's
redistribution terms:

- **Aventra MyEID** — check the `Release_note.txt` inside the ZIP.
- **Yubico** — YubiKey Minidriver is freely redistributable per Yubico's download page terms.
- **Idemia IDOne PIV (Windows Update)** — distributed through the Microsoft Update catalog under Microsoft's standard redistribution license.
