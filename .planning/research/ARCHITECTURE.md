# Architecture Research

**Domain:** SonarQube Issue Remediation Architecture for C++23 Windows LSASS Codebase
**Researched:** 2026-02-18
**Confidence:** HIGH

## Executive Summary

This research addresses how to organize remediation of ~730 remaining SonarQube issues in the EIDAuthentication Windows smart card authentication package. The existing codebase is already on C++23 with MSVC v143, all 7 projects compile successfully, and security issues are resolved. The focus is purely on maintainability improvements while preserving LSASS safety constraints.

---

# SUBSEQUENT MILESTONE: VirusTotal CI/CD Integration

**Domain:** VirusTotal Scanning for Release Artifacts
**Researched:** 2026-02-19
**Confidence:** HIGH

## VirusTotal Integration Architecture

### System Overview

```
+------------------------------------------------------------------+
|                    GitHub Actions Workflow                        |
+------------------------------------------------------------------+
|                                                                   |
|  +-------------+    +----------------+    +-------------------+  |
|  |   Build     |    |    Package     |    |   VirusTotal      |  |
|  |   Job       |--->|    (NSIS)      |--->|   Scan Job        |  |
|  |             |    |                |    |                   |  |
|  | - Checkout  |    | - Installer    |    | - Upload artifact |  |
|  | - Setup     |    |   creation     |    | - Poll analysis   |  |
|  | - Build     |    | - Artifact     |    | - Report results  |  |
|  |             |    |   generation   |    | - Update release  |  |
|  +-------------+    +----------------+    +-------------------+  |
|                                                                   |
+------------------------------------------------------------------+
|                         External Services                         |
+------------------------------------------------------------------+
|  +------------------+         +------------------+                |
|  |   VirusTotal     |         |   GitHub         |                |
|  |   API v3         |<------->|   Release API    |                |
|  |                  |         |                  |                |
|  | - File upload    |         | - Update body    |                |
|  | - Analysis       |         | - Attach links   |                |
|  | - Reports        |         |                  |                |
|  +------------------+         +------------------+                |
+------------------------------------------------------------------+
```

### Component Responsibilities

| Component | Responsibility | Typical Implementation |
|-----------|----------------|------------------------|
| Build Job | Compile Windows credential provider DLL | MSBuild on windows-latest runner |
| Package Step | Create NSIS installer executable | joncloud/makensis-action |
| VirusTotal Scan Job | Upload and scan artifacts with VT | crazy-max/ghaction-virustotal@v4 |
| Release Update | Append scan results to GitHub release | Built-in action output handling |
| Secrets Management | Store VT API key securely | GitHub repository secrets |

### Recommended Project Structure

```
.github/
+-- workflows/
|   +-- windows-build.yaml      # EXISTING: Build and release workflow
|                                  (MODIFY: Add VT scan job)
+-- scripts/                    # Optional: Helper scripts for scan logic
|   +-- check-vt-results.ps1    # Parse VT results, fail on detections
```

### Structure Rationale

- **Integrate into existing workflow:** For EIDAuthentication, integrating into the existing `windows-build.yaml` is recommended because the scan should only run on releases, and the artifact is already available
- **No new source files required:** The VirusTotal integration is purely CI/CD configuration

## Architectural Patterns

### Pattern 1: Post-Build Scan (Recommended for Release Workflow)

**What:** Add VirusTotal scan as a separate job that runs after successful artifact generation
**When to use:** Release workflows where you want to scan final artifacts before/during publication
**Trade-offs:**
- Pros: Does not block builds, scans final packaged artifact, can update release notes
- Cons: Requires artifact passing between jobs, adds time to release process

**Example:**

