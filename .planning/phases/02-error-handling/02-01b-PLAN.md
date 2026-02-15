---
phase: 02-error-handling
plan: 01b
type: execute
wave: 1
depends_on: []
files_modified:
  - EIDCardLibrary/CompleteToken.cpp
  - EIDCardLibrary/Registration.cpp
  - EIDCardLibrary/smartcardmodule.cpp
  - EIDCardLibrary/TraceExport.cpp
autonomous: true
user_setup: []

must_haves:
  truths:
    - "EIDCardLibrary compiles with zero errors from CompleteToken.cpp, Registration.cpp, smartcardmodule.cpp, TraceExport.cpp"
    - "All string literal to LPWSTR conversions use static wchar_t arrays"
    - "All string literal to WCHAR* conversions use static WCHAR arrays"
    - "Function pointer casts use reinterpret_cast instead of C-style casts"
    - "Remaining const-correctness errors resolved (11 of 23 total)"
  artifacts:
    - path: "EIDCardLibrary/CompleteToken.cpp"
      provides: "Fixed WCHAR* string literal assignment"
      contains: "static WCHAR"
    - path: "EIDCardLibrary/Registration.cpp"
      provides: "Fixed LPWSTR string literal assignments in registry functions"
      contains: "static wchar_t"
    - path: "EIDCardLibrary/smartcardmodule.cpp"
      provides: "Fixed LPWSTR string literal assignments"
      contains: "static wchar_t"
    - path: "EIDCardLibrary/TraceExport.cpp"
      provides: "Fixed function pointer cast"
      contains: "reinterpret_cast<PEVENT_CALLBACK>"
  key_links:
    - from: "EIDCardLibrary/CompleteToken.cpp"
      to: "String literal assignments"
      via: "static WCHAR arrays for non-const WCHAR* parameters"
      pattern: "static WCHAR s_"
    - from: "EIDCardLibrary/Registration.cpp"
      to: "RegSetKeyValue function"
      via: "static wchar_t arrays for string literal values"
      pattern: "static wchar_t s_wsz"
    - from: "EIDCardLibrary/smartcardmodule.cpp"
      to: "String literal assignments"
      via: "static wchar_t arrays for non-const LPWSTR parameters"
      pattern: "static wchar_t s_"
    - from: "EIDCardLibrary/TraceExport.cpp"
      to: "wmistr.h PEVENT_CALLBACK type"
      via: "reinterpret_cast for function pointer conversion"
      pattern: "reinterpret_cast<PEVENT_CALLBACK>"
---

<objective>
Fix const-correctness compile errors in CompleteToken.cpp, Registration.cpp, smartcardmodule.cpp, and TraceExport.cpp to enable C++23 compilation with /Zc:strictStrings flag.

Purpose: The /Zc:strictStrings flag (enabled by default in C++23) enforces strict const-correctness for string literals. String literals are now const char*/const wchar_t* and cannot be implicitly converted to non-const LPSTR/LPWSTR parameters used by Windows APIs. This plan fixes the remaining 11 of 23 compile errors in non-certificate files by creating writable static buffers for string literals and using proper C++ casts for function pointers.

Output: CompleteToken.cpp, Registration.cpp, smartcardmodule.cpp, and TraceExport.cpp compile successfully with C++23, completing const-correctness fixes and enabling dependent projects to link.
</objective>

<execution_context>
@C:/Users/user/.claude/get-shit-done/workflows/execute-plan.md
@C:/Users/user/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/PROJECT.md
@.planning/ROADMAP.md
@.planning/STATE.md
@.planning/phases/02-error-handling/02-RESEARCH.md
@.planning/phases/02-error-handling/02-01a-SUMMARY.md
@.planning/phases/01-build-system/01-03-SUMMARY.md

# Locked decisions from STATE.md
- "Incremental modernization - fix compile errors first, then refactor"
- Phase 1 documented 23 compile errors from /Zc:strictStrings requiring fix

