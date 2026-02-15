# Architecture Research

**Domain:** C++ Standard Upgrade Modernization (C++14 to C++23)
**Researched:** 2026-02-15
**Confidence:** HIGH

## Standard Architecture

### System Overview

```
+-------------------------------------------------------------------------+
|                          EIDAuthentication Solution                      |
+-------------------------------------------------------------------------+
|                                                                          |
|  +-------------------------------------------------------------------+  |
|  |                     EIDCardLibrary (Static Lib)                   |  |
|  |                     Core Shared Utilities                         |  |
|  |  [CertificateUtilities] [CredentialManagement] [StringConversion] |  |
|  |  [SmartCardModule] [GPO] [Tracing] [Package] [Registration]       |  |
|  +--------------------------------+----------------------------------+  |
|                                   |                                      |
|           +-----------------------+-----------------------+              |
|           |                       |                       |              |
|           v                       v                       v              |
|  +------------------+    +------------------+    +------------------+    |
|  | DLLs (System)    |    | EXEs (User)      |    | EXEs (Admin)     |    |
|  +------------------+    +------------------+    +------------------+    |
|  | EIDAuthPackage   |    | EIDConfigWizard  |    | EIDConfigWizard  |    |
|  | (LSA DLL)        |    |                  |    | Elevated         |    |
|  +------------------+    +------------------+    +------------------+    |
|  | EIDCredential    |    | EIDLogManager    |    |                  |    |
|  | Provider (DLL)   |    |                  |    |                  |    |
|  +------------------+    +------------------+    +------------------+    |
|  | EIDPassword      |                                                       |
|  | ChangeNotify     |                                                       |
|  +------------------+                                                       |
|                                                                          |
+-------------------------------------------------------------------------+
```

### Component Responsibilities

| Project | Type | Responsibility | Dependencies |
|---------|------|----------------|--------------|
| EIDCardLibrary | Static Library (.lib) | Core shared utilities: certificate handling, credential management, smart card operations, string conversion, GPO, tracing | None (foundation) |
| EIDAuthenticationPackage | DLL (.dll) | LSA authentication package - integrates with Windows Local Security Authority | EIDCardLibrary |
| EIDCredentialProvider | DLL (.dll) | Windows Credential Provider - handles logon UI and authentication | EIDCardLibrary |
| EIDPasswordChangeNotification | DLL (.dll) | Password change notification DLL - handles password events | EIDCardLibrary |
| EIDConfigurationWizard | EXE (.exe) | User-mode configuration wizard application | EIDCardLibrary |
| EIDConfigurationWizardElevated | EXE (.exe) | Elevated admin configuration tasks | EIDCardLibrary |
| EIDLogManager | EXE (.exe) | Log management utility (requires admin) | EIDCardLibrary |

## Recommended Project Structure

```
EIDAuthentication/
+-- EIDCardLibrary/              # Core static library (MODERNIZE FIRST)
|   +-- *.cpp, *.h               # Shared utilities
|   +-- EIDCardLibrary.vcxproj   # v143 toolset, no /std flag (defaults to C++14)
|
+-- EIDAuthenticationPackage/    # LSA DLL (MODERNIZE SECOND)
|   +-- *.cpp, *.h
|   +-- EIDAuthenticationPackage.vcxproj  # ProjectReference to EIDCardLibrary
|
+-- EIDCredentialProvider/       # Credential Provider DLL (MODERNIZE THIRD)
|   +-- *.cpp, *.h
|   +-- EIDCredentialProvider.vcxproj    # ProjectReference to EIDCardLibrary
|
+-- EIDPasswordChangeNotification/  # DLL (MODERNIZE WITH OTHER DLLs)
|   +-- *.cpp, *.h
|   +-- EIDPasswordChangeNotification.vcxproj
|
+-- EIDConfigurationWizard/      # EXE (MODERNIZE AFTER CORE)
|   +-- *.cpp, *.h
|   +-- EIDConfigurationWizard.vcxproj
|
+-- EIDConfigurationWizardElevated/  # EXE (MODERNIZE WITH OTHER EXEs)
|   +-- *.cpp, *.h
|   +-- EIDConfigurationWizardElevated.vcxproj
|
+-- EIDLogManager/               # EXE (MODERNIZE WITH OTHER EXEs)
|   +-- *.cpp, *.h
|   +-- EIDLogManager.vcxproj
```

### Structure Rationale

- **EIDCardLibrary first:** All other projects depend on it via ProjectReference. Changes propagate through the dependency chain. Must modernize first to enable consumers to use modern features.
- **DLLs before EXEs:** System components (LSA, Credential Provider) are higher risk and benefit from earlier validation.
- **Grouped modernization:** DLLs can be modernized together, then EXEs, to minimize context switching.

## Architectural Patterns

### Pattern 1: Dependency-First Modernization (Bottom-Up)

**What:** Modernize projects in reverse dependency order - start with leaf dependencies (no dependencies), work up to root consumers.

**When to use:** Always for C++ standard upgrades across multiple projects.

