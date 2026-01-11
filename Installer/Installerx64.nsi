
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
;Installer Sections

Section "Core" SecCore
  SectionIn RO

  ; Create installation directory
  SetOutPath "$INSTDIR"

  ; Install files to Program Files
  FILE "..\x64\Release\EIDAuthenticationPackage.dll"
  FILE "..\x64\Release\EIDCredentialProvider.dll"
  FILE "..\x64\Release\EIDPasswordChangeNotification.dll"
  FILE "..\x64\Release\EIDConfigurationWizard.exe"

  ; Copy DLLs to System32 (required for LSA and Credential Provider)
  ${DisableX64FSRedirection}
  CopyFiles "$INSTDIR\EIDAuthenticationPackage.dll" "$SYSDIR\EIDAuthenticationPackage.dll"
  CopyFiles "$INSTDIR\EIDCredentialProvider.dll" "$SYSDIR\EIDCredentialProvider.dll"
  CopyFiles "$INSTDIR\EIDPasswordChangeNotification.dll" "$SYSDIR\EIDPasswordChangeNotification.dll"

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
  ExecWait 'rundll32.exe "$SYSDIR\EIDAuthenticationPackage.dll",DllUnRegister'

  ; Delete desktop shortcut
  Delete "$DESKTOP\EID Authentication Configuration.lnk"

  ; Delete System32 files (LSA-locked, require reboot)
  Delete /REBOOTOK "$SYSDIR\EIDAuthenticationPackage.dll"
  Delete /REBOOTOK "$SYSDIR\EIDCredentialProvider.dll"
  Delete /REBOOTOK "$SYSDIR\EIDPasswordChangeNotification.dll"

  ; Delete Program Files installation
  Delete "$INSTDIR\EIDAuthenticationPackage.dll"
  Delete "$INSTDIR\EIDCredentialProvider.dll"
  Delete "$INSTDIR\EIDPasswordChangeNotification.dll"
  Delete "$INSTDIR\EIDConfigurationWizard.exe"
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

  ; Remove smart card removal policy keys (if any exist)
  DeleteRegKey HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon\SmartCardRemovalPolicy"

  ; Remove uninstall information
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EIDAuthentication"

  SetPluginUnload manual
  SetRebootFlag true

  MessageBox MB_OK "EID Authentication has been uninstalled. Please reboot your computer to complete the removal."

SectionEnd


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