---
phase: 03-compile-time-enhancements
plan: 03a
type: execute
wave: 1
depends_on: []
files_modified:
  - EIDCardLibrary/CertificateValidation.h
  - EIDCardLibrary/StoredCredentialManagement.h
autonomous: true

must_haves:
  truths:
    - "if consteval used where compile-time vs runtime code paths differ"
    - "Code compiles without errors after if consteval additions"
    - "if consteval applications are documented with rationale"
  artifacts:
    - path: "EIDCardLibrary/CertificateValidation.h"
      provides: "if consteval usage in validation functions (if applicable)"
      contains: "if consteval"
  key_links:
    - from: "Compile-time path"
      to: "if consteval block"
      via: "Language feature for compile-time detection"
      pattern: "if\\s+consteval\\s*\\{"
---

<objective>
Apply `if consteval` for compile-time vs runtime path differentiation in validation routines.

Purpose: Enable optimized compile-time code paths using C++23's `if consteval` feature. This provides cleaner syntax than `std::is_constant_evaluated()` for distinguishing compile-time from runtime execution. Per research, `if consteval` requires VS 2022 17.14+ (May 2025).

Output: if consteval applied where compile-time vs runtime paths genuinely differ, with clear documentation of why the distinction is beneficial.
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
  <name>Task 1: Identify candidates for if consteval usage</name>
  <files>EIDCardLibrary/CertificateValidation.h, EIDCardLibrary/StoredCredentialManagement.h</files>
  <action>
    Search for functions that could benefit from compile-time vs runtime differentiation:

    1. Look for functions that:
       - Are already constexpr (from 03-02)
       - Have different optimal implementations for compile-time vs runtime
       - Currently use std::is_constant_evaluated() (can be replaced with if consteval)
       - Have simple compile-time paths but more complex runtime paths

    2. Good candidates for if consteval:
       - Validation that could read from policy at runtime but uses constants at compile-time
       - Functions that do stricter checks at compile-time vs runtime
       - Algorithms with different implementations for constant vs variable inputs

    3. Search pattern:
       grep -rn "std::is_constant_evaluated" EIDCardLibrary/
       grep -rn "constexpr.*validate\|constexpr.*check" EIDCardLibrary/*.h

    4. Document findings:
       - List functions that could use if consteval
       - Explain why each would benefit (compile-time path vs runtime path)
       - Note if no good candidates exist (this is acceptable - if consteval is niche)

    Note: Per research, if consteval was added in VS 2022 17.14 (May 2025). Verify toolset supports it.
  </action>
  <verify>grep -rn "std::is_constant_evaluated\|constexpr.*validate" EIDCardLibrary/*.h | head -10</verify>
  <done>Documented list of functions that could benefit from if consteval, or justification if none applicable</done>
</task>

<task type="auto">
  <name>Task 2: Apply if consteval where beneficial</name>
  <files>EIDCardLibrary/CertificateValidation.h, EIDCardLibrary/StoredCredentialManagement.h</files>
  <action>
    For each function identified in Task 1 that would benefit from if consteval:

    1. Modify the function to use if consteval:
       ```cpp
       constexpr bool validate_hash_length(size_t length) noexcept {
           if consteval {
               // Compile-time path: strict constant check
               return length == 32;  // SHA-256 hash length
           } else {
               // Runtime path: could read from config or policy
               return length == 32;  // Or: read from registry/Config
           }
       }
       ```

    2. Use if consteval (NOT if (std::is_constant_evaluated()))
       - if consteval is a statement, not an expression
       - Cleaner syntax: `if consteval { ... } else { ... }`

    3. Only apply where paths genuinely differ:
       - If compile-time and runtime paths are identical, if consteval adds no value
       - Good use case: compile-time uses constants, runtime reads from policy/config
       - Good use case: compile-time does strict validation, runtime is more lenient

    4. If no suitable candidates found, document this and complete the task.

    Safety: if consteval cannot introduce security issues - it only affects WHEN code runs, not WHAT it does.

    5. Add comment explaining the distinction:
       `// Compile-time: strict constant check / Runtime: could read from policy`
  </action>
  <verify>grep -n "if consteval" EIDCardLibrary/CertificateValidation.h EIDCardLibrary/StoredCredentialManagement.h</verify>
  <done>if consteval applied where compile-time vs runtime paths differ, or documented as not applicable</done>
</task>

<task type="auto">
  <name>Task 3: Verify build succeeds with if consteval</name>
  <files>EIDCardLibrary.vcxproj</files>
  <action>
    Build EIDCardLibrary project to verify that if consteval changes compile successfully.

    Command: msbuild EIDCardLibrary/EIDCardLibrary.vcxproj /p:Configuration=Debug /p:Platform=x64 /t:ClCompile

    Expect: Build succeeds. If errors occur:
    - if consteval error: Toolset may be too old (requires VS 2022 17.14+)
    - If toolset doesn't support if consteval, document this and consider alternative approaches

    Verify:
    - No new compile errors introduced
    - if consteval blocks compile correctly (if used)
    - Toolset version is >= 17.14 if if consteval was applied
  </action>
  <verify>msbuild EIDCardLibrary/EIDCardLibrary.vcxproj /p:Configuration=Debug /p:Platform=x64 /t:ClCompile 2>&1 | tail -20</verify>
  <done>Build succeeds with if consteval changes, no new errors</done>
</task>

</tasks>

<verification>
Overall plan verification:

1. if consteval used where compile-time vs runtime paths genuinely differ (or documented as N/A)
2. Build succeeds without errors
3. Toolset supports if consteval (VS 2022 17.14+) if the feature was applied
4. All if consteval uses have explanatory comments
</verification>

<success_criteria>
1. if consteval applied where beneficial, with clear rationale
2. No build regressions
3. Documentation of why if consteval was (or was not) applicable
4. Conservative approach: missing optimizations preferred over complex dual-path code
</success_criteria>

<output>
After completion, create `.planning/phases/03-compile-time-enhancements/03-03a-SUMMARY.md`
</output>