# Research patterns to apply
From 02-RESEARCH.md Pitfall 1: String literal to LPSTR/LPWSTR fix pattern:
```cpp
// BEFORE (compile error):
pwszPointer = L"string";  // const wchar_t* to WCHAR*

// AFTER (fixed):
static WCHAR s_wszString[] = L"string";
pwszPointer = s_wszString;
```

For function pointer casts, use reinterpret_cast instead of C-style casts:
```cpp
// BEFORE (compile warning/error):
ptr = (TargetType)source;

// AFTER (fixed):
ptr = reinterpret_cast<TargetType>(source);
```
</context>

<tasks>

<task type="auto">
  <name>Task 1: Fix CompleteToken.cpp const-correctness (1 error)</name>
  <files>EIDCardLibrary/CompleteToken.cpp</files>
  <action>
Fix 1 const-correctness error in CompleteToken.cpp for WCHAR* string literal assignment.

Line 67: `WCHAR UserName[UNLEN+1];` - array declaration, uses wcsncpy_s which accepts const WCHAR*

The actual error is likely from another string literal assignment. Search for patterns like:
- `(WCHAR*) "string"` assignments
- Direct string literal to WCHAR* parameter

Fix pattern:
```cpp
// BEFORE: Direct string literal assignment
pwszPointer = L"string";  // const wchar_t* to WCHAR*

// AFTER: Use static buffer
static WCHAR s_wszString[] = L"string";
pwszPointer = s_wszString;
```

If the error is from line 67 itself (array initialization), verify wcsncpy_s usage is correct.
  </action>
  <verify>Build EIDCardLibrary and verify CompleteToken.cpp produces zero compile errors</verify>
  <done>All string literal to WCHAR* conversions use static WCHAR arrays</done>
</task>

<task type="auto">
  <name>Task 2: Fix Registration.cpp const-correctness (7 errors)</name>
  <files>EIDCardLibrary/Registration.cpp</files>
  <action>
Fix 7 const-correctness errors in Registration.cpp for string literals in registry functions.

The errors occur in RegSetKeyValue calls (lines 287-346) where string literals like L"EidCredentialProvider", L"EidCredentialProvider.dll", L"Apartment" are passed as LPCWSTR but cast to non-const internally.

RegSetKeyValue signature: `LSTATUS RegSetKeyValueA(HKEY hKey, LPCWSTR lpSubKey, LPCWSTR lpValueName, DWORD dwType, const void* pData, DWORD cbData)`

The 5th parameter is `const void*` but string literals in C++23 are `const wchar_t*` which need explicit handling.

Fix: For each RegSetKeyValue call with a string literal (lines 287-346):
1. Create static wchar_t arrays at file scope (before DllRegister functions):
```cpp
// Registry value string constants (non-const for RegSetKeyValue compatibility)
static wchar_t s_wszEidCredentialProvider[] = L"EidCredentialProvider";
static wchar_t s_wszEidCredentialProviderDll[] = L"EidCredentialProvider.dll";
static wchar_t s_wszApartment[] = L"Apartment";
static wchar_t s_wszEidConfigurationWizard[] = L"EIDConfigurationWizard";
static wchar_t s_wszEidEidConfigurationWizard[] = L"EID.EIDConfigurationWizard";
// ... add all other string literals used in RegSetKeyValue calls
```

2. Replace string literals in RegSetKeyValue calls with static array references

Note: Do NOT modify registry path strings (2nd parameter) - those are LPCWSTR and remain const.
  </action>
  <verify>Build EIDCardLibrary and verify Registration.cpp produces zero compile errors</verify>
  <done>All RegSetKeyValue string literal values use static wchar_t arrays</done>
</task>

<task type="auto">
  <name>Task 3: Fix smartcardmodule.cpp const-correctness (2 errors)</name>
  <files>EIDCardLibrary/smartcardmodule.cpp</files>
  <action>
