
;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"
  !include "X64.nsh"
  !include "WinVer.nsh"

;--------------------------------
;General

  ;Name and file
  Name "EID Authentication"
  OutFile "EIDInstallx64.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES64\EID Authentication"

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
  FILE "..\x64\Release\EIDAuthenticationPackage.dll"
  Push "$INSTDIR\EIDAuthenticationPackage.dll"
  Call AddFileSize

  FILE "..\x64\Release\EIDCredentialProvider.dll"
  Push "$INSTDIR\EIDCredentialProvider.dll"
  Call AddFileSize

  FILE "..\x64\Release\EIDPasswordChangeNotification.dll"
  Push "$INSTDIR\EIDPasswordChangeNotification.dll"
  Call AddFileSize

  ; Install all executable files
  FILE "..\x64\Release\EIDConfigurationWizard.exe"
  Push "$INSTDIR\EIDConfigurationWizard.exe"
  Call AddFileSize

  FILE "..\x64\Release\EIDConfigurationWizardElevated.exe"
  Push "$INSTDIR\EIDConfigurationWizardElevated.exe"
  Call AddFileSize

  FILE "..\x64\Release\EIDLogManager.exe"
  Push "$INSTDIR\EIDLogManager.exe"
  Call AddFileSize

  ; Install support files
  FILE "CleanupCertificates.ps1"
  Push "$INSTDIR\CleanupCertificates.ps1"
  Call AddFileSize

  ; Copy DLLs to System32 (required for LSA and Credential Provider)
  ${DisableX64FSRedirection}
  CopyFiles "$INSTDIR\EIDAuthenticationPackage.dll" "$SYSDIR\EIDAuthenticationPackage.dll"
  CopyFiles "$INSTDIR\EIDCredentialProvider.dll" "$SYSDIR\EIDCredentialProvider.dll"
  CopyFiles "$INSTDIR\EIDPasswordChangeNotification.dll" "$SYSDIR\EIDPasswordChangeNotification.dll"

  ; Create Start Menu folder and shortcuts
  CreateDirectory "$SMPROGRAMS\EID Authentication"
  CreateShortcut "$SMPROGRAMS\EID Authentication\Configuration Wizard.lnk" "$INSTDIR\EIDConfigurationWizard.exe" "" "$INSTDIR\EIDConfigurationWizard.exe" 0
  CreateShortcut "$SMPROGRAMS\EID Authentication\Log Manager.lnk" "$INSTDIR\EIDLogManager.exe" "" "$INSTDIR\EIDLogManager.exe" 0
  CreateShortcut "$SMPROGRAMS\EID Authentication\Uninstall.lnk" "$INSTDIR\EIDUninstall.exe" "" "$INSTDIR\EIDUninstall.exe" 0

  ; Create desktop shortcut pointing to Program Files
  CreateShortcut "$DESKTOP\EID Authentication Configuration.lnk" "$INSTDIR\EIDConfigurationWizard.exe"

  ; Create uninstaller in installation directory
  WriteUninstaller "$INSTDIR\EIDUninstall.exe"

  ; Write installation path to registry
  SetRegView 64
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







;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecCore ${LANG_ENGLISH} "Core"
  LangString DESC_SecCore ${LANG_FRENCH} "Core"

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecCore} $(DESC_SecCore)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ; Unregister all components first (from System32)
  ${DisableX64FSRedirection}
  DetailPrint "Unregistering components..."
  ExecWait 'rundll32.exe "$SYSDIR\EIDAuthenticationPackage.dll",DllUnRegister' $0
  ${If} $0 != 0
    DetailPrint "Warning: DllUnRegister returned error code $0 - continuing with manual cleanup"
  ${EndIf}

  ; Re-enable FS redirection before running PowerShell
  ${EnableX64FSRedirection}

  ; Remove certificates created by the software
  DetailPrint "Removing certificates..."
  nsExec::ExecToLog 'powershell -ExecutionPolicy Bypass -File "$INSTDIR\CleanupCertificates.ps1"'

  ; Delete Start Menu shortcuts and folder
  Delete "$SMPROGRAMS\EID Authentication\Configuration Wizard.lnk"
  Delete "$SMPROGRAMS\EID Authentication\Log Manager.lnk"
  Delete "$SMPROGRAMS\EID Authentication\Uninstall.lnk"
  RMDir "$SMPROGRAMS\EID Authentication"

  ; Delete desktop shortcut
  Delete "$DESKTOP\EID Authentication Configuration.lnk"

  ; Delete System32 files (LSA-locked, require reboot)
  ${DisableX64FSRedirection}
  Delete /REBOOTOK "$SYSDIR\EIDAuthenticationPackage.dll"
  Delete /REBOOTOK "$SYSDIR\EIDCredentialProvider.dll"
  Delete /REBOOTOK "$SYSDIR\EIDPasswordChangeNotification.dll"

  ; Delete ETW log files
  Delete /REBOOTOK "$SYSDIR\LogFiles\WMI\EIDCredentialProvider.etl"

  ${EnableX64FSRedirection}

  ; Delete Program Files installation - DLLs
  Delete "$INSTDIR\EIDAuthenticationPackage.dll"
  Delete "$INSTDIR\EIDCredentialProvider.dll"
  Delete "$INSTDIR\EIDPasswordChangeNotification.dll"

  ; Delete Program Files installation - Executables
  Delete "$INSTDIR\EIDConfigurationWizard.exe"
  Delete "$INSTDIR\EIDConfigurationWizardElevated.exe"
  Delete "$INSTDIR\EIDLogManager.exe"

  ; Delete support files
  Delete "$INSTDIR\CleanupCertificates.ps1"
  Delete "$INSTDIR\EIDUninstall.exe"

  ; Remove installation directory
  RMDir "$INSTDIR"

  ; Remove registry keys
  SetRegView 64

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
  ${Else}
    MessageBox MB_OK "This installer is designed for 64bits only"
    Abort
  ${EndIf}

  ; Check if already installed via registry
  SetRegView 64
  ReadRegStr $0 HKLM "Software\EIDAuthentication" "InstallPath"
  StrCmp $0 "" CheckInstallEnd 0

  ; Installation found - ask user to uninstall first
  MessageBox MB_YESNO "EID Authentication is already installed at:$\n$0$\n$\nDo you want to uninstall it first?" IDYES DoUninstall IDNO AbortInstall

  DoUninstall:
    ; Read uninstaller path
    ReadRegStr $1 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EIDAuthentication" "UninstallString"
    ExecWait '$1'
    Goto CheckInstallEnd

  AbortInstall:
    Abort

  CheckInstallEnd:
FunctionEnd