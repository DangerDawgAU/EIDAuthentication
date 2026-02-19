# Pitfalls Research

**Domain:** SonarQube Issue Remediation in Windows LSASS Security-Critical Codebase
**Researched:** 2026-02-18
**Confidence:** HIGH (based on codebase analysis, prior milestone experience, and Windows security documentation)

---

## Critical Pitfalls

Mistakes that cause rewrites, security vulnerabilities, or major deployment failures.

### Pitfall 1: Converting C-Style Stack Arrays to std::string in LSASS Code

**What goes wrong:**
SonarQube flags 149 instances of "Use std::string instead of C-style char array" and 28 instances of "Use std::array/std::vector instead of C-style array." Blindly converting these to `std::string` or `std::vector` can cause LSASS crashes because:
- `std::string` uses dynamic heap allocation
- LSASS has strict memory constraints and custom allocators
- Dynamic allocation failures in LSASS can crash the entire OS security subsystem
- The codebase already uses custom `EIDAlloc`/`EIDFree` wrappers for LSASS-safe allocation

**Why it happens:**
SonarQube does not understand LSASS context. The rule is designed for general C++ code, not for system processes where stack allocation is intentional for safety and reliability.

**How to avoid:**
1. **Never convert** C-style arrays in EIDAuthenticationPackage (LSASS-loaded code) to `std::string`
2. C-style arrays in LSASS code are **intentional** - they guarantee stack allocation
3. Files like `EIDCardLibrary/StoredCredentialManagement.cpp` contain 84 issues - most are won't-fix
4. Only consider `std::array` (stack-allocated) as a safe alternative, never `std::vector` or `std::string`

**Warning signs:**
- Change introduces `new` or `malloc` in authentication flow
- `std::string` appears in EIDAuthenticationPackage code
- Dynamic allocation in SEH-protected blocks

**Phase to address:** All phases - this is a constraint, not a fix target

**Rollback strategy:** If a std::string conversion causes issues, immediately revert to original C-style array and document as won't-fix.

---

### Pitfall 2: Marking Global Variables as const When They're Runtime-Assigned

**What goes wrong:**
SonarQube flags 71 "Global variables should be const" and 31 "Global pointers should be const at every level" issues. Marking runtime-assigned globals as `const` causes:
- Compilation failures (cannot assign to const)
- Broken LSA initialization (function pointers assigned by `SetAlloc`/`SetFree`/`SetImpersonate`)
- Tracing state corruption (`IsTracingEnabled` changes at runtime)
- COM registration failures (reference counts, instance handles)

**Why it happens:**
v1.3 Phase 23 already documented 32 globals as legitimately mutable. The SonarQube rule doesn't understand that some globals are designed to be assigned once at initialization and then read-only during operation.

**How to avoid:**
Before marking any global as `const`:
1. Check if it's assigned by a `Set*()` function (LSA function pointers)
2. Check if it's modified in `EnableCallback` (tracing state)
3. Check if it's a Windows API output buffer (CryptoAPI requires non-const)
4. Check if it's set during DLL initialization (`DllGetClassObject`, `DllRegisterServer`)

**Won't-fix categories from v1.3 Phase 23:**
| Category | Count | Example |
|----------|-------|---------|
| LSA Function Pointers | 3 | `MyAllocateHeap`, `MyFreeHeap`, `MyImpersonate` |
| Tracing State | 4 | `IsTracingEnabled`, `hPub`, `bFirst` |
| DLL State | 2 | `g_cRef`, `g_hinst` |
| UI State | 6 | `szReader`, `szCard`, `szUserName`, `szPassword` |
| Windows API Buffers | 14 | OID strings for `CERT_ENHKEY_USAGE` |

**Warning signs:**
- Adding `const` to a pointer declared `extern` without checking definition
- Global has a `Set*` setter function
- Global is modified in `EnableCallback`, `DllMain`, or initialization code

**Phase to address:** Issue review phases - identify won't-fix upfront

**Rollback strategy:** Remove const qualifier, add comment explaining runtime assignment pattern.

---

### Pitfall 3: Refactoring SEH-Protected Code Breaks Exception Safety

**What goes wrong:**
The codebase has 215+ `__try`/`__except`/`__finally` blocks. Extracting code from SEH blocks into helper functions breaks structured exception handling because:
- SEH only works within a single function scope
- Extracted code is no longer protected by the `__except` filter
- Access violations in extracted helpers crash LSASS (BSOD potential)
- `__finally` cleanup won't run for extracted code

