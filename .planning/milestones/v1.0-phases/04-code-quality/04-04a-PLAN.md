---
phase: 04-code-quality
plan: 04a
type: execute
wave: 2
depends_on: ["04-01", "04-02", "04-03"]
files_modified:
  - EIDCardLibrary/StoredCredentialManagement.cpp
  - EIDCardLibrary/StoredCredentialManagement.h
  - EIDConfigurationWizard/CContainerHolder.cpp
autonomous: true
user_setup: []

must_haves:
  truths:
    - "std::format used in non-LSASS code (EIDConfigurationWizard only)"
    - "QUAL-02 documented as NOT APPLICABLE (no CRTP patterns)"
    - "std::string::contains() pattern documented for future use"
    - "std::span used for internal buffer handling"
    - "No std::format in LSASS components"
    - "No std::span stored in class members"
  artifacts:
    - path: "EIDCardLibrary/StoredCredentialManagement.h"
      provides: "Internal function declarations using std::span"
      contains: "std::span"
    - path: "EIDCardLibrary/StoredCredentialManagement.cpp"
      provides: "Internal function implementations with std::span usage"
      contains: "std::span"
    - path: "EIDConfigurationWizard/CContainerHolder.cpp"
      provides: "std::format usage for error message formatting"
      contains: "#include <format>"
  key_links:
    - from: "Phase 4 completion"
      to: "Phase 4b verification checkpoint"
      via: "All quality improvements verified and ready for user review"
      pattern: "VERIFICATION.*PASSED"
---

<objective>
Verify Phase 4 Code Quality improvements through automated checks

Purpose: Confirm all code quality improvements (std::format, std::span, string utilities) are correctly implemented and compile successfully. This plan runs automated verification tasks to ensure QUAL-01 through QUAL-04 requirements are satisfied.

Output: All requirements verified through automated checks, ready for user review checkpoint
</objective>

<execution_context>
@C:/Users/user/.claude/get-shit-done/workflows/execute-plan.md
@C:/Users/user/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/PROJECT.md
@.planning/ROADMAP.md
@.planning/STATE.md
@.planning/phases/04-code-quality/04-RESEARCH.md
@.planning/REQUIREMENTS.md
@.planning/phases/04-code-quality/04-01-SUMMARY.md
@.planning/phases/04-code-quality/04-02-SUMMARY.md
@.planning/phases/04-code-quality/04-03-SUMMARY.md
</context>

<tasks>

<task type="auto">
  <name>Task 1: Verify QUAL-01 - std::format in non-LSASS code only</name>
  <files></files>
  <action>
1. Verify std::format usage in EIDConfigurationWizard/CContainerHolder.cpp:
   - Confirm `#include <format>` present
   - Confirm std::format used for error message formatting
   - Confirm swprintf_s replaced

2. Verify NO std::format in LSASS components:
   - EIDCardLibrary: grep -r "include <format>" --include="*.cpp" --include="*.h" | should return nothing
   - EIDAuthenticationPackage: grep -r "include <format>" --include="*.cpp" --include="*.h" | should return nothing
   - EIDNativeLogon: grep -r "include <format>" --include="*.cpp" --include="*.h" | should return nothing
   - EIDPasswordChangeNotification: grep -r "include <format>" --include="*.cpp" --include="*.h" | should return nothing
   - EIDCredentialProvider: grep -r "include <format>" --include="*.cpp" --include="*.h" | should return nothing

Expected: 1 file uses std::format (EIDConfigurationWizard), 0 LSASS files use std::format
  </action>
  <verify>grep -r "std::format\|include <format>" --include="*.cpp" "C:/Users/user/Documents/EIDAuthentication/EIDConfigurationWizard" | head -3</verify>
  <done>QUAL-01 verified: std::format in 1 non-LSASS file (EIDConfigurationWizard), 0 LSASS files</done>
</task>

<task type="auto">
  <name>Task 2: Verify QUAL-02 - Deducing This NOT APPLICABLE</name>
  <files>.planning/REQUIREMENTS.md</files>
  <action>
1. Check REQUIREMENTS.md for QUAL-02 status:
   - Confirm marked as "NOT APPLICABLE"
   - Confirm justification references absence of CRTP patterns
   - Confirm documentation is clear

2. Verify no CRTP patterns were added:
   - grep for "template.*class.*Derived" patterns
   - grep for "static_cast.*Derived.*this" patterns
   - Should return ZERO matches (excluding documentation)