Fix 2 const-correctness errors in smartcardmodule.cpp for LPWSTR string literal assignments.

Errors likely occur from:
- String literals passed to SCardGetCardTypeProviderName output parameter
- String literal assignments to LPWSTR variables

Search for patterns:
- Line 261: `(LPWSTR) &wszCardModule` - this is output parameter, not input
- Direct string literals assigned to card module name pointers

Fix pattern for string literals to LPWSTR:
```cpp
// BEFORE: Direct string literal
wszPointer = L"CardModuleName";  // const wchar_t* to LPWSTR*

// AFTER: Use static buffer
static wchar_t s_wszCardModuleName[] = L"CardModuleName";
wszPointer = s_wszCardModuleName;
```

Focus on string literals being ASSIGNED to LPWSTR variables, not LPWSTR pointers being PASSED to functions (those are output buffers).
  </action>
  <verify>Build EIDCardLibrary and verify smartcardmodule.cpp produces zero compile errors</verify>
  <done>All string literal to LPWSTR assignments use static wchar_t arrays</done>
</task>

<task type="auto">
  <name>Task 4: Fix TraceExport.cpp const-correctness (1 error)</name>
  <files>EIDCardLibrary/TraceExport.cpp</files>
  <action>
Fix 1 const-correctness error in TraceExport.cpp for function pointer cast.

Line 62: `trace.EventCallback = (PEVENT_CALLBACK) (ProcessEvents);`
The error is C-style cast of function pointer from `VOID WINAPI (PEVENT_TRACE)` to `PEVENT_CALLBACK`.

PEVENT_CALLBACK typedef from wmistr.h may have different calling convention or const-ness under C++23.

Fix: Use reinterpret_cast for function pointer conversion (explicit about type punning):
```cpp
trace.EventCallback = reinterpret_cast<PEVENT_CALLBACK>(ProcessEvents);
```

Alternatively, if ProcessEvents signature exactly matches PEVENT_CALLBACK, verify wmistr.h is included correctly.

Note: TEXT() macros on lines 60, 78, 81, 83 expand to string literals but are passed as const parameters to functions accepting const pointers - no fix needed there.
  </action>
  <verify>Build EIDCardLibrary and verify TraceExport.cpp produces zero compile errors</verify>
  <done>Line 62 function pointer cast uses reinterpret_cast<PEVENT_CALLBACK></done>
</task>

</tasks>

<verification>
1. Build EIDCardLibrary project with C++23 (Debug|x64 configuration)
2. Verify zero compile errors from CompleteToken.cpp, Registration.cpp, smartcardmodule.cpp, TraceExport.cpp
3. Run `grep -E "static (wchar_t|WCHAR) s_" EIDCardLibrary/CompleteToken.cpp EIDCardLibrary/Registration.cpp EIDCardLibrary/smartcardmodule.cpp EIDCardLibrary/TraceExport.cpp` to confirm static array pattern used
4. Verify no string literals directly assigned to non-const pointers
5. Verify all function pointer casts use reinterpret_cast
</verification>

<success_criteria>
1. CompleteToken.cpp compiles with zero errors (1 const-correctness error resolved)
2. Registration.cpp compiles with zero errors (7 const-correctness errors resolved)
3. smartcardmodule.cpp compiles with zero errors (2 const-correctness errors resolved)
4. TraceExport.cpp compiles with zero errors (1 const-correctness error resolved)
5. All string literal to non-const pointer conversions use static array pattern
6. All function pointer casts use reinterpret_cast instead of C-style casts
7. EIDCardLibrary.lib produced successfully for all configurations
8. All 23 const-correctness errors across 02-01a and 02-01b resolved
9. Ready for 02-02 (define error types and Result<T> patterns)
</success_criteria>

<output>
After completion, create `.planning/phases/02-error-handling/02-01b-SUMMARY.md`
</output>
