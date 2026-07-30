# Fuzzing EID Authentication

This project fuzzes its own parsers on Windows, with MSVC's libFuzzer and
AddressSanitizer. Quick commands live in [`../fuzz/README.md`](../fuzz/README.md);
this document explains what is covered, what deliberately is not, and why the
OpenSSF Scorecard Fuzzing check will stay at zero.

## Why this exists

On 2026-07-30 a review of the parsers handling attacker-influenced input found
four memory-safety defects, three of them reachable in LSASS:

1. The SSP challenge and response token handlers performed **no length
   validation at all** — no `cbBuffer` check, not even against `sizeof` the
   fixed header — while using attacker-controlled offsets as raw pointer
   displacements.
2. The unlock-logon debug printer indexed the CSP-info buffer with raw 32-bit
   attacker offsets and printed the results with `%s`, and wrote a terminator at
   up to index 32767 of a 1000-element stack array. It ran unconditionally on
   every logon attempt.
3. `EID_SMARTCARD_CSP_INFO.dwCspInfoLen` was never bounded against the length
   the LSA supplied, while both downstream validators bounded their offsets
   against it.
4. A zero `usPasswordLen` underflowed the decrypt loop's round counter, and
   three cleanup paths derived a `SecureZeroMemory` length by summing region
   sizes that were only ever individually bounded.

The instructive detail is defect 2: a careful manual hardening pass had added a
`Pin.Length` bound twenty-five lines *below* the debug printer and missed the
identical exposure just above it. Reviews miss what they are not looking at.
A fuzzer does not have a reading order.

## Architecture

All offset and length rules live in `EIDCardLibrary/InputValidation.{h,cpp}`.
That module is **pure and total**: no allocation, no tracing, no LSA, registry
or CryptoAPI calls, and it never reads outside the `(pointer, size)` pair the
caller supplies. Two properties follow:

- The production call sites share exactly one implementation of each rule.
  Before this, the private-data layout rule existed in three places that
  disagreed on whether the header size was `sizeof` or `FIELD_OFFSET`, and the
  CSP-info rule existed in two places with different field sets — one of which
  compared byte counts against offsets used to index a `TCHAR[]`, permitting a
  2× overrun.
- Fuzz targets link that one translation unit and nothing else, so they run
  outside LSASS with no smart card present.

### The oracle pattern

A target that merely calls a validator and drops the answer finds almost
nothing: it only detects crashes *inside* the validator. Each target here goes
further and asserts the property the production code actually depends on:

> validator returns TRUE ⇒ the consumer's own arithmetic stays in bounds.

After validating, the target performs the same copies and reads the real
consumer performs, against an exact-sized ASan allocation. A validator that
wrongly returns TRUE therefore produces a heap-buffer-overflow report with a
stack trace instead of passing silently.

**This was verified, not assumed.** Removing the
`dwCspInfoLen <= dwCspDataLength` check made the `cspinfo` target report a
`heap-buffer-overflow` READ within 30 seconds and write a reproducer that
replays deterministically. An oracle nobody has seen fail is an oracle nobody
knows works.

## Coverage

| Target | Covers |
|---|---|
| `cspinfo` | `EID_SMARTCARD_CSP_INFO` interior: inner-vs-outer length, WCHAR-unit offset scaling, NUL termination of all four name fields |
| `tokenmessage` | SSP challenge and response tokens: header size, offset/length pairs, the `+ sizeof(WCHAR)` allocation wrap |
| `privatedata` | Stored-credential blob: per-region bounds, region overlap, the zeroize span, the round-counter underflow |
| `json` | The hand-rolled `JsonParser` reached from `.eidm` import — memory safety only |
| `regress` | Deterministic replay of all four defects' PoC inputs, plus benign inputs that must still be accepted |

The regression binary also asserts **benign inputs are still accepted**. Over-
rejection in this code path does not fail safe — it locks users out of the
machine — so "reject everything" must not be able to pass the suite.

## Deliberate non-targets

Not fuzzing something is a decision worth recording, so it is not silently
revisited:

- **Certificate and CRL import.** `InstallCrlFromFile` caps the read at 10 MB
  and hands the bytes straight to `CertCreateCRLContext`; the configuration
  wizard's base64 path goes to `CryptStringToBinary`. There is no manual
  parsing — a target here would be fuzzing `crypt32.dll`.
- **Smart-card responses.** There is no `SCardTransmit` anywhere in the
  repository and no manual APDU or TLV walking. Card data arrives through the
  vendor minidriver's `pfnCard*` entry points and `CryptGetKeyParam`, which
  report their own sizes.
- **The CSV audit log.** Write-only; the repository contains no CSV reader.
- **The ETW trace consumer.** Bounds `UserData` against `UserDataLength`
  correctly and reads only typed `REG_DWORD`/`REG_SZ` configuration values.
- **Profile enumeration during uninstall.** `NTUSER.DAT` is parsed by the
  kernel via `RegLoadKey`, not by this code, and the path is admin-only.
- **Network and RPC.** There are none — no sockets, no MIDL, no named pipes.

## Known gap, not covered by a target

The JSON parser reports malformed input by throwing `std::runtime_error`, and
**nothing in the `.eidm` import path catches it** — EIDMigrate has only four
`catch` sites and none covers `JsonToExportData`. A malformed file therefore
terminates the process. That is a genuine availability defect, but it is an
admin-supplied-file denial of service rather than memory corruption, and fixing
it means adding error handling in EIDMigrate rather than changing the parser.
The `json` target catches the exception so it can report memory faults only;
do not widen that catch and call the gap closed.

## Scorecard will not credit this

The OpenSSF Scorecard Fuzzing check passes only via five probes: OSS-Fuzz,
ClusterFuzzLite, OneFuzz, Go-native fuzzing, or Haskell property-based testing.
**Custom fuzz targets earn nothing**, and all three C/C++ routes are closed here:

- **OneFuzz is gone.** Microsoft ended development in August 2023 and archived
  the repository that November. The probe still exists; the platform does not.
- **OSS-Fuzz and ClusterFuzzLite build inside Linux Docker images with clang.**
  This codebase is Win32-only — LSA, CryptoAPI, SCard, toolset v143.
- OSS-Fuzz additionally requires a project to have "a significant user base
  and/or be critical to global IT infrastructure."

Earning the badge honestly would mean porting these parsers to clang/Linux
behind Win32 shims — substantial work, and the shimmed code is not what ships.
The repository accepts a permanent zero on that check. Scorecard's own
documentation notes that a low Fuzzing score is not a definitive indication of
risk; this is exactly such a case.

## Triage workflow

1. CI fails and uploads a `fuzz-crashes-<run id>` artifact.
2. Download it and replay the reproducer (see `fuzz/README.md` for the exact
   command, including the ASan DLL PATH fix-up).
3. Fix the rule in `InputValidation.cpp` — not at the call site. If a call site
   needs its own check, that is a signal the rule belongs in the module.
4. Add the crashing input as a case in `fuzz/targets/regress_main.cpp` and a
   named seed in `fuzz/Seed-Corpus.ps1`, so the defect can never return
   silently.
5. Re-run `.\fuzz\Run-Fuzzers.ps1 -Regress` and confirm the new case passes.