```yaml
name: Build on windows.
on:
  release:
    types: [published]

jobs:
  build:
    # EXISTING BUILD JOB - no changes needed
    runs-on: windows-latest
    permissions:
      contents: write
    steps:
      - name: Check out repo
        uses: actions/checkout@v4

      - name: Add msbuild to PATH
        uses: microsoft/setup-msbuild@v2
        with:
          msbuild-architecture: x64

      - name: Build app for release
        run: msbuild EIDCredentialProvider.sln -t:rebuild -property:Configuration=Release /p:Platform=x64

      - name: Create nsis installer
        uses: joncloud/makensis-action@v1
        with:
          script-file: Installer/Installerx64.nsi
          arguments: "/V3"

      - name: Upload artifact
        uses: actions/upload-artifact@v4
        with:
          name: installer
          path: Installer/EIDInstallx64.exe

      - name: upload binaries to release
        uses: softprops/action-gh-release@v2
        if: ${{ startsWith(github.ref, 'refs/tags/') }}
        with:
          files: Installer/EIDInstallx64.exe

  virustotal:
    name: VirusTotal Scan
    needs: build
    runs-on: ubuntu-latest
    permissions:
      contents: write
    steps:
      - name: Download artifact
        uses: actions/download-artifact@v4
        with:
          name: installer
          path: ./artifacts

      - name: VirusTotal Scan
        uses: crazy-max/ghaction-virustotal@v4
        with:
          vt_api_key: ${{ secrets.VT_API_KEY }}
          github_token: ${{ github.token }}
          update_release_body: true
          request_rate: 4
          files: |
            ./artifacts/EIDInstallx64.exe
```

### Pattern 2: Standalone Scan Workflow

**What:** Separate workflow triggered by release events or manually
**When to use:** When you want to scan without modifying the main build workflow, or for on-demand scanning
**Trade-offs:**
- Pros: Decoupled from build, can be triggered independently
- Cons: Requires download of release assets, more complex file handling

**Example:**

```yaml
name: VirusTotal Scan

on:
  release:
    types: [published]
  workflow_dispatch:

permissions:
  contents: write

jobs:
  virustotal:
    runs-on: ubuntu-latest
    steps:
      - name: VirusTotal Scan
        uses: crazy-max/ghaction-virustotal@v4
        with:
          vt_api_key: ${{ secrets.VT_API_KEY }}
          github_token: ${{ github.token }}
          update_release_body: true
          request_rate: 4
          files: |
            .exe$
```

### Pattern 3: Fail-on-Detection Pattern

**What:** Parse scan results and fail workflow if detections exceed threshold
**When to use:** When you want to block releases with significant AV detections
**Trade-offs:**
- Pros: Prevents publishing potentially flagged software
- Cons: False positives can block legitimate releases

**Example:**

```yaml
- name: VirusTotal Scan
  id: vt-scan
  uses: crazy-max/ghaction-virustotal@v4
  with:
    vt_api_key: ${{ secrets.VT_API_KEY }}
    files: ./artifacts/EIDInstallx64.exe

- name: Check for detections
  run: |
    ANALYSIS="${{ steps.vt-scan.outputs.analysis }}"
    # Parse analysis URL and fetch results
    # Implement threshold logic (e.g., fail if > 5 detections)
    echo "Analysis results: $ANALYSIS"
```

## Data Flow

### Release Scan Flow

```
[Release Published]
        |
        v
[Build Job] --> [Compile DLL] --> [Create NSIS Installer] --> [Upload to Release]
        |                                                        |
        +--> [Upload Artifact] <---------------------------------+
                                        |
                                        v
                              [VT Scan Job downloads artifact]
                                        |
                                        v
                              [Upload to VirusTotal API]
                                        |
                                        v
                              [Poll for analysis completion]
                                        |
                                        v
                              [Update release body with VT link]
```

### Key Data Flows

1. **Artifact Flow:** Build job creates `EIDInstallx64.exe` -> upload-artifact -> VT scan job downloads
2. **API Communication:** VT action uploads file -> VirusTotal queues analysis -> action polls for completion -> returns analysis URL
3. **Release Update:** Analysis URL written to release body via GitHub API using `update_release_body: true`

