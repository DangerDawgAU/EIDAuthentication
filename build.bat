@echo off
REM Build EIDAuthentication Project
REM Usage: build.bat [Debug|Release] [x64|Win32]

setlocal

REM Set default configuration and platform
set CONFIG=%1
set PLATFORM=%2

if "%CONFIG%"=="" set CONFIG=Release
if "%PLATFORM%"=="" set PLATFORM=x64

echo Building EID Authentication...
echo Configuration: %CONFIG%
echo Platform: %PLATFORM%
echo.

REM Find Visual Studio installation
set DEVENV="C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\devenv.com"

if not exist %DEVENV% (
    echo ERROR: Visual Studio 2025 not found at %DEVENV%
    exit /b 1
)

REM Build the solution
echo Running build...
%DEVENV% EIDCredentialProvider.sln /Rebuild "%CONFIG%|%PLATFORM%"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo BUILD FAILED with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo.
echo BUILD SUCCEEDED
echo.
echo Output files:
dir /b x64\%CONFIG%\*.dll x64\%CONFIG%\*.exe 2>nul

endlocal
