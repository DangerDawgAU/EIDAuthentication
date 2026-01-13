# Building EID Authentication

This document describes how to build the EID Authentication project from source.

## Prerequisites

### Required Software

1. **Visual Studio 2025 Community** (or higher)
   - Workload: Desktop development with C++
   - Windows SDK: 10.0.22621.0 or later
   - Platform Toolset: v145

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
2. **Build Solution** - Compiles all 8 C++ projects:
   - EIDCardLibrary (C++ static library)
   - EIDAuthenticationPackage (C++ LSA authentication DLL)
   - EIDCredentialProvider (C++ Credential Provider v2 DLL)
   - EIDPasswordChangeNotification (C++ password change notification DLL)
   - EIDConfigurationWizard (C++ GUI configuration tool)
   - EIDConfigurationWizardElevated (C++ elevated operations tool)
   - EIDLogManager (C++ log management utility)
   - EIDTest (C++ testing and debugging tool)
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
- `EIDTest.exe` (371 KB) - Test utility

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

**Compiler:** Microsoft Visual C++ 2025 (MSVC v145)
**Target OS:** Windows 10/11 (64-bit)
**Language Standard:** C++14
**Character Set:** Unicode (TCHAR)
**Runtime:** Multi-threaded DLL (/MD for Release, /MDd for Debug)

## Notes

- **Internet functionality removed:** All online database lookup and telemetry reporting features have been removed from the codebase
- **Certificate cleanup:** The installer now includes `CleanupCertificates.ps1` for proper uninstallation
- **No external dependencies:** All required libraries are included in Windows SDK or Visual Studio
- **64-bit only:** This project targets 64-bit Windows exclusively (x64 platform)

## Additional Resources

- **Deployment Guide:** See `DEPLOYMENT.md` for installation instructions
- **Project Documentation:** See `README.md` for project overview
- **Technical Notes:** See `notes.md` for detailed technical documentation
