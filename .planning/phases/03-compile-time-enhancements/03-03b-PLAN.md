---
phase: 03-compile-time-enhancements
plan: 03b
type: execute
wave: 1
depends_on: []
files_modified:
  - EIDCardLibrary/EIDCardLibrary.h
  - EIDCardLibrary/StoredCredentialManagement.cpp
  - EIDCardLibrary/CompleteToken.cpp
autonomous: true

must_haves:
  truths:
    - "std::unreachable() ONLY used on truly unreachable paths (exhaustive switch defaults)"
    - "No std::unreachable on error handling or security validation paths"
    - "Code compiles without errors after std::unreachable additions"
    - "All std::unreachable uses have safety comments"
  artifacts:
    - path: "EIDCardLibrary/EIDCardLibrary.h"
      provides: "std::unreachable() in exhaustive switch statements (if any exist)"
      contains: "std::unreachable"
  key_links:
    - from: "Exhaustive switch default"
      to: "std::unreachable()"
      via: "Optimization hint for impossible code paths"
      pattern: "default:\\s*\\{[^}]*std::unreachable"
---

<objective>
Apply `std::unreachable()` ONLY to provably impossible code paths (exhaustive switch defaults).

Purpose: Provide optimization hints for truly unreachable branches with EXTREME CAUTION. Per research, `std::unreachable()` triggers undefined behavior if executed - must ONLY be used on exhaustive switch defaults with ALL enum cases handled. NEVER on error handling or security validation paths.

Output: std::unreachable() used ONLY on provably unreachable exhaustive switch defaults, documented with safety justification. Conservative approach preferred over optimization.
</objective>

<execution_context>
@C:/Users/user/.claude/get-shit-done/workflows/execute-plan.md
@C:/Users/user/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/PROJECT.md
@.planning/ROADMAP.md
@.planning/STATE.md
@.planning/phases/03-compile-time-enhancements/03-RESEARCH.md
</context>

<tasks>

<task type="auto">
  <name>Task 1: Identify exhaustive switch statements for std::unreachable</name>
  <files>EIDCardLibrary/EIDCardLibrary.h, EIDCardLibrary/StoredCredentialManagement.cpp, EIDCardLibrary/CompleteToken.cpp</files>
  <action>
    Search for switch statements that could benefit from std::unreachable():

    1. Look for switch statements over enum types:
       grep -rn "switch.*EID_" EIDCardLibrary/*.cpp EIDCardLibrary/*.h
       grep -rn "switch.*GPOPolicy\|switch.*CheckType\|switch.*SAMPLE_FIELD" EIDCardLibrary/*.cpp EIDCredentialProvider/*.cpp

    2. For each switch found, verify:
       - Does it handle ALL enum cases explicitly?
       - Is there a default case?
       - Is the default truly unreachable (all cases handled)?

    3. SAFE use of std::unreachable:
       - Exhaustive switch: All enum values have explicit case labels
       - Default case: Only exists to satisfy compiler, never executes
       - No enum coercion: Switch variable is actual enum, not cast from int

    4. DANGEROUS - DO NOT use std::unreachable:
       - Switch statements with missing enum cases
       - Default cases that handle unexpected values
       - Error handling paths (these CAN be reached)
       - Security validation failures (attackers might trigger)
       - Any code depending on external input

    5. Document findings:
       - List switches that are candidates for std::unreachable
       - Explain why each is safe (all enum cases handled)
       - Note if no safe candidates exist

    CRITICAL: If unsure about a switch, DO NOT add std::unreachable. Default to safe behavior.
  </action>
  <verify>grep -rn "switch.*case.*default:" EIDCardLibrary/*.cpp | head -10</verify>
  <done>Documented list of safe exhaustive switches, or justification if none suitable</done>
</task>

<task type="auto">
  <name>Task 2: Apply std::unreachable() to safe exhaustive switches</name>
  <files>EIDCardLibrary/EIDCardLibrary.h, EIDCardLibrary/StoredCredentialManagement.cpp, EIDCardLibrary/CompleteToken.cpp</files>
  <action>
    For each exhaustive switch identified in Task 1 that is provably safe:

    1. Add std::unreachable() to the default case:
       ```cpp
       const char* get_message_name(EID_CALLPACKAGE_MESSAGE msg) noexcept {
           switch (msg) {
               case EID_CALLPACKAGE_MESSAGE::Message:
                   return "Message";
               case EID_CALLPACKAGE_MESSAGE::CardInserted:
                   return "CardInserted";
               case EID_CALLPACKAGE_MESSAGE::CardRemoved:
                   return "CardRemoved";
               default:
                   // SAFE: All enum values handled above
                   std::unreachable();
           }
       }
       ```

    2. CRITICAL SAFETY CHECKS before adding std::unreachable():
       - [ ] ALL enum values have explicit case labels
       - [ ] Enum is scoped (enum class) or unscoped but fully covered
       - [ ] No integer casting to enum type in surrounding code
       - [ ] Default case would NEVER execute in correct usage
       - [ ] NOT in security-critical validation code
       - [ ] NOT in error handling paths

    3. If ANY safety check fails, DO NOT add std::unreachable.
       Use return or break in default case instead.

    4. Add comment explaining WHY unreachable is safe:
       `// All enum cases handled above - this path is unreachable`

    5. If no safe switches found, document this and complete the task.

    Remember: std::unreachable() triggers undefined behavior if executed. In security-critical LSASS code, prefer safe defaults over optimization hints.
  </action>
  <verify>grep -n "std::unreachable" EIDCardLibrary/*.h EIDCardLibrary/*.cpp 2>/dev/null | head -5</verify>
  <done>std::unreachable() applied ONLY to exhaustive switches with all cases handled, or documented as not applicable</done>
</task>

<task type="auto">
  <name>Task 3: Verify build succeeds with std::unreachable</name>
  <files>EIDCardLibrary.vcxproj</files>
  <action>
    Build EIDCardLibrary project to verify that std::unreachable changes compile successfully.

    Command: msbuild EIDCardLibrary/EIDCardLibrary.vcxproj /p:Configuration=Debug /p:Platform=x64 /t:ClCompile

    Expect: Build succeeds. If errors occur:
    - std::unreachable error: May be missing <utility> header (added in 03-01)

    Verify:
    - No new compile errors introduced
    - std::unreachable() is recognized (if used)
    - <utility> header is included from 03-01
  </action>
  <verify>msbuild EIDCardLibrary/EIDCardLibrary.vcxproj /p:Configuration=Debug /p:Platform=x64 /t:ClCompile 2>&1 | tail -20</verify>
  <done>Build succeeds with std::unreachable changes, no new errors</done>
</task>

</tasks>

<verification>
Overall plan verification:

1. std::unreachable() ONLY used in exhaustive switch defaults with all enum cases handled (or documented as N/A)
2. No std::unreachable() on error handling or security validation paths
3. Build succeeds without errors
4. <utility> header is included (from 03-01) if std::unreachable is used
5. All std::unreachable uses have safety comments
6. Conservative approach: safe defaults preferred over optimization
</verification>

<success_criteria>
1. std::unreachable used ONLY on provably unreachable paths, with safety justification
2. No std::unreachable on security-critical or error-handling paths
3. Build succeeds without errors
4. Conservative approach: missing optimizations preferred over unsafe unreachable
5. Clear documentation of why std::unreachable was (or was not) applicable
</success_criteria>

<output>
After completion, create `.planning/phases/03-compile-time-enhancements/03-03b-SUMMARY.md`
</output>