**Why it happens:**
Nesting depth reduction (Phase 24) and cognitive complexity reduction (Phase 25) goals can tempt refactoring of SEH-protected code. The relationship between code location and exception protection is not obvious.

**How to avoid:**
1. Never extract code from inside `__try` blocks
2. `__try`/`__except`/`__finally` structures must remain intact
3. Nesting reduction in SEH code should use early return/guard clauses only
4. Document SEH-protected functions as won't-fix for nesting/cognitive complexity

**Files with most SEH usage:**
- `EIDCardLibrary/StoredCredentialManagement.cpp` - 42 SEH blocks
- `EIDAuthenticationPackage/EIDAuthenticationPackage.cpp` - 14 SEH blocks
- `EIDCardLibrary/Package.cpp` - 12 SEH blocks
- `EIDCardLibrary/Registration.cpp` - 10 SEH blocks

**Warning signs:**
- Refactoring touches `__try`, `__except`, `__finally` keywords
- Helper function created inside SEH block
- Error handling path split across functions

**Phase to address:** All refactoring phases - this is a constraint

**Rollback strategy:** Restore SEH block structure immediately; document as won't-fix with SEH safety justification.

---

### Pitfall 4: Adding [[fallthrough]] Incorrectly to Switch Statements

**What goes wrong:**
SonarQube's blocker issue (1 instance) flags "Unannotated fall-through between switch labels." Adding `[[fallthrough]]` without understanding the code can:
- Mask logic bugs where fall-through was unintentional
- Hide missing break statements that should exist
- Create security vulnerabilities if authentication cases fall through

**Why it happens:**
The `[[fallthrough]]` attribute (C++17) explicitly indicates intentional fall-through. Without analysis, you might add it to cases where the original code had a bug.

**How to avoid:**
1. Analyze the switch case logic before adding `[[fallthrough]]`
2. Confirm the fall-through is intentional (related cases grouped together)
3. If fall-through is a bug, add `break` instead of `[[fallthrough]]`
4. The blocker issue in `EIDConfigurationWizardPage06.cpp:44` needs careful review

**Warning signs:**
- Switch cases have completely unrelated logic
- No comment indicating intentional fall-through
- Different error handling in consecutive cases

**Phase to address:** First phase (blocker fix)

**Rollback strategy:** Remove `[[fallthrough]]` and add `break` if analysis shows it was a bug.

---

### Pitfall 5: Breaking Windows API Const-Correctness Requirements

**What goes wrong:**
Windows APIs often require non-const pointers even when they don't modify the data. Adding `const` to match SonarQube's const-correctness rules breaks Windows API calls:
- `LSA_UNICODE_STRING` buffers must be non-const
- `SecBuffer` descriptors require mutable pointers
- `CERT_ENHKEY_USAGE.rgpszUsageIdentifier` requires `LPSTR*` (non-const)
- Many Win32 APIs use `LPTSTR` out-params even for input

**Why it happens:**
Windows API design predates modern C++ const-correctness. Many APIs were designed for C compatibility and use non-const for flexibility. SonarQube doesn't understand Windows API contracts.

**How to avoid:**
1. Check MSDN documentation before adding const to Windows API parameters
2. Use `const_cast` sparingly and document why
3. Don't change function signatures for Windows callbacks
4. Document Windows API const requirements as won't-fix

**Known won't-fix patterns:**
- `LsaInitializeString` - `PLSA_STRING` is non-const
- `CertOpenStore` - provider parameters are non-const
- `SCardEstablishContext` - context handles are non-const pointers

**Warning signs:**
- Adding `const` to a pointer passed to Windows API
- Function is a Windows callback (WNDPROC, etc.)
- Parameter type is a Windows structure pointer

**Phase to address:** Const-correctness review phases

**Rollback strategy:** Remove const qualifier, add comment referencing Windows API requirement.

---

### Pitfall 6: Converting Macros Used in Resource Files to constexpr

**What goes wrong:**
SonarQube flags 111 "Replace macro with const, constexpr or enum" issues. Converting macros used in `.rc` resource files to `constexpr` breaks the resource compiler:
- `RC.exe` cannot process C++ constexpr
- Build fails with "undefined identifier" in resource files
- Version information macros like `EIDAuthenticateVersionText` must be `#define`

**Why it happens:**
v1.3 Phase 22-03 already discovered this - `EIDAuthenticateVersionText` was converted to constexpr and had to be reverted because the resource compiler only understands preprocessor macros.

**How to avoid:**
1. Check if macro is used in any `.rc` file before converting
2. Resource ID macros (~50 in this codebase) must remain as `#define`
3. Version string macros must remain as `#define`
4. Only convert macros that are purely C++ code constants

