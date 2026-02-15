# Building EID Authentication

This document describes how to build the EID Authentication project from source.

## Prerequisites

### Required Software

1. **Visual Studio 2022 or later** (Community, Professional, or Enterprise)
   - Workload: Desktop development with C++
   - Windows SDK: 10.0.22621.0 or later
   - Platform Toolset: v143

   Note: v143 toolset is required for Windows 7+ compatibility (v145 dropped Win7 support)

2. **NSIS (Nullsoft Scriptable Install System)** - Optional, for installer
   - Download from: https://nsis.sourceforge.io/
   - Default installation path: `C:\Program Files (x86)\NSIS\`
   - Used to create the installer package

3. **PowerShell 5.0 or later** - Required for certificate cleanup
   - Included with Windows 10/11

## Build Instructions

### Quick Build

Run the build script with default settings (Release x64):

```batch
build.bat
```

### Build Options

```batch
build.bat [Configuration] [Platform]
```

**Configuration:**
- `Release` (default) - Optimized production build
- `Debug` - Debug build with symbols

**Platform:**
- `x64` (default) - 64-bit build
- `Win32` - 32-bit build (not recommended)

**Examples:**
```batch
build.bat Release x64    # Release build for x64
build.bat Debug x64      # Debug build for x64
build.bat Release        # Release build (x64 assumed)
```

### Build Process

The build script performs the following steps:

1. **Clean** - Removes previous build artifacts from `x64\[Configuration]`
2. **Build Solution** - Compiles all 7 C++ projects:
   - EIDCardLibrary (C++ static library)
   - EIDAuthenticationPackage (C++ LSA authentication DLL)
   - EIDCredentialProvider (C++ Credential Provider v2 DLL)
   - EIDPasswordChangeNotification (C++ password change notification DLL)
   - EIDConfigurationWizard (C++ GUI configuration tool)
   - EIDConfigurationWizardElevated (C++ elevated operations tool)
   - EIDLogManager (C++ log management utility)
3. **Build Installer** - Creates NSIS installer (Release builds only)

### Build Output

**Location:** `x64\Release\` (or `x64\Debug\`)

**Files:**
- `EIDAuthenticationPackage.dll` (271 KB) - LSA authentication package
- `EIDCredentialProvider.dll` (663 KB) - Credential Provider
- `EIDPasswordChangeNotification.dll` (151 KB) - Password change notification
- `EIDConfigurationWizard.exe` (277 KB) - Configuration wizard
- `EIDConfigurationWizardElevated.exe` (145 KB) - Elevated wizard
- `EIDLogManager.exe` (187 KB) - Log manager

**Installer:** `Installer\EIDInstallx64.exe` (688 KB)

## Build Verification

After building, verify the following:

1. **All DLLs and executables exist** in `x64\Release\`
2. **File sizes match expected values** (see above)
3. **Installer was created** (Release builds only): `Installer\EIDInstallx64.exe`
4. **Build log shows no errors**: Check `build.log` if build fails

## Troubleshooting

### Build Errors

**Error: "Visual Studio 2025 not found"**
- Ensure Visual Studio 2025 is installed at: `C:\Program Files\Microsoft Visual Studio\18\Community\`
- Install the "Desktop development with C++" workload

**Error: "Windows SDK not found"**
- Install Windows SDK 10.0.22621.0 or later through Visual Studio Installer
- Go to: Individual Components → SDKs, libraries, and frameworks

**Error: "Platform Toolset v145 not found"**
- Install Platform Toolset v145 through Visual Studio Installer
- Go to: Individual Components → Compilers, build tools, and runtimes

**Build fails with compiler errors**
- Check `build.log` for detailed error information
- Ensure all source files are present and not corrupted
- Try cleaning the solution: Delete `x64\` folder and rebuild

### Installer Errors

**Warning: "NSIS not found"**
- Install NSIS from https://nsis.sourceforge.io/
- Ensure it's installed to default location: `C:\Program Files (x86)\NSIS\`
- Installer build is optional - DLLs/executables still usable

**Installer build fails**
- Check `Installer\Installerx64.nsi` for syntax errors
- Ensure `CleanupCertificates.ps1` exists in `Installer\` folder
- Verify all DLLs/executables exist in `x64\Release\`

## Manual Build (Visual Studio IDE)

If you prefer to build through Visual Studio:

1. Open `EIDCredentialProvider.sln`
2. Select configuration: **Release** or **Debug**
3. Select platform: **x64**
4. Build → Rebuild Solution (or press Ctrl+Shift+B)

For installer:
1. Open command prompt in `Installer\` folder
2. Run: `"C:\Program Files (x86)\NSIS\makensis.exe" Installerx64.nsi`

## Clean Build

To perform a completely clean build:

```batch
rmdir /s /q x64
del /q build.log
build.bat Release
```

## Build Environment

**Compiler:** Microsoft Visual C++ 2022 (MSVC v143)
**Target OS:** Windows 7 or later (64-bit)
**Language Standard:** C++23 (/std:c++23preview)
**Character Set:** Unicode (TCHAR)
**Runtime:** Multi-threaded Static (/MT for Release, /MTd for Debug)

Note: Static CRT is required for LSASS compatibility

### C++23 Modernization Notes

This project uses C++23 language features with the MSVC compiler. All 7 projects
in the solution are configured with `<LanguageStandard>stdcpp23preview</LanguageStandard>`.

**Key Requirements:**
- Visual Studio 2022 or later (v143 toolset for Windows 7+ compatibility)
- `/std:c++23preview` compiler flag (set via project configuration)
- Static CRT linkage (/MT) for LSASS-loaded DLLs

**Modern Features Used:**
- `std::expected<T, E>` for type-safe error handling
- `std::format` for text formatting (non-LSASS components)
- `std::span<T>` for bounds-safe buffer access
- `std::to_underlying()` for enum conversions
- Enhanced `constexpr` and `noexcept` specifications

For details on the modernization effort, see [.planning/phases/](.planning/phases/).

## Notes

- **Internet functionality removed:** All online database lookup and telemetry reporting features have been removed from the codebase
- **Certificate cleanup:** The installer now includes `CleanupCertificates.ps1` for proper uninstallation
- **No external dependencies:** All required libraries are included in Windows SDK or Visual Studio
- **64-bit only:** This project targets 64-bit Windows exclusively (x64 platform)

## Additional Resources

- **Deployment Guide:** See `DEPLOYMENT.md` for installation instructions
- **Project Documentation:** See `README.md` for project overview
- **Technical Notes:** See `notes.md` for detailed technical documentation