## Integration Points

### External Services

| Service | Integration Pattern | Notes |
|---------|---------------------|-------|
| VirusTotal API v3 | REST API via ghaction-virustotal | Public API has 4 req/min rate limit; set `request_rate: 4` |
| GitHub Releases API | Built into action with `github_token` | Requires `contents: write` permission |

### Integration with Existing EIDAuthentication Workflow

| Existing Component | Integration Point | Modification Required |
|-------------------|-------------------|----------------------|
| `windows-build.yaml` | Add `needs: build` job dependency | Add new `virustotal` job |
| `softprops/action-gh-release` | Artifact already uploaded | No change - artifact reused |
| Build permissions | Add `contents: write` to VT job | New permission declaration |
| Secrets | Add `VT_API_KEY` repository secret | One-time setup |

### New vs Modified Components

| Component | Type | Description |
|-----------|------|-------------|
| `VT_API_KEY` secret | NEW | Repository secret containing VirusTotal API key |
| `virustotal` job | NEW | Job definition in windows-build.yaml |
| `actions/download-artifact@v4` | NEW | Step to download build artifact |
| `crazy-max/ghaction-virustotal@v4` | NEW | VT scan action |
| `needs: build` | NEW | Job dependency declaration |

## Anti-Patterns

### Anti-Pattern 1: Scanning Source Code Instead of Artifacts

**What people do:** Point VT scanner at source directories or build intermediates
**Why it's wrong:** AV engines detect compiled binaries differently; source code may have different signatures
**Do this instead:** Always scan the final packaged artifact (the NSIS installer in this case)

### Anti-Pattern 2: Blocking Build on Scan Results

**What people do:** Fail the entire workflow if any VT detection is found
**Why it's wrong:** Credential providers often trigger false positives due to legitimate security-sensitive operations
**Do this instead:** Report results informatively, allow manual review before taking action

### Anti-Pattern 3: Not Setting Rate Limits

**What people do:** Use default `request_rate: 0` with free API key
**Why it's wrong:** Exceeds free tier quota (4 requests/minute), causes 429 errors
**Do this instead:** Always set `request_rate: 4` when using public VirusTotal API

### Anti-Pattern 4: Exposing API Key

**What people do:** Hardcode VT API key in workflow file
**Why it's wrong:** API key visible in git history, can be abused
**Do this instead:** Use `${{ secrets.VT_API_KEY }}` with repository secret

## API Rate Limits

| API Tier | Rate Limit | Daily Quota | Commercial Use |
|----------|------------|-------------|----------------|
| Public (Free) | 4 req/min | 5,760 req/day | No |
| Premium | Higher (varies) | Higher | Yes |

**For EIDAuthentication:** Public API is sufficient. Single file scan per release (infrequent) will not exceed limits.

## Recommended Build Order

1. **Prerequisite:** Add `VT_API_KEY` to repository secrets
   - Navigate to Settings > Secrets and variables > Actions
   - Add new repository secret named `VT_API_KEY`
   - Value: API key from VirusTotal account

2. **Modify `windows-build.yaml`:**
   - Add artifact upload step to build job (after NSIS creation)
   - Add new `virustotal` job with `needs: build`
   - Set appropriate permissions

3. **Test:** Create a test release or use `workflow_dispatch` for manual testing

4. **Optional:** Add detection threshold logic if automated blocking desired

## Sources