**Won't-fix macro categories from v1.3 Phase 22:**
| Category | Count | Reason |
|----------|-------|--------|
| Windows Header Macros | ~25 | Configure SDK behavior |
| Function-Like Macros | ~10 | Use `#`, `##`, `__FILE__`, `__LINE__` |
| Resource ID Macros | ~50 | Required by RC.exe |
| Version String Macros | 1+ | Required by version resources |

**Warning signs:**
- Macro name starts with `IDC_`, `IDD_`, `IDS_` (resource IDs)
- Macro contains `__FILE__`, `__LINE__`, `__FUNCTION__`
- Macro used in `STRINGTABLE`, `VERSIONINFO` resource blocks

**Phase to address:** Macro conversion phases

**Rollback strategy:** Revert to `#define`, document resource compiler limitation.

---

## Moderate Pitfalls

### Pitfall 7: Over-Extracting Helper Functions for Cognitive Complexity

**What goes wrong:**
Extracting too many helper functions for cognitive complexity reduction leads to:
- Code scattered across many functions, harder to understand as a whole
- Context loss - helpers need many parameters to access needed state
- Non-idiomatic Windows code (dialog procedures should be self-contained)
- Testing complexity increases

**Why it happens:**
SonarQube's cognitive complexity metric doesn't account for:
- Security-critical explicit flow (validation chains)
- Windows message handler idioms
- State machine patterns

**How to avoid:**
1. Only extract when helper has clear single responsibility
2. Limit helpers to 3-4 parameters maximum
3. Keep message handlers together in their original function
4. Document complex validation chains as won't-fix (security-relevant)

**Won't-fix categories from v1.3 Phases 24-25:**
- SEH-Protected Code - Exception safety
- Primary Authentication Functions - Security-critical
- Complex State Machines - Deliberate design
- Crypto Validation Chains - Explicit security flow
- Windows Message Handlers - Idiomatic pattern

**Warning signs:**
- Helper function named "HandleCase1", "DoTheWork", "Helper"
- Helper needs 5+ parameters
- Message handler split across 3+ functions

**Phase to address:** Cognitive complexity phases

---

### Pitfall 8: Changing Error Return Semantics

**What goes wrong:**
Modifying error handling while refactoring can change:
- `HRESULT` to `NTSTATUS` conversion behavior
- Error codes returned to calling code
- `SetLastError` values that callers depend on
- LSA dispatch table expectations

**Why it happens:**
The codebase uses mixed error types (HRESULT, NTSTATUS, BOOL, DWORD) for different contexts. Refactoring can accidentally change error propagation.

**How to avoid:**
1. Preserve exact return types at function boundaries
2. Don't change `return FALSE` to `return E_FAIL` without checking callers
3. Keep LSA entry point return types as `NTSTATUS`
4. Keep credential provider entry points as `HRESULT`
5. Use `HRESULT_FROM_NT` / `HRESULT_FROM_WIN32` for conversions

**Warning signs:**
- Changing function return type
- Adding new error return paths
- Converting between HRESULT and NTSTATUS

**Phase to address:** All refactoring phases

---

### Pitfall 9: Breaking DLL Load Order Dependencies

**What goes wrong:**
Modifying global initialization order can break:
- LSA function pointer assignment (must happen before any allocation)
- ETW tracing registration (must happen before first trace)
- Critical section initialization (must happen before first use)

**Why it happens:**
Globals are initialized in undefined order across translation units. The current code may rely on specific initialization order that's fragile.

**How to avoid:**
1. Don't change where `SetAlloc`/`SetFree`/`SetImpersonate` are called
2. Keep `DllMain` initialization sequence unchanged
3. Don't move critical section initialization
4. Test DLL loading in isolation after any initialization changes

**Warning signs:**
- Moving initialization code to different functions
- Adding new globals that need initialization
- Changing `DllMain` entry point

**Phase to address:** Any phase touching initialization code

---

### Pitfall 10: Auto Type Deduction Breaking Windows API Types

**What goes wrong:**
Using `auto` for Windows API types can cause:
- Wrong type deduction (HANDLE vs. PHANDLE)
- Pointer level confusion
- Implicit conversion hiding
- Compile errors with Windows typedefs

**Why it happens:**
Windows uses many typedef aliases. `auto` deduces the underlying type, which may lose typedef semantics.

**How to avoid:**
1. Use `auto` primarily for iterator declarations (v1.3 Phase 21 pattern)
2. Keep explicit types for Windows handles and pointers
3. Don't use `auto` for function parameters or return types
4. Verify deduced type matches expected Windows type