**Trade-offs:**
- (+) Isolates ABI issues to the static library boundary
- (+) Consumers can use new language features immediately after library upgrade
- (-) Higher risk if core library has issues (affects everything)

**Example:**
```
Modernization Order:
1. EIDCardLibrary     (leaf - no dependencies)
2. DLLs (parallel)   (depend on EIDCardLibrary)
   - EIDAuthenticationPackage
   - EIDCredentialProvider
   - EIDPasswordChangeNotification
3. EXEs (parallel)   (depend on EIDCardLibrary)
   - EIDConfigurationWizard
   - EIDConfigurationWizardElevated
   - EIDLogManager
```

### Pattern 2: Incremental Standard Upgrade (Stepwise)

**What:** Upgrade C++ standard in steps (C++14 -> C++17 -> C++20 -> C++23) rather than directly.

**When to use:** Large codebases or when ABI compatibility concerns exist.

**Trade-offs:**
- (+) Easier to identify which standard introduced a breaking change
- (+) Can adopt features incrementally
- (-) More total work (multiple build/test cycles)
- (-) May not be necessary if Microsoft binary compatibility holds

**Recommendation:** For this project, go directly to C++23 since MSVC Build Tools v14.50 maintains binary compatibility with VS2015+ code. Direct upgrade is acceptable.

### Pattern 3: Configuration Synchronization

**What:** Ensure all projects use consistent compiler flags and settings.

**When to use:** Multi-project solutions to prevent ODR violations and ABI mismatches.

**Trade-offs:**
- (+) Prevents subtle ABI issues
- (+) Consistent behavior across projects
- (-) Requires property sheet management

**Example:**
```xml
<!-- Create a shared property sheet: CommonSettings.props -->
<PropertyGroup>
  <LanguageStandard>stdcpp23</LanguageStandard>
  <PlatformToolset>v145</PlatformToolset>
  <RuntimeLibrary Condition="'$(Configuration)'=='Debug'">MultiThreadedDebug</RuntimeLibrary>
  <RuntimeLibrary Condition="'$(Configuration)'=='Release'">MultiThreaded</RuntimeLibrary>
</PropertyGroup>
```

## Data Flow

### Build Dependency Flow

```
EIDCardLibrary.lib
       |
       +---> EIDAuthenticationPackage.dll (links statically)
       |
       +---> EIDCredentialProvider.dll (links statically)
       |
       +---> EIDPasswordChangeNotification.dll (links statically)
       |
       +---> EIDConfigurationWizard.exe (links statically)
       |
       +---> EIDConfigurationWizardElevated.exe (links statically)
       |
       +---> EIDLogManager.exe (links statically)
```

### Modernization Validation Flow

```
Phase 1: EIDCardLibrary
    |
    v
[Build Library] --> [Unit Tests] --> [Integration Smoke Test]
    |
    v
Phase 2: DLLs (Parallel if desired)
    |
    v
[Build DLLs] --> [Load Tests] --> [LSA/CP Registration Tests]
    |
    v
Phase 3: EXEs
    |
    v
[Build EXEs] --> [Functional Tests] --> [End-to-End Scenarios]
```

### Key Data Flows

1. **Static Linking:** EIDCardLibrary is statically linked into all consumers. No DLL boundary concerns, but all consumers must rebuild when library changes.
2. **Runtime Integration:** EIDAuthenticationPackage integrates with LSA (lsass.exe), EIDCredentialProvider with LogonUI. These have strict Windows loading requirements.
3. **Admin Elevation:** EIDConfigurationWizardElevated and EIDLogManager require UAC elevation (RequireAdministrator manifest).

## Modernization Strategy

### Recommended Modernization Order

| Phase | Projects | Rationale | Risk Level |
|-------|----------|-----------|------------|
| **1** | EIDCardLibrary | Core dependency - must be first | Medium (affects all) |
| **2a** | EIDAuthenticationPackage | LSA DLL - system critical | High |
| **2b** | EIDCredentialProvider | Credential Provider - UI critical | High |
| **2c** | EIDPasswordChangeNotification | Password notification | Medium |
| **3a** | EIDConfigurationWizard | User-facing application | Low |
| **3b** | EIDConfigurationWizardElevated | Admin operations | Low |
| **3c** | EIDLogManager | Log utility | Low |

### Per-Project Validation Strategy

| Project Type | Validation Approach | Key Tests |
|--------------|---------------------|-----------|
| **Static Library** | Unit tests + build verification | Compile, link to test harness, ABI check |
| **LSA DLL** | Load test in VM, authentication flow | LSA registration, auth sequences |
| **Credential Provider** | LogonUI integration test | Logon screen appearance, credential handling |
| **Password Notification DLL** | Password change event test | Event receipt, processing |
| **EXE (User)** | Functional testing | UI behavior, configuration persistence |
| **EXE (Admin)** | Elevated context testing | Registry writes, system config changes |

### Build Validation Approach

