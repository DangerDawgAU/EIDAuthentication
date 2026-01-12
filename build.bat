@echo off
REM Build EIDAuthentication Project
REM Usage: build.bat [Debug|Release] [x64|Win32]
REM
REM This script builds all components of the EID Authentication system:
REM - EIDAuthenticationPackage.dll (LSA authentication package)
REM - EIDCredentialProvider.dll (Credential Provider v2)
REM - EIDPasswordChangeNotification.dll (Password change notification)
REM - EIDConfigurationWizard.exe (Configuration tool)
REM - EIDCardLibrary (shared library)
REM - EIDLogManager.exe (Log management utility)
REM - EIDTest.exe (Testing tool)
REM - Installer (NSIS installer for Release builds)

setlocal

REM Set default configuration and platform
set CONFIG=%1
set PLATFORM=%2

if "%CONFIG%"=="" set CONFIG=Release
if "%PLATFORM%"=="" set PLATFORM=x64

echo ============================================================
echo Building EID Authentication
echo ============================================================
echo Configuration: %CONFIG%
echo Platform: %PLATFORM%
echo.

REM Find Visual Studio 2025 installation (v18.x)
set DEVENV="C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\devenv.com"

if not exist %DEVENV% (
    echo ERROR: Visual Studio 2025 not found at %DEVENV%
    echo.
    echo Please ensure Visual Studio 2025 Community is installed with:
    echo - Desktop development with C++
    echo - Windows SDK 10.0.22621.0 or later
    echo - Platform Toolset v145
    echo.
    exit /b 1
)

REM Clean previous build artifacts
echo Cleaning previous build...
if exist "x64\%CONFIG%" rmdir /s /q "x64\%CONFIG%" 2>nul

REM Build the solution
echo.
echo ============================================================
echo Building solution: EIDCredentialProvider.sln
echo ============================================================
%DEVENV% EIDCredentialProvider.sln /Rebuild "%CONFIG%|%PLATFORM%" /Out build.log

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ============================================================
    echo BUILD FAILED with error code %ERRORLEVEL%
    echo ============================================================
    echo.
    echo Check build.log for detailed error information
    exit /b %ERRORLEVEL%
)

echo.
echo ============================================================
echo BUILD SUCCEEDED
echo ============================================================
echo.
echo Output files in x64\%CONFIG%:
dir /b x64\%CONFIG%\*.dll x64\%CONFIG%\*.exe 2>nul

REM Display file sizes for verification
echo.
echo File sizes:
for %%f in (x64\%CONFIG%\*.dll x64\%CONFIG%\*.exe) do (
    echo   %%~nxf - %%~zf bytes
)

REM Build installer (Release only)
if "%CONFIG%"=="Release" (
    echo.
    echo ============================================================
    echo Building NSIS Installer
    echo ============================================================

    if exist "C:\Program Files (x86)\NSIS\makensis.exe" (
        REM Delete old installer
        if exist "Installer\EIDInstallx64.exe" del /q "Installer\EIDInstallx64.exe"

        pushd Installer
        "C:\Program Files (x86)\NSIS\makensis.exe" Installerx64.nsi
        popd

        if exist Installer\EIDInstallx64.exe (
            echo.
            echo ============================================================
            echo INSTALLER BUILD SUCCEEDED
            echo ============================================================
            echo Output: Installer\EIDInstallx64.exe
            for %%f in (Installer\EIDInstallx64.exe) do (
                echo Size: %%~zf bytes
            )
            echo.
            echo Installer includes:
            echo   - Certificate cleanup script (CleanupCertificates.ps1)
            echo   - All DLLs and executables
            echo   - Uninstaller with certificate removal
            echo.
        ) else (
            echo.
            echo ============================================================
            echo INSTALLER BUILD FAILED
            echo ============================================================
            echo Check Installer\Installerx64.nsi for errors
            exit /b 1
        )
    ) else (
        echo.
        echo WARNING: NSIS not found at "C:\Program Files (x86)\NSIS\makensis.exe"
        echo Skipping installer build
        echo.
        echo To build the installer, install NSIS from https://nsis.sourceforge.io/
    )
)

echo.
echo ============================================================
echo Build Complete
echo ============================================================
echo Configuration: %CONFIG%
echo Platform: %PLATFORM%
echo All components built successfully
echo.

endlocal
