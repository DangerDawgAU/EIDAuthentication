;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"
  !include "X64.nsh"
  !include "WinVer.nsh"
  !include "FileFunc.nsh"

;--------------------------------
;General

  ;Name and file
  Name "EID Authentication"
  OutFile "EIDInstall.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\EID Authentication"

  ;Get installation folder from registry if available
  InstallDirRegKey HKLM "Software\EIDAuthentication" "InstallPath"

  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_LICENSE "License.txt"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_INSTFILES

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH
  !insertmacro MUI_UNPAGE_FINISH
;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"
  !insertmacro MUI_LANGUAGE "French"

;--------------------------------
;Variables for size calculation

  Var /GLOBAL InstallSize

;--------------------------------
;Installer Sections

Section "Core" SecCore
  SectionIn RO

  ; Initialize install size counter
  StrCpy $InstallSize 0

  ; Create installation directory
  SetOutPath "$INSTDIR"

  ; Install DLL files to Program Files
  FILE "..\Release\EIDAuthenticationPackage.dll"
  Push "$INSTDIR\EIDAuthenticationPackage.dll"
  Call AddFileSize

  FILE "..\Release\EIDCredentialProvider.dll"
  Push "$INSTDIR\EIDCredentialProvider.dll"
  Call AddFileSize

  FILE "..\Release\EIDPasswordChangeNotification.dll"
  Push "$INSTDIR\EIDPasswordChangeNotification.dll"
  Call AddFileSize

  ; Install all executable files
  FILE "..\Release\EIDConfigurationWizard.exe"
  Push "$INSTDIR\EIDConfigurationWizard.exe"
  Call AddFileSize

  ; Copy DLLs to System32 (required for LSA and Credential Provider)
  CopyFiles "$INSTDIR\EIDAuthenticationPackage.dll" "$SYSDIR\EIDAuthenticationPackage.dll"
  CopyFiles "$INSTDIR\EIDCredentialProvider.dll" "$SYSDIR\EIDCredentialProvider.dll"
  CopyFiles "$INSTDIR\EIDPasswordChangeNotification.dll" "$SYSDIR\EIDPasswordChangeNotification.dll"

  ; Create Start Menu folder and shortcuts
  CreateDirectory "$SMPROGRAMS\EID Authentication"
  CreateShortcut "$SMPROGRAMS\EID Authentication\Configuration Wizard.lnk" "$INSTDIR\EIDConfigurationWizard.exe" "" "$INSTDIR\EIDConfigurationWizard.exe" 0
  CreateShortcut "$SMPROGRAMS\EID Authentication\Uninstall.lnk" "$INSTDIR\EIDUninstall.exe" "" "$INSTDIR\EIDUninstall.exe" 0

  ; Create desktop shortcut pointing to Program Files
  CreateShortcut "$DESKTOP\EID Authentication Configuration.lnk" "$INSTDIR\EIDConfigurationWizard.exe"

  ; Create uninstaller in installation directory
  WriteUninstaller "$INSTDIR\EIDUninstall.exe"

  ; Write installation path to registry
  WriteRegStr HKLM "Software\EIDAuthentication" "InstallPath" "$INSTDIR"

  ; Uninstall info
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EIDAuthentication" "DisplayName" "EID Authentication"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EIDAuthentication" "UninstallString" "$INSTDIR\EIDUninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EIDAuthentication" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EIDAuthentication" "Publisher" "EID Authentication"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EIDAuthentication" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EIDAuthentication" "NoRepair" 1

  ; Convert total install size from bytes to KB and write to registry
  IntOp $InstallSize $InstallSize / 1024
  ; Add ~100 KB for uninstaller and directory structures
  IntOp $InstallSize $InstallSize + 100
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EIDAuthentication" "EstimatedSize" $InstallSize

  ; Register authentication package (from System32)
  ExecWait 'rundll32.exe "$SYSDIR\EIDAuthenticationPackage.dll",DllRegister'

  ; Enable and start Smart Card services (required for smart card functionality)
  DetailPrint "Configuring Smart Card services..."
  nsExec::ExecToLog 'sc config SCardSvr start= demand'
  nsExec::ExecToLog 'sc config ScDeviceEnum start= demand'
  nsExec::ExecToLog 'net start SCardSvr'
  nsExec::ExecToLog 'net start ScDeviceEnum'

  SetPluginUnload manual
  SetRebootFlag true

SectionEnd

Section /o "Belgium EID Patch" SecBeid

  ExecWait 'rundll32.exe "$SYSDIR\EIDAuthenticationPackage.dll",EIDPatch'

SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecCore ${LANG_ENGLISH} "Core"
  LangString DESC_SecCore ${LANG_FRENCH} "Core"

  LangString DESC_SecBeid ${LANG_ENGLISH} "Insert missing configuration parameters required to use Belgium EID Card - the Belgium middleware must be installed !"
  LangString DESC_SecBeid ${LANG_FRENCH} "Ins�re des param�tres de configuration n�cessaires pour l'utilisation de la carte d'identit� belge - le middleware doit �tre install� !"

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} $(DESC_SecCore)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecBeid} $(DESC_SecBeid)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Helper Functions for Certificate Cleanup

Function RemoveEIDCertificates
  ; This function removes certificates created by EID Authentication
  ; We use certutil.exe which is built into Windows

  DetailPrint "Removing EID Authentication certificates..."

  ; Create a temporary batch file to do the certificate cleanup
  GetTempFileName $0 "$TEMP"
  FileOpen $1 $0 "w"
  FileWrite $1 "@echo off$\r$\n"
  FileWrite $1 "setlocal enabledelayedexpansion$\r$\n"
  FileWrite $1 "echo Removing EID Authentication certificates...$\r$\n"
  FileWrite $1 "$\r$\n"

  ; Remove from Root store
  FileWrite $1 "echo Checking Root store...$\r$\n"
  FileWrite $1 "for /f $\"tokens=* delims=$\" %%a in ('certutil -store Root 2^>nul ^| findstr /i $\"EID Authentication My Smart Logon$\"') do ($\r$\n"
  FileWrite $1 "  for /f $\"tokens=1$\" %%b in ($\"%%a$\") do ($\r$\n"
  FileWrite $1 "    certutil -delstore Root %%b ^>nul 2^>^&1 && echo   Removed: %%b$\r$\n"
  FileWrite $1 "  )$\r$\n"
  FileWrite $1 ")$\r$\n"
  FileWrite $1 "$\r$\n"

  ; Remove from MY store (LocalMachine)
  FileWrite $1 "echo Checking MY store...$\r$\n"
  FileWrite $1 "for /f $\"tokens=* delims=$\" %%a in ('certutil -store My 2^>nul ^| findstr /i $\"EID Authentication My Smart Logon$\"') do ($\r$\n"
  FileWrite $1 "  for /f $\"tokens=1$\" %%b in ($\"%%a$\") do ($\r$\n"
  FileWrite $1 "    certutil -delstore My %%b ^>nul 2^>^&1 && echo   Removed: %%b$\r$\n"
  FileWrite $1 "  )$\r$\n"
  FileWrite $1 ")$\r$\n"
  FileWrite $1 "$\r$\n"

  ; Remove from TrustedPeople store
  FileWrite $1 "echo Checking TrustedPeople store...$\r$\n"
  FileWrite $1 "for /f $\"tokens=* delims=$\" %%a in ('certutil -store TrustedPeople 2^>nul ^| findstr /i $\"EID Authentication My Smart Logon$\"') do ($\r$\n"
  FileWrite $1 "  for /f $\"tokens=1$\" %%b in ($\"%%a$\") do ($\r$\n"
  FileWrite $1 "    certutil -delstore TrustedPeople %%b ^>nul 2^>^&1 && echo   Removed: %%b$\r$\n"
  FileWrite $1 "  )$\r$\n"
  FileWrite $1 ")$\r$\n"
  FileWrite $1 "$\r$\n"

  FileWrite $1 "echo Certificate cleanup complete.$\r$\n"
  FileWrite $1 "endlocal$\r$\n"
  FileClose $1

  ; Execute the batch file
  nsExec::ExecToLog 'cmd.exe /c "$0"'
  Pop $2

  ; Delete the temporary file
  Delete $0

FunctionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ; Unregister all components first (from System32)
  DetailPrint "Unregistering components..."
  ExecWait 'rundll32.exe "$SYSDIR\EIDAuthenticationPackage.dll",DllUnRegister' $0
  ${If} $0 != 0
    DetailPrint "Warning: DllUnRegister returned error code $0 - continuing with manual cleanup"
  ${EndIf}

  ; Remove certificates created by the software
  Call RemoveEIDCertificates

  ; Remove EID credential mappings from LSA Private Data
  DetailPrint "Removing EID credential mappings from LSA..."
  ExecWait 'rundll32.exe "$SYSDIR\EIDAuthenticationPackage.dll",CleanupLsaCredentials' $1
  ${If} $1 != 0
    DetailPrint "Note: LSA cleanup returned code $1 (may be expected if not installed)"
  ${EndIf}

  ; Remove Belgium EID patch if applied
  ExecWait 'rundll32.exe "$SYSDIR\EIDAuthenticationPackage.dll",EIDUnPatch'

  ; Delete Start Menu shortcuts and folder
  Delete "$SMPROGRAMS\EID Authentication\Configuration Wizard.lnk"
  Delete "$SMPROGRAMS\EID Authentication\Uninstall.lnk"
  RMDir "$SMPROGRAMS\EID Authentication"

  ; Delete desktop shortcut
  Delete "$DESKTOP\EID Authentication Configuration.lnk"

  ; Delete System32 files (LSA-locked, require reboot)
  Delete /REBOOTOK "$SYSDIR\EIDAuthenticationPackage.dll"
  Delete /REBOOTOK "$SYSDIR\EIDCredentialProvider.dll"
  Delete /REBOOTOK "$SYSDIR\EIDPasswordChangeNotification.dll"

  ; Delete ETW log files
  Delete /REBOOTOK "$SYSDIR\LogFiles\WMI\EIDCredentialProvider.etl"

  ; Delete Program Files installation - DLLs
  Delete "$INSTDIR\EIDAuthenticationPackage.dll"
  Delete "$INSTDIR\EIDCredentialProvider.dll"
  Delete "$INSTDIR\EIDPasswordChangeNotification.dll"

  ; Delete Program Files installation - Executables
  Delete "$INSTDIR\EIDConfigurationWizard.exe"

  ; Delete uninstaller
  Delete "$INSTDIR\EIDUninstall.exe"

  ; Remove installation directory
  RMDir "$INSTDIR"

  ; Remove registry keys

  ; Remove installation path registry
  DeleteRegKey HKLM "Software\EIDAuthentication"

  ; Remove Credential Provider registry keys
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\Credential Providers\{B4866A0A-DB08-4835-A26F-414B46F3244C}"
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Authentication\Credential Provider Filters\{B4866A0A-DB08-4835-A26F-414B46F3244C}"
  DeleteRegKey HKCR "CLSID\{B4866A0A-DB08-4835-A26F-414B46F3244C}"

  ; Remove Configuration Wizard registry keys
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\ControlPanel\NameSpace\{F5D846B4-14B0-11DE-B23C-27A355D89593}"
  DeleteRegKey HKCR "CLSID\{F5D846B4-14B0-11DE-B23C-27A355D89593}"

  ; Remove WMI Autologger for EIDCredentialProvider
  DeleteRegKey HKLM "SYSTEM\CurrentControlSet\Control\WMI\Autologger\EIDCredentialProvider"

  ; Remove crash dump configuration for lsass.exe
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\lsass.exe"

  ; Remove GPO policy values set by the Configuration Wizard
  DeleteRegKey HKLM "SOFTWARE\Policies\Microsoft\Windows\SmartCardCredentialProvider"
  DeleteRegValue HKLM "Software\Microsoft\Windows NT\CurrentVersion\Winlogon" "scremoveoption"
  DeleteRegValue HKLM "Software\Microsoft\Windows\CurrentVersion\Policies\System" "scforceoption"

  ; Reset ScPolicySvc service to demand-start (installer may have set it to auto-start)
  DetailPrint "Resetting Smart Card Removal Policy service..."
  nsExec::ExecToLog 'sc config ScPolicySvc start= demand'
  nsExec::ExecToLog 'sc stop ScPolicySvc'

  ; Remove uninstall information
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EIDAuthentication"

  SetPluginUnload manual
  SetRebootFlag true

  MessageBox MB_OK "EID Authentication has been uninstalled. Please reboot your computer to complete the removal."

SectionEnd

;--------------------------------
;Helper function to calculate file size and add to total

Function AddFileSize
  ; This function receives a file path on the stack
  ; Adds the file size to the $InstallSize variable

  Pop $0  ; File path

  ; Get file size by opening and seeking to end
  FileOpen $1 $0 "r"

  ${If} $1 != ""
    FileSeek $1 0 END $2  ; Seek to end, get position (file size in bytes)
    FileClose $1

    ; Add file size to running total
    IntOp $InstallSize $InstallSize + $2
  ${EndIf}
FunctionEnd

;--------------------------------
;Initializer function

Function .onInit

  ${If} ${RunningX64}
    MessageBox MB_OK "This installer is designed for 32bits only"
    Abort
  ${EndIf}

IfFileExists "$PROGRAMFILES\Belgium Identity Card\beid35libCpp.dll" CheckOk CheckEnd
CheckOk:
  ; This is what is done by sections.nsh SelectSection macro
   !insertmacro SelectSection ${SecBeid}


CheckEnd:

IfFileExists "$INSTDIR\EIDAuthenticationPackage.dll" CheckInstallNotOk CheckInstallEnd
CheckInstallNotOk:
  MessageBox MB_OK "Please uninstall first !"
  Abort

CheckInstallEnd:
FunctionEnd