```powershell
# Phase 1: Core Library Validation
1. Build EIDCardLibrary with /std:c++23
2. Run static analysis (/analyze)
3. Address all warnings
4. Build test harness that links library
5. Verify no linker errors or ABI warnings

# Phase 2: DLL Validation
1. Build each DLL with /std:c++23
2. Verify exports (.def files unchanged)
3. Test DLL load in isolated environment
4. For LSA: Test in VM with rollback capability
5. Verify registration/unregistration

# Phase 3: EXE Validation
1. Build each EXE with /std:c++23
2. Verify manifests intact (UAC settings)
3. Functional testing
4. Performance comparison (optional)
```

## Anti-Patterns

### Anti-Pattern 1: Top-Down Modernization

**What people do:** Modernize executables first, then libraries.

**Why it's wrong:** Consumers cannot use new C++ features until their dependencies support them. Creates unnecessary intermediate states.

**Do this instead:** Always modernize dependencies before consumers (bottom-up).

### Anti-Pattern 2: Big-Bang Upgrade

**What people do:** Change /std flag on all projects simultaneously.

**Why it's wrong:** When errors occur, impossible to isolate which project introduced the issue. Difficult to create targeted rollbacks.

**Do this instead:** One project at a time, with full validation between each.

### Anti-Pattern 3: Ignoring Runtime Library Consistency

**What people do:** Allow different /MD vs /MDd or /MT vs /MTd settings across projects.

**Why it's wrong:** ODR violations, heap corruption, crashes. Static library built with /MT linked into DLL built with /MD causes runtime failures.

**Do this instead:** Ensure consistent RuntimeLibrary settings across all projects in solution. Current project uses:
- Debug: `MultiThreadedDebug` (/MTd)
- Release: `MultiThreaded` (/MT)

### Anti-Pattern 4: Skipping VM Testing for LSA Components

**What people do:** Test LSA DLL changes on development machine.

**Why it's wrong:** LSA crashes can render system unbootable. Recovery requires safe mode or reinstall.

**Do this instead:** Always test LSA changes in a VM with checkpoints/snapshots for instant rollback.

## Integration Points

### External Services

| Service | Integration Pattern | Notes |
|---------|---------------------|-------|
| Windows LSA (lsass.exe) | Authentication Package API | Requires system restart for registration changes |
| LogonUI | Credential Provider API | Loaded during logon, lock screen |
| Smart Card Subsystem | WinSCard API | Via winscard.lib |
| Certificate Store | CryptoAPI / CNG | Certificate validation and management |
| Windows Registry | Registry API | GPO and configuration storage |

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| EIDCardLibrary -> Consumers | Static linkage | All code inlined into consumer binaries |
| EIDAuthenticationPackage -> LSA | COM/LSA API | Strict interface requirements |
| EIDCredentialProvider -> LogonUI | Credential Provider interfaces | COM-based |
| EXEs -> Registry | Win32 Registry API | Configuration persistence |

## Scaling Considerations

| Concern | Description |
|---------|-------------|
| **Build time** | Static linking means full rebuild of consumers when library changes. Consider precompiled headers for faster builds. |
| **Binary size** | Each consumer embeds full EIDCardLibrary code. Acceptable for this project size. |
| **Deployment** | No shared DLL dependency - each component is self-contained. Simplifies deployment. |

## Rollback Strategy

### Version Control Strategy

```
Before modernization:
1. Create branch: feature/cpp23-migration
2. Tag current state: pre-cpp23-migration

After each project:
1. Commit with message: "Modernize EIDCardLibrary to C++23"
2. Tag: cpp23-EIDCardLibrary-complete

If rollback needed:
1. Revert to tag or use git reset --hard
2. Projects can be individually rolled back
```

### Rollback Triggers

| Trigger | Action |
|---------|--------|
| Unfixable compiler errors in library | Rollback to C++14, investigate in isolation |
| Runtime crash in LSA DLL | Immediate rollback, debug in VM |
| Credential Provider fails to load | Rollback, verify registration |
| EXE functional regression | Rollback specific EXE only |

## Sources

- [Upgrading C++ Projects to Visual Studio 2026](https://devblogs.microsoft.com/cppblog/upgrading-c-projects-to-visual-studio-2026/) - Official Microsoft guidance on C++ upgrade process (HIGH confidence)
- [MSVC Conformance Improvements](https://learn.microsoft.com/en-us/cpp/overview/msvc-conformance-improvements?view=msvc-170) - C++23 feature support in MSVC v14.50 (HIGH confidence)
- [Overview of Potential Upgrade Issues](https://learn.microsoft.com/en-us/cpp/porting/overview-of-potential-upgrade-issues-visual-cpp?view=msvc-170) - Microsoft documentation on upgrade considerations (HIGH confidence)
- [Modernizing C++: Building Rock Solid Stability](https://drlongnecker.com/blog/2025/03/modernizing-c-building-rock-solid-stability/) - Modernization strategy patterns (MEDIUM confidence)
- [Visual Studio C++ Multiple Project Solution Setup](https://stackoverflow.com/questions/60539041/visual-studio-c-multiple-project-solution-setup) - Community guidance on multi-project solutions (MEDIUM confidence)

---
*Architecture research for: C++ Standard Upgrade Modernization*
*Researched: 2026-02-15*