**Safe auto usage (from v1.3 Phase 21):**
```cpp
// SAFE: Iterator declarations
auto iter = certificates.begin();
auto it = m_mapCredentials.find(hash);

// UNSAFE: Windows handles
auto hToken = GetCurrentToken(); // Could be HANDLE or PHANDLE
```

**Warning signs:**
- `auto` used with Windows HANDLE, HWND, HKEY types
- `auto` for function return types in header files
- `auto` deducing pointer-to-pointer

**Phase to address:** Style modernization phases

---

## Minor Pitfalls

### Pitfall 11: Adding Comments to Empty Compound Statements

**What goes wrong:**
SonarQube flags "Fill compound statement or add nested comment" (17 issues). Adding comments to intentional empty blocks can:
- Obscure that the block is intentionally empty
- Add noise without value
- Miss the real issue (should the block be empty?)

**How to avoid:**
1. Use `; // intentional` or `; // no action required` for truly empty blocks
2. Consider if empty block indicates missing error handling
3. Don't add verbose comments that add no information

---

### Pitfall 12: Merging Nested If Statements Blindly

**What goes wrong:**
SonarQube flags "Merge if statement with enclosing one" (17 issues). Merging can:
- Lose short-circuit evaluation benefits
- Change evaluation order (side effects)
- Make code less readable with complex conditions

**How to avoid:**
1. Merge only when both conditions are simple
2. Keep short-circuit evaluation explicit when order matters
3. Don't merge if debugging needs to distinguish which condition failed

---

## Technical Debt Patterns

Shortcuts that seem reasonable but create long-term problems.

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Mark all globals const | Reduces SonarQube count | Compilation failures, broken initialization | Never - requires analysis |
| Convert all C arrays to std::string | Cleaner code | LSASS crashes from dynamic allocation | Never in LSASS-loaded code |
| Disable SEH for refactoring | Easier extraction | Access violations crash OS | Never |
| Suppress warnings with #pragma | Quick fix | Hides real issues | Only for external headers |
| Use // NOSONAR everywhere | Fast cleanup | Hides real issues, audit trail lost | Only with documented justification |

---

## Integration Gotchas

Common mistakes when connecting to external services/APIs.

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| LSA Dispatch Table | Assuming functions are available | Check function pointers are non-null before use |
| ETW Tracing | Registering in DllMain | Defer registration to first use or dedicated init |
| CryptoAPI | Using const with CERT_ structures | Keep non-const as Windows API requires |
| Smart Card API | Not checking SCard return values | All SCard functions need explicit error handling |
| Credential Provider | Throwing exceptions across COM boundary | Use HRESULT error codes only |

---

## Performance Traps

Patterns that work at small scale but fail as usage grows.

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Large stack buffers in recursion | Stack overflow | Keep buffers <1KB in recursive code | Deep call chains |
| Certificate validation on every auth | Slow login | Consider caching validation results (future) | Many concurrent logins |
| Tracing every operation | ETW buffer overflow | Use trace levels appropriately | High-frequency operations |

---

## Security Mistakes

Domain-specific security issues for LSASS authentication code.

| Mistake | Risk | Prevention |
|---------|------|------------|
| std::string for credential data | Heap-based credential exposure | Use stack buffers, SecureZeroMemory after use |
| Exception crossing LSASS boundary | LSASS crash (BSOD) | Error codes only, no exceptions |
| Logging sensitive data | Credential exposure in logs | Never log PINs, passwords, or key material |
| Not clearing credential buffers | Memory forensics credential theft | SecureZeroMemory all credential buffers |
| Trusting certificate without validation | Authentication bypass | Full chain validation always |
| Ignoring smart card removal | Session hijacking | Handle card removal events |
| Timing attacks on PIN validation | PIN enumeration | Consider constant-time comparison |

---

## "Looks Done But Isn't" Checklist

Things that appear complete after SonarQube remediation but are missing critical pieces.

- [ ] **Build succeeds:** But LSASS DLL fails to load - verify DLL registration and LSA package loading
- [ ] **SonarQube shows zero issues:** But won't-fix issues not documented - ensure justifications in analysis
- [ ] **Tests pass:** But edge cases in authentication changed - manual smoke test required
- [ ] **Code compiles:** But resource files broken - verify .rc compilation succeeds
- [ ] **No new warnings:** But behavior changed - test credential provider UI and auth flow
- [ ] **Local VM works:** But production fails - test on clean Windows install
- [ ] **Debug build works:** But Release fails - verify Release configuration builds

---

## Recovery Strategies

