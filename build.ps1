<#
.SYNOPSIS
    Build EIDAuthentication Project

.DESCRIPTION
    This script builds all components of the EID Authentication system:
    - EIDAuthenticationPackage.dll (LSA authentication package)
    - EIDCredentialProvider.dll (Credential Provider v2)
    - EIDPasswordChangeNotification.dll (Password change notification)
    - EIDConfigurationWizard.exe (Configuration tool)
    - EIDCardLibrary (shared library)
    - EIDLogManager.exe (Log management utility)
    - Installer (NSIS installer for Release builds)

.PARAMETER Configuration
    Build configuration: Debug or Release (default: Release)

.PARAMETER Platform
    Target platform: x64 or Win32 (default: x64)

.EXAMPLE
    .\build.ps1
    Builds Release configuration for x64 platform

.EXAMPLE
    .\build.ps1 Debug x64
    Builds Debug configuration for x64 platform
#>

param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    [ValidateSet("x64", "Win32")]
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "Building EID Authentication" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "Configuration: $Configuration" -ForegroundColor White
Write-Host "Platform: $Platform" -ForegroundColor White
Write-Host ""

# Find Visual Studio 2025 installation (using VS2022 build tools v143)
$devEnvPath = "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\devenv.com"

if (-not (Test-Path $devEnvPath)) {
    Write-Host "ERROR: Visual Studio 2025 not found at $devEnvPath" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please ensure Visual Studio 2025 Community is installed with:" -ForegroundColor Yellow
    Write-Host "  - Desktop development with C++" -ForegroundColor Yellow
    Write-Host "  - Windows SDK 10.0.22621.0 or later" -ForegroundColor Yellow
    Write-Host "  - Platform Toolset v143 (VS 2022 Build Tools)" -ForegroundColor Yellow
    Write-Host ""
    exit 1
}

# Clean previous build artifacts
Write-Host "Cleaning previous build..." -ForegroundColor Yellow
$buildDir = "x64\$Configuration"
if (Test-Path $buildDir) {
    Remove-Item -Path $buildDir -Recurse -Force -ErrorAction SilentlyContinue
}

# Build the solution
Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "Building solution: EIDCredentialProvider.sln" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "Build output will be logged to build.log" -ForegroundColor White

$buildArgs = @(
    "EIDCredentialProvider.sln",
    "/Rebuild",
    "$Configuration|$Platform"
)

& $devEnvPath @buildArgs 2>&1 | Tee-Object -FilePath "build.log"

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "============================================================" -ForegroundColor Red
    Write-Host "BUILD FAILED with error code $LASTEXITCODE" -ForegroundColor Red
    Write-Host "============================================================" -ForegroundColor Red
    Write-Host ""
    Write-Host "Check the output above for detailed error information" -ForegroundColor Yellow
    exit $LASTEXITCODE
}

Write-Host ""
Write-Host "============================================================" -ForegroundColor Green
Write-Host "BUILD SUCCEEDED" -ForegroundColor Green
Write-Host "============================================================" -ForegroundColor Green
Write-Host ""
Write-Host "Output files in ${buildDir}:" -ForegroundColor White

$outputFiles = Get-ChildItem -Path $buildDir -File -ErrorAction SilentlyContinue | Where-Object { $_.Extension -in '.dll', '.exe' }
$outputFiles | ForEach-Object { Write-Host "  $($_.Name)" -ForegroundColor White }

# Display file sizes for verification
Write-Host ""
Write-Host "File sizes:" -ForegroundColor White
$outputFiles | ForEach-Object {
    Write-Host "  $($_.Name) - $($_.Length) bytes" -ForegroundColor Gray
}

# Build installer (Release only)
if ($Configuration -eq "Release") {
    Write-Host ""
    Write-Host "============================================================" -ForegroundColor Cyan
    Write-Host "Building NSIS Installer" -ForegroundColor Cyan
    Write-Host "============================================================" -ForegroundColor Cyan

    $makensisPath = "C:\Program Files (x86)\NSIS\makensis.exe"

    if (Test-Path $makensisPath) {
        # Delete old installer
        $installerPath = "Installer\EIDInstallx64.exe"
        if (Test-Path $installerPath) {
            Remove-Item -Path $installerPath -Force
        }

        Push-Location "Installer"
        $process = Start-Process -FilePath $makensisPath -ArgumentList "Installerx64.nsi" -NoNewWindow -Wait
        Pop-Location

        if (Test-Path $installerPath) {
            $installerFile = Get-Item $installerPath
            Write-Host ""
            Write-Host "============================================================" -ForegroundColor Green
            Write-Host "INSTALLER BUILD SUCCEEDED" -ForegroundColor Green
            Write-Host "============================================================" -ForegroundColor Green
            Write-Host "Output: $installerPath" -ForegroundColor White
            Write-Host "Size: $($installerFile.Length) bytes" -ForegroundColor White
            Write-Host ""
            Write-Host "Installer includes:" -ForegroundColor White
            Write-Host "  - Certificate cleanup script (CleanupCertificates.ps1)" -ForegroundColor Gray
            Write-Host "  - All DLLs and executables" -ForegroundColor Gray
            Write-Host "  - Uninstaller with certificate removal" -ForegroundColor Gray
            Write-Host ""
        } else {
            Write-Host ""
            Write-Host "============================================================" -ForegroundColor Red
            Write-Host "INSTALLER BUILD FAILED" -ForegroundColor Red
            Write-Host "============================================================" -ForegroundColor Red
            Write-Host "Check Installer\Installerx64.nsi for errors" -ForegroundColor Yellow
            exit 1
        }
    } else {
        Write-Host ""
        Write-Host "WARNING: NSIS not found at `"$makensisPath`"" -ForegroundColor Yellow
        Write-Host "Skipping installer build" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "To build the installer, install NSIS from https://nsis.sourceforge.io/" -ForegroundColor Cyan
    }
}

Write-Host ""
Write-Host "============================================================" -ForegroundColor Green
Write-Host "Build Complete" -ForegroundColor Green
Write-Host "============================================================" -ForegroundColor Green
Write-Host "Configuration: $Configuration" -ForegroundColor White
Write-Host "Platform: $Platform" -ForegroundColor White
Write-Host "All components built successfully" -ForegroundColor Green
Write-Host ""