Expected: QUAL-02 documented as not applicable, no CRTP code added
  </action>
  <verify>grep -A3 "QUAL-02" "C:/Users/user/Documents/EIDAuthentication/.planning/REQUIREMENTS.md"</verify>
  <done>QUAL-02 verified: Documented as NOT APPLICABLE with clear justification</done>
</task>

<task type="auto">
  <name>Task 3: Verify QUAL-03 - std::string::contains() pattern</name>
  <files></files>
  <action>
1. Note: Codebase grep found NO existing `.find() != npos` patterns to replace
2. Document that std::string::contains() is available for future code:
   - C++23 enables contains() method
   - Pattern: `if (str.contains(substring))` instead of `if (str.find(substring) != std::string::npos)`
   - No existing code needed changes

3. Verify contains() is available by checking compiler support:
   - MSVC 19.31+ (VS 2022 17.1+) supports contains()
   - Project uses /std:c++23preview which includes this feature

Expected: QUAL-03 satisfied - contains() available, no existing code needed changes
  </action>
  <verify>grep -r "\.find.*!=.*npos" --include="*.cpp" "C:/Users/user/Documents/EIDAuthentication/EIDCardLibrary" "C:/Users/user/Documents/EIDAuthentication/EIDConfigurationWizard" "C:/Users/user/Documents/EIDAuthentication/EIDCredentialProvider" || echo "No find/npos patterns found"</verify>
  <done>QUAL-03 verified: std::string::contains() available, no existing patterns to replace</done>
</task>

<task type="auto">
  <name>Task 4: Verify QUAL-04 - std::span for buffer handling</name>
  <files>EIDCardLibrary/StoredCredentialManagement.h EIDCardLibrary/StoredCredentialManagement.cpp</files>
  <action>
1. Verify StoredCredentialManagement.h:
   - Confirm `#include <span>` present
   - Confirm internal functions declared with std::span parameters
   - Confirm ProcessSecretBufferInternal declared

2. Verify StoredCredentialManagement.cpp:
   - Confirm `#include <span>` present
   - Confirm ProcessSecretBufferInternal implemented with std::span
   - Confirm exported functions convert to span and call internal helpers

3. Verify NO std::span in class members:
   - grep for "span.*m_" pattern
   - Should return ZERO matches (lifetime safety)

4. Verify API boundaries unchanged:
   - StorePrivateData signature: PBYTE pbSecret, USHORT usSecretSize (unchanged)
   - StorePrivateDataDebug signature: PBYTE pbSecret, USHORT usSecretSize (unchanged)

Expected: Internal functions use span, exports maintain C-style, no member storage
  </action>
  <verify>grep -n "ProcessSecretBufferInternal\|std::span" "C:/Users/user/Documents/EIDAuthentication/EIDCardLibrary/StoredCredentialManagement.cpp" | head -5</verify>
  <done>QUAL-04 verified: std::span used internally, exports C-style, no member storage</done>
</task>

<task type="auto">
  <name>Task 5: Build verification - all projects compile</name>
  <files></files>
  <action>
1. Build EIDCardLibrary project:
   - msbuild EIDCardLibrary/EIDCardLibrary.vcxproj /p:Configuration=Release /p:Platform=x64
   - Should compile with zero errors

2. Build EIDConfigurationWizard project:
   - msbuild EIDConfigurationWizard/EIDConfigurationWizard.vcxproj /p:Configuration=Release /p:Platform=x64
   - Should compile with zero errors

3. Note: EIDCredentialProvider verification included in Task 1 (LSASS check)
4. Note: Full solution build blocked by pre-existing cardmod.h SDK dependency (not Phase 4 issue)

Expected: All 2 modified projects compile successfully with C++23
  </action>
  <verify>msbuild EIDCardLibrary/EIDCardLibrary.vcxproj /p:Configuration=Release /p:Platform=x64 2>&1 | tail -5</verify>
  <done>Build verification: EIDCardLibrary, EIDConfigurationWizard compile with C++23</done>
</task>

</tasks>

<verification>
1. QUAL-01 verified: std::format in non-LSASS only
2. QUAL-02 verified: NOT APPLICABLE documented
3. QUAL-03 verified: contains() available
4. QUAL-04 verified: std::span used internally
5. Build verification passed
</verification>

<success_criteria>
1. All QUAL-01 through QUAL-04 requirements verified via automated checks
2. Build verification passed for modified projects
3. Ready for user review checkpoint in Plan 04-04b
</success_criteria>

<output>
After completion, create `.planning/phases/04-code-quality/04-04a-SUMMARY.md`
</output>