When pitfalls occur despite prevention, how to recover.

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| std::string in LSASS | HIGH | Revert all std::string changes, restore C arrays, full test cycle |
| Broken SEH safety | HIGH | Restore original SEH structure, document as won't-fix |
| Changed error semantics | MEDIUM | Diff return types against baseline, restore exact types |
| Macro in .rc file | LOW | Revert to #define, document resource compiler limitation |
| Global const failure | LOW | Remove const, add runtime-assignment comment |
| Auto type confusion | LOW | Restore explicit type, document Windows API requirement |
| [[fallthrough]] bug | LOW | Remove fallthrough, add break statement |
| Empty block comment | TRIVIAL | Remove or improve comment |

---

## Phase-Specific Warnings

| Phase Topic | Likely Pitfall | Mitigation |
|-------------|---------------|------------|
| Blocker fix ([[fallthrough]]) | Adding fallthrough to bug | Analyze case logic before adding |
| Global const issues | Marking runtime-assigned as const | Check for Set* functions |
| C-style array issues | Converting to std::string | Keep as C arrays in LSASS code |
| Nesting reduction | Extracting from SEH blocks | Keep SEH structures intact |
| Cognitive complexity | Over-extracting helpers | Limit helpers, keep security flow explicit |
| Macro conversion | Breaking resource compiler | Check .rc files before converting |
| std::array conversion | Using std::vector instead | Only std::array is stack-safe |

---

## Pitfall-to-Phase Mapping

How roadmap phases should address these pitfalls.

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| std::string in LSASS | All phases (constraint) | grep for std::string in EIDAuthenticationPackage |
| Global const issues | Issue triage phase | Check for Set*/EnableCallback patterns |
| SEH safety | All refactoring phases | grep for __try changes, diff carefully |
| [[fallthrough]] blocker | First phase | Manual review of switch logic |
| Windows API const | Const-correctness phases | Check MSDN for API requirements |
| Macro in .rc files | Macro conversion phases | grep .rc files for macro usage |
| Over-extraction | Complexity reduction phases | Count helper function parameters |
| Error semantics | All refactoring phases | Diff function signatures |
| DLL load order | Initialization changes | Test DLL load in isolation |
| Auto type confusion | Style modernization phases | Verify deduced type with static_assert |

---

## Sources

### Codebase Documentation (HIGH confidence)
- `.planning/sonarqube-analysis.md` - Issue categorization and counts
- `.planning/STATE.md` - Key constraints (LSASS context, Windows 7 support, no exceptions)
- `.planning/codebase/CONCERNS.md` - Known tech debt and security considerations
- `.planning/milestones/v1.3-phases/30-final-sonarqube-scan/30-QUALITY-SUMMARY.md` - Prior milestone achievements

### Prior Research (HIGH confidence)
- `.planning/research/PITFALLS.md` - C++14 to C++23 upgrade pitfalls
- `.planning/milestones/v1.3-phases/24-sonarqube-nesting-issues/24-RESEARCH.md` - Nesting reduction patterns

### Windows/LSASS Documentation (MEDIUM confidence)
- Microsoft Learn - LSA Authentication documentation
- Microsoft Learn - Structured Exception Handling (SEH)
- Windows SDK documentation for CryptoAPI, Smart Card API

### Community Knowledge (MEDIUM confidence)
- C++ Core Guidelines for security-critical code
- Windows driver development patterns (LSASS shares similar constraints)

---

## Appendix: Quick Reference - Won't Fix Categories

Based on v1.3 milestone analysis, these SonarQube issue categories should be documented as won't-fix:

| Issue Type | Count | Category | Justification |
|------------|-------|----------|---------------|
| Global variables const | 71 | ~67 won't-fix | Runtime assignment, LSA pointers, tracing state |
| Global pointers const | 31 | ~28 won't-fix | Windows API requirements, DLL state |
| C-style char array | 149 | ~140 won't-fix | Stack allocation required in LSASS |
| C-style array | 28 | ~25 won't-fix | Stack allocation for safety |
| Macro to constexpr | 111 | ~107 won't-fix | Resource compiler, function-like macros |
| Nesting depth | 52 | ~33 won't-fix | SEH safety, security-critical flow |
| Cognitive complexity | varies | ~21-30 won't-fix | SEH, state machines, crypto chains |

**Expected actual fixes in v1.4:** ~730 issues documented, ~50-100 actionable, ~600+ won't-fix with justifications

---

*Pitfalls research for: SonarQube Issue Remediation in Windows LSASS Security-Critical Codebase*
*Researched: 2026-02-18*
