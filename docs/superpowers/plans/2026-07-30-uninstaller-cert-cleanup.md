# Uninstaller EID Certificate Cleanup Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the uninstaller's "Remove EID Root Certificate Authority and user certificates" option actually work: native cleanup of all `EID:`-subject and `EID:`-issued certificates from machine stores and every user profile, plus the CA's machine key container.

**Architecture:** New `RemoveAllEIDCertificates()` in EIDCardLibrary does the sweep; a thin `CleanupEIDCertificates` rundll32 export on EIDAuthenticationPackage.dll (mirroring the existing `CleanupLsaCredentials`) exposes it to the NSIS uninstaller, which replaces the broken inline PowerShell with an `ExecWait rundll32` call.

**Tech Stack:** Win32 CryptoAPI (CertOpenStore/CertEnumCertificatesInStore/CertDeleteCertificateFromStore, CertEnumSystemStore, CryptAcquireContext), Win32 registry (RegLoadKey hive mounting), NSIS.

**Spec:** `docs/superpowers/specs/2026-07-30-uninstaller-cert-cleanup-design.md` (approved).

## Global Constraints

- Branch: `binskim-hardening` (lands in PR #53). Do not create a new branch.
- Build ONLY via `.\build.ps1 Debug x64` (C++ verify) / `.\build.ps1 Release x64` (installer verify). Never msbuild a single project.
- NEVER commit `Installer/SHA256SUMS.txt` — a Release build rewrites it; restore with `git checkout -- Installer/SHA256SUMS.txt`.
- Match rule everywhere: subject CN **starts with** `EID:` (root CA) OR issuer CN **starts with** `EID:` (issued certs). Prefix match, never substring.
- Key-container deletion ONLY for subject-matched certs whose `CRYPT_KEY_PROV_INFO` has `CRYPT_MACHINE_KEYSET` — this is the safety rail that makes smart-card key destruction impossible. Do not weaken it.
- Code style: match EIDCardLibrary conventions — tabs, C-style buffers, `EIDCardLibraryTrace`, `EIDAlloc`/`EIDFree`, no STL in these files.
- Cleanup must never block uninstall: trace-and-skip on every per-store/per-profile failure.
- No unit-test infrastructure exists in this repo; the test cycle per task is a clean build via build.ps1, with the VM install/uninstall test as the final gate (spec §7).

---

### Task 1: `RemoveAllEIDCertificates()` in EIDCardLibrary

**Files:**
- Modify: `EIDCardLibrary/CertificateUtilities.h` (add declaration at the end, before any closing guard)
- Modify: `EIDCardLibrary/CertificateUtilities.cpp` (append implementation at end of file)

**Interfaces:**
- Consumes: `EIDCardLibraryTrace` (Tracing.h, already included), `EIDAlloc`/`EIDFree` (already used in this file).
- Produces: `HRESULT RemoveAllEIDCertificates(VOID);` — S_OK if the sweep ran (even with skips), failure HRESULT only if it could not start. Task 2 calls exactly this.

- [ ] **Step 1: Add the declaration to `CertificateUtilities.h`**

Add at the end of the file (after the last existing declaration):

```cpp
// Uninstall cleanup: removes every certificate whose subject CN starts with "EID:"
// (the wizard-created root CA) or whose issuer CN starts with "EID:" (certificates
// issued by that CA) from the LocalMachine Root/CA/TrustedPeople/My stores and from
// the same stores of every user profile (loaded hives via CertEnumSystemStore, unloaded
// hives via temporary RegLoadKey). Deletes the CA's machine key container.
// Never deletes smart-card key containers (machine-keyset check).
// Returns S_OK when the sweep ran, even if individual stores/profiles were skipped.
HRESULT RemoveAllEIDCertificates(VOID);
```

- [ ] **Step 2: Append the implementation to `CertificateUtilities.cpp`**

```cpp
//////////////////////////////////////////////////////////////////////////////
// Uninstall certificate cleanup
//////////////////////////////////////////////////////////////////////////////

namespace
{
	struct EID_CERT_CLEANUP_STATS
	{
		DWORD dwCertsRemoved;
		DWORD dwKeysDeleted;
		DWORD dwProfilesSwept;
		DWORD dwErrors;
	};

	LPCWSTR const EID_SWEEP_STORE_NAMES[] = { L"Root", L"CA", L"TrustedPeople", L"My" };

	BOOL HasEIDPrefix(LPCWSTR szName)
	{
		return szName != nullptr && _wcsnicmp(szName, L"EID:", 4) == 0;
	}

	// TRUE if the certificate was created by this product. *pfSubjectMatch is set when
	// the SUBJECT carries the EID: prefix (i.e. the certificate IS the EID root CA).
	BOOL IsEIDOwnedCertificate(PCCERT_CONTEXT pCertContext, PBOOL pfSubjectMatch)
	{
		WCHAR szName[512] = L"";  // NOSONAR - LSASS-01: C-style buffer for LSASS safety
		*pfSubjectMatch = FALSE;
		if (CertGetNameStringW(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, szName, ARRAYSIZE(szName)) > 1
			&& HasEIDPrefix(szName))
		{
			*pfSubjectMatch = TRUE;
			return TRUE;
		}
		if (CertGetNameStringW(pCertContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, nullptr, szName, ARRAYSIZE(szName)) > 1
			&& HasEIDPrefix(szName))
		{
			return TRUE;
		}
		return FALSE;
	}

	// Deletes the private key container of the EID root CA. Safety rail: only machine
	// keysets qualify — smart-card containers never carry CRYPT_MACHINE_KEYSET, so a
	// card key cannot be destroyed here by construction.
	void DeleteMachineKeyContainer(PCCERT_CONTEXT pCertContext, EID_CERT_CLEANUP_STATS* pStats)
	{
		DWORD dwSize = 0;
		if (!CertGetCertificateContextProperty(pCertContext, CERT_KEY_PROV_INFO_PROP_ID, nullptr, &dwSize))
		{
			return; // no private key recorded — nothing to delete
		}
		PCRYPT_KEY_PROV_INFO pInfo = (PCRYPT_KEY_PROV_INFO) EIDAlloc(dwSize);
		if (!pInfo)
		{
			pStats->dwErrors++;
			return;
		}
		if (CertGetCertificateContextProperty(pCertContext, CERT_KEY_PROV_INFO_PROP_ID, pInfo, &dwSize)
			&& (pInfo->dwFlags & CRYPT_MACHINE_KEYSET))
		{
			HCRYPTPROV hProv = NULL;
			if (CryptAcquireContextW(&hProv, pInfo->pwszContainerName, pInfo->pwszProvName,
				pInfo->dwProvType, CRYPT_DELETEKEYSET | CRYPT_MACHINE_KEYSET))
			{
				// CRYPT_DELETEKEYSET returns no handle to release
				pStats->dwKeysDeleted++;
				EIDCardLibraryTrace(WINEVENT_LEVEL_INFO, L"RemoveAllEIDCertificates: deleted CA key container %s", pInfo->pwszContainerName);
			}
			else
			{
				pStats->dwErrors++;
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING, L"RemoveAllEIDCertificates: CryptAcquireContext(DELETEKEYSET) failed 0x%08x", GetLastError());
			}
		}
		EIDFree(pInfo);
	}

	// Removes every EID-owned certificate from an open store. Restarts enumeration after
	// each deletion (CertDeleteCertificateFromStore frees the context, which would
	// invalidate the enumerator). Store sizes are tiny, so O(n^2) is irrelevant.
	void CleanOpenStore(HCERTSTORE hStore, LPCWSTR szLabel, EID_CERT_CLEANUP_STATS* pStats)
	{
		BOOL fDeletedOne = TRUE;
		while (fDeletedOne)
		{
			fDeletedOne = FALSE;
			PCCERT_CONTEXT pCert = nullptr;
			while ((pCert = CertEnumCertificatesInStore(hStore, pCert)) != nullptr)
			{
				BOOL fSubjectMatch = FALSE;
				if (!IsEIDOwnedCertificate(pCert, &fSubjectMatch))
				{
					continue;
				}
				if (fSubjectMatch)
				{
					DeleteMachineKeyContainer(pCert, pStats);
				}
				// CertDeleteCertificateFromStore always frees pCert (success or failure)
				if (CertDeleteCertificateFromStore(pCert))
				{
					pStats->dwCertsRemoved++;
					fDeletedOne = TRUE;
				}
				else
				{
					pStats->dwErrors++;
					EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING, L"RemoveAllEIDCertificates: delete failed in %s (0x%08x)", szLabel, GetLastError());
					// leave fDeletedOne FALSE: abandon this store instead of looping on a stuck certificate
				}
				break; // restart enumeration from scratch after any deletion attempt
			}
		}
	}

	// CertEnumSystemStore callback for CERT_SYSTEM_STORE_USERS: store names arrive as
	// "<SID>\<StoreName>" for every loaded profile hive.
	BOOL WINAPI CleanUserSystemStoreCallback(const void* pvSystemStore, DWORD dwFlags, PCERT_SYSTEM_STORE_INFO pStoreInfo, void* pvReserved, void* pvArg)  // NOSONAR - API-01: signature dictated by Windows/callback API
	{
		UNREFERENCED_PARAMETER(dwFlags);
		UNREFERENCED_PARAMETER(pStoreInfo);
		UNREFERENCED_PARAMETER(pvReserved);
		EID_CERT_CLEANUP_STATS* pStats = (EID_CERT_CLEANUP_STATS*) pvArg;
		LPCWSTR szStore = (LPCWSTR) pvSystemStore;
		LPCWSTR szBackslash = wcsrchr(szStore, L'\\');
		if (!szBackslash)
		{
			return TRUE;
		}
		LPCWSTR szName = szBackslash + 1;
		BOOL fWanted = FALSE;
		for (DWORD i = 0; i < ARRAYSIZE(EID_SWEEP_STORE_NAMES); i++)
		{
			if (_wcsicmp(szName, EID_SWEEP_STORE_NAMES[i]) == 0)
			{
				fWanted = TRUE;
				break;
			}
		}
		if (fWanted)
		{
			HCERTSTORE hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, NULL,  // NOSONAR - Windows API requires NULL
				CERT_SYSTEM_STORE_USERS | CERT_STORE_OPEN_EXISTING_FLAG, szStore);
			if (hStore)
			{
				CleanOpenStore(hStore, szStore, pStats);
				CertCloseStore(hStore, 0);
			}
		}
		return TRUE; // always continue enumeration
	}

	BOOL EnablePrivilege(LPCWSTR szPrivilege)
	{
		HANDLE hToken = NULL;
		TOKEN_PRIVILEGES tp = {0};
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
		{
			return FALSE;
		}
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		BOOL fOk = LookupPrivilegeValueW(nullptr, szPrivilege, &tp.Privileges[0].Luid)
			&& AdjustTokenPrivileges(hToken, FALSE, &tp, 0, nullptr, nullptr)
			&& GetLastError() == ERROR_SUCCESS;
		CloseHandle(hToken);
		return fOk;
	}

	// Opens a certificate store rooted at an arbitrary registry key (used on mounted
	// NTUSER.DAT hives) and cleans it. CERT_STORE_PROV_REG persists deletions to the key.
	void CleanRegistryStore(HKEY hHiveRoot, LPCWSTR szStoreName, EID_CERT_CLEANUP_STATS* pStats)
	{
		WCHAR szSubKey[MAX_PATH] = L"";  // NOSONAR - LSASS-01: C-style buffer for LSASS safety
		swprintf_s(szSubKey, ARRAYSIZE(szSubKey), L"SOFTWARE\\Microsoft\\SystemCertificates\\%s", szStoreName);
		HKEY hKey = NULL;
		if (RegOpenKeyExW(hHiveRoot, szSubKey, 0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS)
		{
			return; // store never created for this profile — nothing to clean
		}
		HCERTSTORE hStore = CertOpenStore(CERT_STORE_PROV_REG, 0, NULL, 0, hKey);  // NOSONAR - Windows API requires NULL
		if (hStore)
		{
			CleanOpenStore(hStore, szStoreName, pStats);
			CertCloseStore(hStore, 0);
		}
		RegCloseKey(hKey);
	}

	// Mounts each not-currently-loaded user profile hive (ProfileList enumeration,
	// S-1-5-21-* accounts only) and cleans its certificate stores. Loaded hives are
	// covered separately by CertEnumSystemStore(CERT_SYSTEM_STORE_USERS).
	void SweepUnloadedProfiles(EID_CERT_CLEANUP_STATS* pStats)
	{
		if (!EnablePrivilege(SE_BACKUP_NAME) || !EnablePrivilege(SE_RESTORE_NAME))
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING, L"RemoveAllEIDCertificates: backup/restore privilege unavailable (0x%08x) - unloaded profiles skipped", GetLastError());
			pStats->dwErrors++;
			return;
		}
		HKEY hProfileList = NULL;
		if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList",
			0, KEY_READ, &hProfileList) != ERROR_SUCCESS)
		{
			pStats->dwErrors++;
			return;
		}
		for (DWORD dwIndex = 0; ; dwIndex++)
		{
			WCHAR szSid[256] = L"";  // NOSONAR - LSASS-01: C-style buffer for LSASS safety
			DWORD dwSidLen = ARRAYSIZE(szSid);
			if (RegEnumKeyExW(hProfileList, dwIndex, szSid, &dwSidLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
			{
				break;
			}
			if (_wcsnicmp(szSid, L"S-1-5-21-", 9) != 0)
			{
				continue; // only real local/domain accounts
			}
			HKEY hLoaded = NULL;
			if (RegOpenKeyExW(HKEY_USERS, szSid, 0, KEY_READ, &hLoaded) == ERROR_SUCCESS)
			{
				RegCloseKey(hLoaded);
				continue; // hive loaded — already swept via CertEnumSystemStore
			}
			WCHAR szHivePath[MAX_PATH] = L"";  // NOSONAR - LSASS-01: C-style buffer for LSASS safety
			DWORD cbPath = sizeof(szHivePath);
			// RRF_RT_REG_SZ alone: RegGetValue auto-expands REG_EXPAND_SZ and returns it
			// as REG_SZ (adding RRF_RT_REG_EXPAND_SZ without RRF_NOEXPAND is an error)
			if (RegGetValueW(hProfileList, szSid, L"ProfileImagePath", RRF_RT_REG_SZ,
				nullptr, szHivePath, &cbPath) != ERROR_SUCCESS)
			{
				continue;
			}
			if (FAILED(StringCchCatW(szHivePath, ARRAYSIZE(szHivePath), L"\\NTUSER.DAT")))
			{
				continue;
			}
			LSTATUS lLoad = RegLoadKeyW(HKEY_USERS, L"EID_CertCleanup_Tmp", szHivePath);
			if (lLoad != ERROR_SUCCESS)
			{
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING, L"RemoveAllEIDCertificates: RegLoadKey failed for %s (0x%08x) - profile skipped", szSid, lLoad);
				pStats->dwErrors++;
				continue;
			}
			HKEY hHive = NULL;
			if (RegOpenKeyExW(HKEY_USERS, L"EID_CertCleanup_Tmp", 0, KEY_READ | KEY_WRITE, &hHive) == ERROR_SUCCESS)
			{
				for (DWORD i = 0; i < ARRAYSIZE(EID_SWEEP_STORE_NAMES); i++)
				{
					CleanRegistryStore(hHive, EID_SWEEP_STORE_NAMES[i], pStats);
				}
				RegCloseKey(hHive);
				pStats->dwProfilesSwept++;
			}
			LSTATUS lUnload = RegUnLoadKeyW(HKEY_USERS, L"EID_CertCleanup_Tmp");
			if (lUnload != ERROR_SUCCESS)
			{
				EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING, L"RemoveAllEIDCertificates: RegUnLoadKey failed for %s (0x%08x)", szSid, lUnload);
				pStats->dwErrors++;
			}
		}
		RegCloseKey(hProfileList);
	}
}

HRESULT RemoveAllEIDCertificates(VOID)
{
	EID_CERT_CLEANUP_STATS stats = {0};
	EIDCardLibraryTrace(WINEVENT_LEVEL_INFO, L"RemoveAllEIDCertificates: starting certificate cleanup");

	// 1. LocalMachine stores — everything MakeTrustedCertifcate and the wizard write to
	for (DWORD i = 0; i < ARRAYSIZE(EID_SWEEP_STORE_NAMES); i++)
	{
		HCERTSTORE hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, NULL,  // NOSONAR - Windows API requires NULL
			CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_OPEN_EXISTING_FLAG, EID_SWEEP_STORE_NAMES[i]);
		if (hStore)
		{
			CleanOpenStore(hStore, EID_SWEEP_STORE_NAMES[i], &stats);
			CertCloseStore(hStore, 0);
		}
	}

	// 2. Loaded user profiles (logged-on users, .DEFAULT)
	if (!CertEnumSystemStore(CERT_SYSTEM_STORE_USERS, nullptr, &stats, CleanUserSystemStoreCallback))
	{
		EIDCardLibraryTrace(WINEVENT_LEVEL_WARNING, L"RemoveAllEIDCertificates: CertEnumSystemStore failed 0x%08x", GetLastError());
		stats.dwErrors++;
	}

	// 3. Unloaded user profiles (users not logged on during uninstall — the common case)
	SweepUnloadedProfiles(&stats);

	EIDCardLibraryTrace(WINEVENT_LEVEL_INFO,
		L"RemoveAllEIDCertificates: removed %u certificates, deleted %u key containers, swept %u offline profiles, %u errors",
		stats.dwCertsRemoved, stats.dwKeysDeleted, stats.dwProfilesSwept, stats.dwErrors);
	return S_OK; // partial skips never fail the uninstall
}
```

Note: `strsafe.h` (`StringCchCatW`) is already used across EIDCardLibrary; if this translation unit doesn't include it, add `#include <strsafe.h>` after the existing includes. Same for `<wincrypt.h>` (already pulled in via existing cert calls in this file).

- [ ] **Step 3: Build to verify it compiles**

Run: `.\build.ps1 Debug x64`
Expected: `========== Rebuild All:` summary in build.log with 0 failed. If `SE_BACKUP_NAME`/`LookupPrivilegeValueW` link errors appear, confirm `advapi32.lib` is in EIDCardLibrary's consumers' linker inputs (it already is — `AllocateAndInitializeSid` is used in this same file).

- [ ] **Step 4: Commit**

```powershell
git add EIDCardLibrary/CertificateUtilities.h EIDCardLibrary/CertificateUtilities.cpp
git commit -m @'
feat: native EID certificate sweep for uninstall cleanup

RemoveAllEIDCertificates removes every cert with an EID: subject (the
wizard root CA) or EID: issuer (certs it issued) from LocalMachine
Root/CA/TrustedPeople/My and the same stores of every user profile,
mounting unloaded NTUSER.DAT hives. Deletes the CA machine key
container; machine-keyset check makes touching smart-card keys
impossible.

Co-Authored-By: Claude Fable 5 <noreply@anthropic.com>
Claude-Session: https://claude.ai/code/session_01ENvebUKMLiPxTZUY5q8bcV
'@
```

---

### Task 2: `CleanupEIDCertificates` export on EIDAuthenticationPackage.dll

**Files:**
- Modify: `EIDAuthenticationPackage/EIDAuthenticationPackage.cpp` (add function directly AFTER the closing brace of `CleanupLsaCredentials`, ~line 1520, inside the same linkage scope)
- Modify: `EIDAuthenticationPackage/EIDAuthenticationPackage.def` (add export after line 9 `CleanupLsaCredentials`)

**Interfaces:**
- Consumes: `HRESULT RemoveAllEIDCertificates(VOID)` from Task 1 (`EIDCardLibrary/CertificateUtilities.h`).
- Produces: rundll32-callable export `CleanupEIDCertificates` — Task 3's uninstaller calls `rundll32.exe "...\EIDAuthenticationPackage.dll",CleanupEIDCertificates`.

- [ ] **Step 1: Add the export to the .def file**

In `EIDAuthenticationPackage.def`, after the `CleanupLsaCredentials` line, add:

```
CleanupEIDCertificates
```

- [ ] **Step 2: Add the wrapper function**

First check the top of `EIDAuthenticationPackage.cpp` for its includes: if `CertificateUtilities.h` is not already included, add `#include "../EIDCardLibrary/CertificateUtilities.h"` alongside the existing EIDCardLibrary includes. Verify how `CleanupLsaCredentials` is scoped (it must be `extern "C"` or in an extern-"C" block for the undecorated .def export to link) and place the new function in the SAME scope, directly after it:

```cpp
	// CleanupEIDCertificates - Removes the EID root CA (certificate + machine key
	// container) and every certificate it issued, from machine stores and all user
	// profiles. Called by the uninstaller via rundll32 before this DLL is deleted.
	HRESULT WINAPI CleanupEIDCertificates()
	{
		HRESULT hr = E_FAIL;  // NOSONAR - EXPLICIT-TYPE-03: HRESULT visible for security audit
		EIDCardLibraryTrace(WINEVENT_LEVEL_INFO, L"CleanupEIDCertificates: starting");
		__try
		{
			hr = RemoveAllEIDCertificates();
		}
		__except(EIDExceptionHandler(GetExceptionInformation()))
		{
			EIDCardLibraryTrace(WINEVENT_LEVEL_ERROR, L"CleanupEIDCertificates: Exception 0x%08x", GetExceptionCode());
			EIDLogStackTrace(GetExceptionCode());
			hr = E_FAIL;
		}
		EIDCardLibraryTrace(WINEVENT_LEVEL_INFO, L"CleanupEIDCertificates: finished 0x%08x", hr);
		return hr;
	}
```

- [ ] **Step 3: Build to verify the export links**

Run: `.\build.ps1 Debug x64`
Expected: 0 failed. Then verify the export exists:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe" /exports x64\Debug\EIDAuthenticationPackage.dll | Select-String CleanupEIDCertificates
```

Expected: one line containing `CleanupEIDCertificates`. (If the dumpbin path glob fails, find it with `Get-ChildItem "C:\Program Files\Microsoft Visual Studio" -Recurse -Filter dumpbin.exe | Select-Object -First 1`.)

- [ ] **Step 4: Commit**

```powershell
git add EIDAuthenticationPackage/EIDAuthenticationPackage.cpp EIDAuthenticationPackage/EIDAuthenticationPackage.def
git commit -m @'
feat: CleanupEIDCertificates rundll32 export for the uninstaller

Mirrors the CleanupLsaCredentials pattern; thin SEH-guarded wrapper
around RemoveAllEIDCertificates.

Co-Authored-By: Claude Fable 5 <noreply@anthropic.com>
Claude-Session: https://claude.ai/code/session_01ENvebUKMLiPxTZUY5q8bcV
'@
```

---

### Task 3: Installer changes — call the export, fix checkbox defaults

**Files:**
- Modify: `Installer/Installerx64.nsi:438-491` (uninstall options page + delete `un.RemoveEIDCertificates`)
- Modify: `Installer/Installerx64.nsi:508-514` (call site in `Section "Uninstall"`)

**Interfaces:**
- Consumes: the `CleanupEIDCertificates` export from Task 2 (already in `$SYSDIR\EIDAuthenticationPackage.dll` at uninstall time; the DLL is deleted later in the section, so ordering is already correct).
- Produces: nothing consumed by later tasks.

- [ ] **Step 1: Update the options page — defaults unchecked, honest label**

In `Function un.ShowUninstallOptions`, replace the two checkbox blocks (currently lines 445-453) with:

```nsis
  ; Checkbox for removing EID certificate mappings from users (LSA credentials)
  ; Default UNCHECKED: destructive cleanup is opt-in (a temporary uninstall/upgrade
  ; must not destroy enrollments)
  ${NSD_CreateCheckbox} 10u 50u 100% 12u "Remove EID certificate mappings from users"
  Pop $Uninstall_RemoveMappings

  ; Checkbox for removing EID root CA + issued certificates
  ${NSD_CreateCheckbox} 10u 70u 100% 24u "Remove EID Root Certificate Authority and all EID-issued user certificates from this machine, including the CA private key (irreversible)"
  Pop $Uninstall_RemoveCertificates
```

(The two `${NSD_Check}` lines are removed — that is the defaults change. The second checkbox gets `24u` height for the two-line label.)

- [ ] **Step 2: Delete `Function un.RemoveEIDCertificates` entirely**

Remove the whole function (lines 464-491, from `Function un.RemoveEIDCertificates` through its `FunctionEnd`) — the broken multi-argument `nsExec::ExecToLog` PowerShell block goes away completely.

- [ ] **Step 3: Replace the call site in `Section "Uninstall"`**

Replace:

```nsis
  ; Conditionally remove certificates created by the software (if checkbox was selected)
  ${If} $Uninstall_RemoveCertificates = 1
    DetailPrint "Removing EID Root Certificates..."
    Call un.RemoveEIDCertificates
  ${Else}
    DetailPrint "Skipping certificate removal (not selected)"
  ${EndIf}
```

with:

```nsis
  ; Conditionally remove certificates created by the software (if checkbox was selected).
  ; Native cleanup inside the package DLL (still present - deleted later in this
  ; section): sweeps machine stores and every user profile, deletes the CA key.
  ${If} $Uninstall_RemoveCertificates = 1
    DetailPrint "Removing EID certificates (machine stores and all user profiles)..."
    ${DisableX64FSRedirection}
    ExecWait 'rundll32.exe "$SYSDIR\EIDAuthenticationPackage.dll",CleanupEIDCertificates' $2
    ${If} $2 != 0
      DetailPrint "Warning: certificate cleanup returned code $2 - some certificates may remain"
    ${EndIf}
    ${EnableX64FSRedirection}
  ${Else}
    DetailPrint "Skipping certificate removal (not selected)"
  ${EndIf}
```

(Note: rundll32 usually exits 0 regardless of the function's HRESULT — the real diagnostics are the trace counts. The `$2` check is best-effort, same as the existing LSA-cleanup call.)

- [ ] **Step 4: Build Release to compile the NSIS script**

Run: `.\build.ps1 Release x64`
Expected: build.log ends with a successful NSIS `makensis` run producing `Installer\EIDInstallx64.exe`, 0 failed projects. This is the only way the .nsi compiles locally.

- [ ] **Step 5: Restore the build-dirtied checksum file, commit**

```powershell
git checkout -- Installer/SHA256SUMS.txt
git add Installer/Installerx64.nsi
git commit -m @'
fix: uninstaller certificate cleanup never ran and missed most stores

The old un.RemoveEIDCertificates passed its PowerShell script to
nsExec::ExecToLog as ~16 separate NSIS plugin arguments; ExecToLog pops
only the first, so PowerShell ran with an empty -Command and removed
nothing. Its filter also matched only EID: SUBJECTS, missing every
user certificate (issuer-only match), and skipped the LocalMachine
CA/TrustedPeople/My stores. Replaced with the native
CleanupEIDCertificates export. Cleanup checkboxes now default to
unchecked so an uninstall/reinstall cycle does not destroy enrollments.

Co-Authored-By: Claude Fable 5 <noreply@anthropic.com>
Claude-Session: https://claude.ai/code/session_01ENvebUKMLiPxTZUY5q8bcV
'@
```

---

### Task 4: Push, CI verification, VM test checklist

**Files:**
- Modify: `docs/VM_TEST_PLAN.md` (append uninstall-cleanup test, if the file exists — check first; otherwise skip this file)

**Interfaces:**
- Consumes: the three commits from Tasks 1-3 on `binskim-hardening`.
- Produces: green CI on PR #53; updated VM checklist.

- [ ] **Step 1: Push and watch PR #53 checks**

```powershell
git push origin binskim-hardening
gh pr checks 53 --watch
```

Expected: all checks pass (SonarCloud gate OK, CodeQL 0 new, BinSkim still 10 residuals — the new code inherits Directory.Build.props mitigations automatically).

- [ ] **Step 2: Append the uninstall test to the VM checklist (if `docs/VM_TEST_PLAN.md` exists)**

Append this section verbatim:

```markdown
## Uninstaller certificate cleanup (PR #53)

1. Install the build, enrol a user (creates root CA `CN=EID:<machine>` in
   LocalMachine Root and a user certificate issued by it).
2. Verify presence: `certutil -store root | findstr EID:` shows the CA;
   the enrolled user's My store contains the issued certificate.
3. Uninstall with "Remove EID Root Certificate Authority..." TICKED.
4. Verify: `certutil -store root | findstr EID:` → nothing;
   `certutil -store ca | findstr EID:` → nothing; enrolled user's My store
   has no cert issued by `EID:<machine>` (log on as that user or load the
   hive); `certutil -key | findstr /i <CA container>` → gone.
5. Reinstall, uninstall again with the box UNTICKED → CA cert, user certs
   and key container all survive.
```

- [ ] **Step 3: Commit the checklist (if modified) and report**

```powershell
git add docs/VM_TEST_PLAN.md
git commit -m @'
docs: add uninstaller certificate cleanup to VM test checklist

Co-Authored-By: Claude Fable 5 <noreply@anthropic.com>
Claude-Session: https://claude.ai/code/session_01ENvebUKMLiPxTZUY5q8bcV
'@
git push origin binskim-hardening
```

Then report to the user: PR #53 now needs the VM smart-card logon test RE-RUN (the LSA package DLL changed) plus the new uninstall-cleanup test before release.