- [crazy-max/ghaction-virustotal GitHub Action](https://github.com/crazy-max/ghaction-virustotal) - HIGH confidence - Official action documentation
- [VirusTotal API v3 Overview](https://developers.virustotal.com/reference/overview) - HIGH confidence - Official API documentation
- [VirusTotal Public vs Premium API](https://developers.virustotal.com/reference/public-vs-premium-api) - HIGH confidence - Rate limit information
- Existing `windows-build.yaml` workflow analysis - HIGH confidence - Project-specific context

---

# ORIGINAL CONTENT: SonarQube Issue Remediation Architecture

*The following sections remain from the original SonarQube remediation research.*

---

## Issue Remediation Architecture

### System Overview

```
+-------------------------------------------------------------------------+
|                    SonarQube Issue Remediation Flow                      |
+-------------------------------------------------------------------------+
|                                                                          |
|   +-------------------+       +-------------------+                      |
|   | INDEPENDENT LAYER |       | DEPENDENT LAYER   |                      |
|   | (No Dependencies) |       | (Requires Prior)  |                      |
|   +-------------------+       +-------------------+                      |
|                                                                          |
|   +-------------+  +-------------+  +-------------+  +----------------+  |
|   | Macro ->    |  | auto        |  | enum class  |  | Independent    |  |
|   | constexpr   |  | Conversion  |  | Conversion  |  | Style Issues   |  |
|   +------+------+  +------+------+  +------+------+  +--------+-------+  |
|          |                |                |                    |        |
|          v                v                v                    v        |
|   +------------------------------------------------------------------+  |
|   |                     VERIFICATION GATE                            |  |
|   |              (Build + Static Analysis Check)                     |  |
|   +------------------------------------------------------------------+  |
|                                |                                        |
|   +-----------------------------v-------------------------------------+  |
|   |                     DEPENDENT LAYER                               |  |
|   +------------------------------------------------------------------+  |
|   |                                                                   |  |
|   |  +-------------+    +-------------+    +-------------------+     |  |
|   |  | Const       |    | Complexity  |    | Nesting           |     |  |
|   |  | Correctness |    | Reduction   |    | Reduction         |     |  |
|   |  +-------------+    +-------------+    +-------------------+     |  |
|   |        ^                   ^                    ^                |  |
|   |        |                   |                    |                |  |
|   |  [Requires macros done]    |  [Requires helpers from complexity] |  |
|   |                           |                                     |  |
|   +---------------------------+-------------------------------------+  |
|                                                                          |
+-------------------------------------------------------------------------+
```

### Issue Categories by Dependency

#### Layer 1: Independent Issues (No Dependencies)

These issues can be fixed in any order or in parallel. They do not affect each other.

| Issue Category | Count | Description | Rationale |
|----------------|-------|-------------|-----------|
| **auto Conversion** | ~40 | Replace redundant type with `auto` | Pure style change, no API impact |
| **enum class Conversion** | ~8 | Convert unscoped enums to `enum class` | Scoped to definition site |
| **C-style Cast Review** | ~8 | Review and document/fix C-style casts | Independent per occurrence |
| **Ellipsis Notation** | ~3 | Review variadic function usage | Independent per function |
| **Unused Parameters** | ~8 | Handle or document unused parameters | Independent per function |
| **Range-based For Loops** | ~8 | Convert index loops to range-based | Independent per loop |

#### Layer 2: Foundation Issues (Enable Others)

These issues should be addressed first because other fixes depend on their completion.

| Issue Category | Count | Description | Enables |
|----------------|-------|-------------|---------|
| **Macro to constexpr** | ~35 | Convert preprocessor macros to `constexpr` | Const correctness (macros cannot be const) |

#### Layer 3: Dependent Issues (Require Foundation)

These issues require foundation issues to be complete first.

| Issue Category | Count | Description | Dependency |
|----------------|-------|-------------|------------|
| **Const Correctness (globals)** | ~60 | Add `const` to global variables | Requires macro -> constexpr (macros cannot be const) |
| **Const Correctness (functions)** | ~15 | Add `const` to member functions | Independent of macros |
| **Complexity Reduction** | ~30 | Reduce cognitive complexity | Creates helper functions |
| **Nesting Reduction** | ~25 | Reduce nesting depth | Requires complexity helpers |
| **Init-statements** | ~80 | Use if/switch init-statements | Independent, but benefits from nesting reduction |

#### Layer 4: Integration Issues (Affect Multiple Files)

These issues span multiple files and require coordination.

| Issue Category | Count | Description | Complexity |
|----------------|-------|-------------|------------|
| **std::array Conversion** | ~30 | Convert C-style arrays to `std::array` | Changes API signatures |
| **Initialization Lists** | ~10 | Use constructor initialization lists | Changes class structure |
| **Redundant Casts** | ~10 | Remove unnecessary casts | Requires type analysis |
| **Rule of Five/Three** | ~15 | Review special member functions | Changes class design |

## Dependency Graph

```
Macro -> constexpr
       |
       +---> Const Correctness (globals can now be const)

Complexity Reduction
       |
       +---> Nesting Reduction (helpers reduce both complexity AND nesting)

Independent (parallel execution):
       +---> auto Conversion
       +---> enum class Conversion
       +---> C-style Cast Review
       +---> Ellipsis Notation
       +---> Unused Parameters
       +---> Range-based For Loops
       +---> Init-statements
       +---> std::array Conversion (careful: LSASS)
       +---> Initialization Lists
       +---> Redundant Casts
       +---> Rule of Five/Three
       +---> Const Correctness (functions)
```

## Recommended Phase Structure

### Phase Ordering Rationale

Based on dependency analysis, the following phase order ensures:

1. **Foundation first** - Issues that unblock others
2. **Independent parallelization** - Issues that can run concurrently
3. **Dependent last** - Issues that require foundation work
4. **Risk mitigation** - High-risk changes isolated and verified

### Recommended Phase Order

| Phase | Focus | Issue Count | Dependencies | Risk Level |
|-------|-------|-------------|--------------|------------|
| **31** | Macro to constexpr | ~35 | None | Low |
| **32** | auto Conversion | ~40 | None | Very Low |
| **33** | Independent Style Issues | ~27 | None | Very Low |
| **34** | Const Correctness (globals) | ~60 | Phase 31 | Low |
| **35** | Const Correctness (functions) | ~15 | None | Low |
| **36** | Complexity Reduction | ~30 | None | Medium |
| **37** | Nesting Reduction | ~25 | Phase 36 | Medium |
| **38** | Init-statements | ~80 | None (but Phase 37 helps) | Low |
| **39** | Integration Changes | ~65 | Varies | Medium-High |
| **40** | Final Verification | - | All | N/A |

### Phase Detail

#### Phase 31: Macro to constexpr (Foundation)

**Goal:** Convert ~35 preprocessor macros to `constexpr` constants where safe.

**Issue Types:**
- Replace macro with "const", "constexpr" or "enum"

**Files Most Affected:**
- EIDCardLibrary/*.cpp (constants)
- EIDConfigurationWizard/*.cpp (message IDs, flags)

**Constraints:**
- Resource compiler requires `#define` for resource IDs
- `__FILE__`, `LINE__`, `__FUNCTION__` must remain macros
- Windows header configuration macros must remain

**Success Criteria:**
1. All convertible macros replaced with `constexpr`
2. Build passes with no new warnings
3. Resource compilation unaffected

#### Phase 32: auto Conversion (Independent)

**Goal:** Replace explicit types with `auto` where type is obvious from context.

**Issue Types:**
- Replace redundant type with "auto"

**Files Most Affected:**
- EIDCardLibrary/CredentialManagement.cpp
- EIDCardLibrary/CContainerHolderFactory.cpp

**Constraints:**
- Avoid `auto` where type is not obvious (readability)
- Template iterator declarations benefit most

**Success Criteria:**
1. ~40 issues resolved
2. Code readability maintained or improved
3. Build passes

#### Phase 33: Independent Style Issues (Independent)

**Goal:** Address multiple independent low-risk style issues.

**Issue Types:**
- Convert to enum class (~8)
- C-style cast review (~8)
- Ellipsis notation review (~3)
- Unused parameter handling (~8)

**Files Most Affected:**
- Spread across all projects

**Success Criteria:**
1. ~27 issues resolved or documented as won't-fix
2. Build passes

#### Phase 34: Const Correctness - Globals (Dependent on Phase 31)

**Goal:** Add `const` to global variables that should be immutable.

**Issue Types:**
- Global variables should be const
- Global pointers should be const at every level

**Dependency on Phase 31:**
- Macros cannot be `const` - must convert to `constexpr` first
- After macro conversion, these globals can be properly const-qualified

**Files Most Affected:**
- EIDCardLibrary/Tracing.cpp
- EIDCardLibrary/Package.cpp
- EIDAuthenticationPackage/EIDSecuritySupportProvider.cpp

**Won't-Fix Categories:**
- LSA function pointers (assigned at runtime)
- Tracing state (dynamically enabled/disabled)
- DLL state (COM reference counting)
- UI state (track user selections)
- Windows API buffers (CryptoAPI requires non-const)

**Success Criteria:**
1. All legitimately const globals marked `const`
2. Won't-fix categories documented
3. Build passes

#### Phase 35: Const Correctness - Functions (Independent)

**Goal:** Add `const` to member functions that should be const.

**Issue Types:**
- Function should be const

**Files Most Affected:**
- EIDCardLibrary class methods
- EIDCredentialProvider COM objects (careful)

**Constraints:**
- COM interface methods cannot change signature
- Virtual function overrides must match base

**Success Criteria:**
1. ~15 issues resolved or documented
2. Build passes

#### Phase 36: Complexity Reduction (Foundation for Phase 37)

**Goal:** Reduce cognitive complexity in high-complexity functions.

**Issue Types:**
- Refactor this function to reduce its cognitive complexity

**Creates Helper Functions:**
- Extract complex boolean conditions
- Extract validation chains
- Extract repeated patterns

**These helpers enable Phase 37:**
- Helper functions reduce both cognitive complexity AND nesting depth
- Nesting reduction can reuse the extracted helpers

**Files Most Affected:**
- EIDCardLibrary/StoredCredentialManagement.cpp
- EIDCardLibrary/CertificateUtilities.cpp
- EIDAuthenticationPackage/EIDSecuritySupportProvider.cpp

**Won't-Fix Categories:**
- SEH-protected code
- Primary authentication functions
- Complex state machines
- Crypto validation chains

**Success Criteria:**
1. ~30 complexity issues reduced or documented
2. Helper functions extracted where beneficial
3. Build passes

#### Phase 37: Nesting Reduction (Dependent on Phase 36)

**Goal:** Reduce nesting depth in deeply nested functions.

**Issue Types:**
- Refactor nesting > 3 if/for/while/switch

**Dependency on Phase 36:**
- Complexity reduction phase creates helper functions
- Nesting reduction can leverage same helpers
- Early return/continue patterns benefit from reduced complexity

**Files Most Affected:**
- EIDCardLibrary/CertificateUtilities.cpp
- EIDConfigurationWizard/EIDConfigurationWizardPage03.cpp
- EIDAuthenticationPackage/EIDAuthenticationPackage.cpp

**Won't-Fix Categories:**
- SEH-protected code (exception safety)
- Complex state machines
- Windows message handlers

**Success Criteria:**
1. ~25 nesting issues reduced or documented
2. Build passes

#### Phase 38: Init-statements (Independent but Phase 37 Helps)

**Goal:** Use C++17 if/switch init-statements to limit variable scope.

**Issue Types:**
- Use "init-statement" in "if" or "switch"

**Phase 37 Synergy:**
- Nesting reduction often creates patterns where init-statements help
- Can be done independently, but benefits from cleaner code structure

**Files Most Affected:**
- Spread across all projects

**Success Criteria:**
1. ~80 issues resolved
2. Variable scopes narrowed
3. Build passes

#### Phase 39: Integration Changes (Multiple Dependencies)

**Goal:** Address changes that span multiple files or affect APIs.

**Issue Types:**
- Convert to std::array (~30)
- Initialization lists (~10)
- Redundant casts (~10)
- Rule of Five/Three (~15)

**Risk Assessment:**
- std::array changes API signatures - medium risk
- Rule of Five/Three changes class design - medium risk
- Redundant cast removal - low risk
- Initialization lists - low risk

**LSASS Safety Concerns:**
- std::array is stack-allocated, generally safe for LSASS
- Avoid std::vector (dynamic allocation)
- Test thoroughly in VM before deployment

**Success Criteria:**
1. ~65 issues resolved or documented
2. No API breakage
3. Build passes

#### Phase 40: Final Verification

**Goal:** Confirm all remediation is complete and stable.

**Verification Steps:**
1. Full solution rebuild (all 7 projects)
2. Static analysis scan
3. No new warnings
4. SonarQube re-scan
5. Document remaining won't-fix issues

## Data Flow

### Issue Remediation Flow

```
SonarQube Scan Results
        |
        v
+-------------------+
| Categorize Issues |
+-------------------+
        |
        +---> Independent Issues ------> [Phase 32, 33, 35, 38]
        |
        +---> Foundation Issues ------> [Phase 31, 36]
        |              |
        |              v
        +---> Dependent Issues -------> [Phase 34, 37, 39]
        |
        v
+-------------------+
| Verification Gate |
+-------------------+
        |
        v
Final SonarQube Scan
```

### Build Order Within Phases

```
EIDCardLibrary (Static Library)
        |
        +---> EIDAuthenticationPackage (LSA DLL)
        +---> EIDCredentialProvider (Credential Provider DLL)
        +---> EIDPasswordChangeNotification (Password Filter DLL)
        +---> EIDConfigurationWizard (GUI EXE)
        +---> EIDConfigurationWizardElevated (Elevated EXE)
        +---> EIDLogManager (Diagnostic EXE)
```

All phases must verify:
1. EIDCardLibrary builds first (no dependencies)
2. All 7 projects build successfully
3. No new warnings introduced
4. Static CRT linkage confirmed for LSASS-loaded components

## Anti-Patterns

### Anti-Pattern 1: Ignoring Macro Dependencies

**What people do:** Try to add `const` to globals before converting macros.

**Why it's wrong:** Preprocessor macros cannot be `const`. The code will fail to compile or the macro will remain unaddressable.

**Do this instead:** Always complete macro -> constexpr conversion (Phase 31) before const correctness (Phase 34).

### Anti-Pattern 2: Treating Nesting Before Complexity

**What people do:** Try to reduce nesting without reducing complexity first.

**Why it's wrong:** Deeply nested code often has high cognitive complexity. Extracting helpers for complexity automatically helps nesting. Doing nesting first risks duplicating effort.

**Do this instead:** Complete complexity reduction (Phase 36) before nesting reduction (Phase 37) to leverage shared helper functions.

### Anti-Pattern 3: Using std::vector in LSASS Context

**What people do:** Convert C-style arrays to `std::vector` for "modernization."

**Why it's wrong:** `std::vector` uses dynamic allocation which is unsafe in LSASS. LSASS has strict memory constraints and exception handling requirements.

**Do this instead:** Use `std::array` for fixed-size arrays (stack-allocated, no exceptions). Keep C-style arrays if `std::array` is not suitable.

### Anti-Pattern 4: Changing COM Interface Signatures

**What people do:** Add `const` to COM interface methods.

**Why it's wrong:** COM interfaces are ABI contracts. Changing method signatures breaks binary compatibility.

**Do this instead:** Document COM methods as won't-fix for const correctness. Focus on internal implementation.

### Anti-Pattern 5: Batch Modifying All Projects Simultaneously

**What people do:** Fix issues across all 7 projects in one massive change.

**Why it's wrong:** When issues occur, impossible to isolate which project introduced the problem. Difficult to create targeted rollbacks.

**Do this instead:** Fix issues project by project, with full verification between each. Start with EIDCardLibrary (leaf dependency).

## Integration Points

### Project Dependency Integration

| Project | Depends On | Integration Type |
|---------|------------|------------------|
| EIDCardLibrary | None | Static library, leaf dependency |
| EIDAuthenticationPackage | EIDCardLibrary | Static link, LSA integration |
| EIDCredentialProvider | EIDCardLibrary | Static link, LogonUI integration |
| EIDPasswordChangeNotification | EIDCardLibrary | Static link, LSASS integration |
| EIDConfigurationWizard | EIDCardLibrary | Static link, UI application |
| EIDConfigurationWizardElevated | EIDCardLibrary | Static link, admin operations |
| EIDLogManager | EIDCardLibrary | Static link, diagnostic tool |

### Windows API Integration Points

| Integration | Issue Impact | Won't-Fix Category |
|------------|--------------|-------------------|
| LSA Function Pointers | Global variables | Runtime-assigned pointers |
| CryptoAPI Buffers | Const correctness | Non-const char arrays required |
| Resource Compiler | Macro usage | Resource IDs must be `#define` |
| Credential Provider | Const methods | COM interface signatures |
| Windows Headers | Macro usage | Configuration macros |

## Verification Strategy

### Per-Phase Verification

| Check | Tool | Pass Criteria |
|-------|------|---------------|
| Compilation | MSBuild | 0 errors |
| Warnings | /W4 | 0 new warnings |
| Static Analysis | /analyze | 0 new issues |
| Linkage | Linker | Static CRT for LSASS |
| Size | Binary compare | No unexpected growth |

### Milestone Completion Verification

1. **Build Verification:** All 7 projects compile
2. **Static Analysis:** No new /analyze warnings
3. **SonarQube Scan:** Metrics improved
4. **Won't-Fix Documentation:** All categories documented
5. **Runtime Smoke Test:** Basic functionality verified (VM)

## LSASS Safety Constraints

### Always Safe in LSASS

- `std::array` - Stack-allocated, no exceptions
- `constexpr` - Compile-time, no runtime cost
- `const` - No runtime impact
- `auto` - Type deduction, no runtime impact
- `enum class` - Scoped enumeration, no runtime impact
- Early return/continue - Control flow, no allocation
- Helper function extraction - Code organization, no allocation

### Never Safe in LSASS

- `std::vector` - Dynamic allocation
- `std::string` - Dynamic allocation
- `std::map` / `std::unordered_map` - Dynamic allocation
- Exceptions - LSASS cannot use exceptions
- `new` / `delete` - Dynamic allocation
- `malloc` / `free` - Dynamic allocation

### Conditional (Evaluate Per Case)

- `std::span` - Non-owning view, lifetime analysis required
- `std::string_view` - Non-owning view, dangling reference risk
- `std::optional` - Check implementation for allocation
- `std::expected` - Check implementation for allocation

## Sources

- [SonarQube C++ Rules](https://rules.sonarsource.com/cpp/) - Issue categories and severity (HIGH confidence)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/) - Modern C++ patterns (HIGH confidence)
- [MSVC LSASS Development](https://learn.microsoft.com/en-us/windows/win32/secauthn/authentication-packages) - LSA integration requirements (HIGH confidence)
- [MSVC C++23 Support](https://learn.microsoft.com/en-us/cpp/overview/msvc-conformance-improvements) - Feature availability (HIGH confidence)
- v1.0-v1.3 Milestone Documentation - Established patterns and won't-fix categories (HIGH confidence)

---

*Architecture research for: SonarQube Issue Remediation in C++23 Windows LSASS Codebase*
*Researched: 2026-02-18*

*VirusTotal CI/CD Integration added: 2026-02-19*
