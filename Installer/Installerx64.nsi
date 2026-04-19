;--------------------------------
;Include Modern UI

  !include "MUI2.nsh"
  !include "nsDialogs.nsh"
  !include "X64.nsh"
  !include "WinVer.nsh"
  !include "FileFunc.nsh"

;--------------------------------
;General

  ;Name and file
  Name "EID Authentication"
  OutFile "EIDInstallx64.exe"

  ;Installer icon (optional - copied by build.ps1 if exists)
  Icon "installer.ico"
  UninstallIcon "installer.ico"

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
  !insertmacro MUI_PAGE_FINISH

  ; Custom uninstall page for certificate/cleanup options
  UninstPage custom un.ShowUninstallOptions un.LeaveUninstallOptions

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH
;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"
  !insertmacro MUI_LANGUAGE "French"

;--------------------------------
;Install types
;
;  1 = Core     - application only (existing behaviour)
;  2 = Complete - application + bundled smart-card minidrivers
;                 (MyEID / YubiKey / Idemia IDOne PIV). All minidriver
;                 packages are embedded in the installer at build time -
;                 no internet access is required at install time.

  InstType "Core"
  InstType "Complete"

;--------------------------------
;Variables for size calculation

  Var /GLOBAL InstallSize

;--------------------------------
;Uninstaller Variables

  Var /GLOBAL Uninstall_RemoveMappings
  Var /GLOBAL Uninstall_RemoveCertificates

;--------------------------------
;Installer Sections

Section "Core" SecCore
  SectionIn RO 1 2

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

  FILE "..\x64\Release\EIDMigrate.exe"
  Push "$INSTDIR\EIDMigrate.exe"
  Call AddFileSize

  FILE "..\x64\Release\EIDMigrateUI.exe"
  Push "$INSTDIR\EIDMigrateUI.exe"
  Call AddFileSize

  FILE "..\x64\Release\EIDManageUsers.exe"
  Push "$INSTDIR\EIDManageUsers.exe"
  Call AddFileSize

  FILE "..\x64\Release\EIDTraceConsumer.exe"
  Push "$INSTDIR\EIDTraceConsumer.exe"
  Call AddFileSize

  ; Install icon for DisplayIcon (installed programs list)
  FILE "cred_provider.ico"

  ; Install Group Policy administrative templates (ADMX/ADML) so the
  ; custom EID Authentication policies appear in gpedit.msc.
  ; Destination: %WINDIR%\PolicyDefinitions (picked up by Group Policy
  ; Editor automatically on next launch).
  DetailPrint "Installing Group Policy templates..."
  SetOutPath "$WINDIR\PolicyDefinitions"
  File "PolicyDefinitions\EIDAuthentication.admx"
  SetOutPath "$WINDIR\PolicyDefinitions\en-US"
  File "PolicyDefinitions\en-US\EIDAuthentication.adml"
  SetOutPath "$INSTDIR"

  ; Install manual-run administrator tools. Disable-LsaProtection.ps1 must
  ; NOT be executed by the installer - the sysadmin has to read the warning
  ; page and type a confirmation phrase. Ship it under $INSTDIR\tools\ so
  ; it is always available locally after install.
  DetailPrint "Installing administrator tools..."
  SetOutPath "$INSTDIR\tools"
  File "tools\Disable-LsaProtection.ps1"
  SetOutPath "$INSTDIR"

  ; Copy DLLs to System32 (required for LSA and Credential Provider)
  ${DisableX64FSRedirection}
  ; Use /REBOOTOK to handle locked files (LSA loads DLLs at boot only)
  Delete /REBOOTOK "$SYSDIR\EIDAuthenticationPackage.dll"
  Delete /REBOOTOK "$SYSDIR\EIDCredentialProvider.dll"
  Delete /REBOOTOK "$SYSDIR\EIDPasswordChangeNotification.dll"

  CopyFiles /SILENT "$INSTDIR\EIDAuthenticationPackage.dll" "$SYSDIR\EIDAuthenticationPackage.dll"
  CopyFiles /SILENT "$INSTDIR\EIDCredentialProvider.dll" "$SYSDIR\EIDCredentialProvider.dll"
  CopyFiles /SILENT "$INSTDIR\EIDPasswordChangeNotification.dll" "$SYSDIR\EIDPasswordChangeNotification.dll"

  ; Create Start Menu folder and shortcuts for all executables
  CreateDirectory "$SMPROGRAMS\EID Authentication"
  CreateShortcut "$SMPROGRAMS\EID Authentication\Configuration Wizard.lnk" "$INSTDIR\EIDConfigurationWizard.exe" "" "$INSTDIR\EIDConfigurationWizard.exe" 0
  CreateShortcut "$SMPROGRAMS\EID Authentication\Log Manager.lnk" "$INSTDIR\EIDLogManager.exe" "" "$INSTDIR\EIDLogManager.exe" 0
  CreateShortcut "$SMPROGRAMS\EID Authentication\Credential Migration (CLI).lnk" "$INSTDIR\EIDMigrate.exe" "" "$INSTDIR\EIDMigrate.exe" 0
  CreateShortcut "$SMPROGRAMS\EID Authentication\Credential Migration (GUI).lnk" "$INSTDIR\EIDMigrateUI.exe" "" "$INSTDIR\EIDMigrateUI.exe" 0
  CreateShortcut "$SMPROGRAMS\EID Authentication\Manage Users.lnk" "$INSTDIR\EIDManageUsers.exe" "" "$INSTDIR\EIDManageUsers.exe" 0
  CreateShortcut "$SMPROGRAMS\EID Authentication\Trace Consumer.lnk" "$INSTDIR\EIDTraceConsumer.exe" "" "$INSTDIR\EIDTraceConsumer.exe" 0
  ; Elevated PowerShell shortcut for the LSA Protection toggle script.
  ; Uses -NoExit so the warning page and outcome remain visible after the
  ; script returns; ExecutionPolicy Bypass scoped to this process only.
  CreateShortcut "$SMPROGRAMS\EID Authentication\Disable LSA Protection (manual).lnk" \
    "powershell.exe" \
    '-NoProfile -NoExit -ExecutionPolicy Bypass -File "$INSTDIR\tools\Disable-LsaProtection.ps1"' \
    "$INSTDIR\cred_provider.ico" 0 SW_SHOWNORMAL "" \
    "Manually disable Windows LSA Protection so unsigned EID DLLs can load. Reads a warning page and requires confirmation."
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
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EIDAuthentication" "DisplayIcon" "$INSTDIR\cred_provider.ico"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EIDAuthentication" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EIDAuthentication" "NoRepair" 1

  ; Convert total install size from bytes to KB and write to registry
  IntOp $InstallSize $InstallSize / 1024
  ; Add ~100 KB for uninstaller and directory structures
  IntOp $InstallSize $InstallSize + 100
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\EIDAuthentication" "EstimatedSize" $InstallSize

  ; Register authentication package (from System32)
  ExecWait 'rundll32.exe "$SYSDIR\EIDAuthenticationPackage.dll",DllRegister'

  ; Configure Smart Card services to start automatically on boot. The
  ; default state on Windows is "Manual (Trigger Start)" which only
  ; starts the services when a reader is already attached - if the user
  ; plugs the reader in after logon, or sign-in needs the service before
  ; any trigger has fired, the Credential Provider will not see any
  ; readers. Forcing start= auto ensures SCardSvr and its device
  ; enumerator are running before the logon UI appears.
  DetailPrint "Configuring Smart Card services for automatic startup..."
  nsExec::ExecToLog 'sc config SCardSvr start= auto'
  nsExec::ExecToLog 'sc config ScDeviceEnum start= auto'
  nsExec::ExecToLog 'net start SCardSvr'
  nsExec::ExecToLog 'net start ScDeviceEnum'

  SetPluginUnload manual
  SetRebootFlag true

SectionEnd

;--------------------------------
;Smart Card Minidrivers  (Complete install type)
;
;  These sections install vendor smart-card minidrivers that are
;  BUNDLED INTO THE INSTALLER at build time. No network access is
;  required at install time - suitable for isolated / air-gapped
;  deployments.
;
;  Each vendor package is extracted into $PLUGINSDIR (auto-cleaned
;  on installer exit) and installed with the appropriate tool:
;    - MyEID  (ZIP containing INF+DLL+CAT)  -> Expand-Archive + pnputil -i -a
;    - YubiKey (signed MSI)                  -> msiexec /i /qn /norestart
;    - IDOne PIV (CAB from Windows Update)   -> expand.exe + pnputil -i -a
;
;  Failures are logged as warnings and do not abort the Core install.
;
;  The bundled files live in Installer\drivers\ and are staged by
;  build.ps1 (download + SHA-256 verification). See the README.md
;  in that directory.

SectionGroup /e "Smart Card Minidrivers" SecMinidrivers

Section /o "MyEID Minidriver (Aventra)" SecMyEIDMinidriver
  SectionIn 2

  InitPluginsDir
  SetOutPath "$PLUGINSDIR\MyEID"
  File "drivers\MyEID_Minidriver.zip"

  DetailPrint "Extracting MyEID Minidriver..."
  nsExec::ExecToLog 'powershell.exe -NoProfile -ExecutionPolicy Bypass -Command ' \
    'try { Expand-Archive -LiteralPath "$PLUGINSDIR\MyEID\MyEID_Minidriver.zip" -DestinationPath "$PLUGINSDIR\MyEID\x" -Force; exit 0 } ' \
    'catch { Write-Error $_.Exception.Message; exit 1 }' \
    ''
  Pop $0
  ${If} $0 != 0
    DetailPrint "WARNING: MyEID extraction failed (code $0) - skipping"
    Goto MyEIDDone
  ${EndIf}

  DetailPrint "Installing MyEID Minidriver (pnputil)..."
  nsExec::ExecToLog 'powershell.exe -NoProfile -ExecutionPolicy Bypass -Command ' \
    '$inf = Get-ChildItem -Path "$PLUGINSDIR\MyEID\x" -Recurse -Filter *.inf -ErrorAction SilentlyContinue | Select-Object -First 1; ' \
    'if (-not $inf) { Write-Error "No INF found in MyEID archive"; exit 2 }; ' \
    '& pnputil.exe -i -a $inf.FullName | Out-Host; ' \
    'exit $LASTEXITCODE' \
    ''
  Pop $0
  ${If} $0 = 0
    DetailPrint "MyEID Minidriver installed successfully"
  ${Else}
    DetailPrint "WARNING: MyEID Minidriver install returned code $0"
  ${EndIf}

MyEIDDone:
SectionEnd

Section /o "YubiKey Minidriver (Yubico)" SecYubiKeyMinidriver
  SectionIn 2

  InitPluginsDir
  SetOutPath "$PLUGINSDIR\YubiKey"
  File "drivers\YubiKey-Minidriver-5.0.4.273-x64.msi"

  DetailPrint "Installing YubiKey Minidriver (msiexec)..."
  nsExec::ExecToLog 'msiexec.exe /i "$PLUGINSDIR\YubiKey\YubiKey-Minidriver-5.0.4.273-x64.msi" /qn /norestart'
  Pop $0
  ${If} $0 = 0
    DetailPrint "YubiKey Minidriver installed successfully"
  ${ElseIf} $0 = 3010
    DetailPrint "YubiKey Minidriver installed successfully (reboot required)"
    SetRebootFlag true
  ${Else}
    DetailPrint "WARNING: YubiKey Minidriver install returned code $0"
  ${EndIf}
SectionEnd

Section /o "IDOne PIV Minidriver (Idemia / Windows Update)" SecWUMinidriver
  SectionIn 2

  InitPluginsDir
  SetOutPath "$PLUGINSDIR\WU"
  File "drivers\WindowsUpdate_Minidriver.cab"
  CreateDirectory "$PLUGINSDIR\WU\x"

  DetailPrint "Extracting CAB contents..."
  nsExec::ExecToLog 'expand.exe -F:* "$PLUGINSDIR\WU\WindowsUpdate_Minidriver.cab" "$PLUGINSDIR\WU\x"'
  Pop $0
  ${If} $0 != 0
    DetailPrint "WARNING: CAB extraction failed (code $0) - skipping"
    Goto WUDone
  ${EndIf}

  DetailPrint "Adding INF driver(s) to the driver store..."
  nsExec::ExecToLog 'powershell.exe -NoProfile -ExecutionPolicy Bypass -Command ' \
    '$infs = Get-ChildItem -Path "$PLUGINSDIR\WU\x" -Recurse -Filter *.inf -ErrorAction SilentlyContinue; ' \
    'if (-not $infs) { Write-Error "No INF found in CAB"; exit 2 }; ' \
    'foreach ($inf in $infs) { Write-Host ("Installing: " + $inf.FullName); & pnputil.exe -i -a $inf.FullName | Out-Host }; ' \
    'exit 0' \
    ''
  Pop $0
  ${If} $0 = 0
    DetailPrint "IDOne PIV Minidriver installed successfully"
  ${Else}
    DetailPrint "WARNING: IDOne PIV Minidriver install returned code $0"
  ${EndIf}

WUDone:
SectionEnd

SectionGroupEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecCore ${LANG_ENGLISH} "Core EID Authentication components: LSA Authentication Package, Credential Provider, Configuration Wizard, Log Manager, Migrate CLI/UI, and Manage Users tool. Always installed."
  LangString DESC_SecCore ${LANG_FRENCH}  "Composants principaux EID Authentication: LSA, Credential Provider, assistant de configuration et outils associes. Toujours installes."

  LangString DESC_SecMinidrivers ${LANG_ENGLISH} "Smart card minidrivers bundled with the installer. No internet access required at install time. Auto-selected for the Complete install type."
  LangString DESC_SecMinidrivers ${LANG_FRENCH}  "Minidrivers de carte a puce fournis avec l'installateur. Aucun acces Internet requis. Selectionnes automatiquement pour l'installation Complete."

  LangString DESC_SecMyEID ${LANG_ENGLISH} "Aventra MyEID minidriver v3.0.1.2 (Certified). Bundled in the installer; extracts and installs via pnputil."
  LangString DESC_SecMyEID ${LANG_FRENCH}  "Minidriver Aventra MyEID v3.0.1.2 (Certifie). Inclus dans l'installateur; extrait et installe via pnputil."

  LangString DESC_SecYubiKey ${LANG_ENGLISH} "YubiKey Smart Card Minidriver 5.0.4.273 (x64). Bundled signed MSI installed silently via msiexec."
  LangString DESC_SecYubiKey ${LANG_FRENCH}  "Minidriver YubiKey 5.0.4.273 (x64). MSI signe inclus, installe silencieusement via msiexec."

  LangString DESC_SecWU ${LANG_ENGLISH} "IDOne PIV minidriver from the Microsoft Update catalog. Bundled signed CAB; extracted and added to the driver store via pnputil."
  LangString DESC_SecWU ${LANG_FRENCH}  "Minidriver IDOne PIV du catalogue Microsoft Update. CAB signe inclus; extrait et ajoute au magasin de pilotes via pnputil."

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecCore}              $(DESC_SecCore)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecMinidrivers}       $(DESC_SecMinidrivers)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecMyEIDMinidriver}   $(DESC_SecMyEID)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecYubiKeyMinidriver} $(DESC_SecYubiKey)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecWUMinidriver}      $(DESC_SecWU)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Helper Functions for Certificate Cleanup

Function un.ShowUninstallOptions
  ; Create custom page with checkboxes for uninstall options
  nsDialogs::Create /NOUNLOAD 1018
  Pop $0

  ${NSD_CreateLabel} 0 0 100% 40u "Select additional cleanup options:"

  ; Checkbox for removing EID certificate mappings from users (LSA credentials)
  ${NSD_CreateCheckbox} 10u 50u 100% 12u "Remove EID certificate mappings from users"
  Pop $Uninstall_RemoveMappings
  ${NSD_Check} $Uninstall_RemoveMappings

  ; Checkbox for removing EID root certificates
  ${NSD_CreateCheckbox} 10u 70u 100% 12u "Remove EID Root Certificate Authority and user certificates (only those with 'EID:' prefix)"
  Pop $Uninstall_RemoveCertificates
  ${NSD_Check} $Uninstall_RemoveCertificates

  nsDialogs::Show
FunctionEnd

Function un.LeaveUninstallOptions
  ; Get the state of checkboxes when leaving the page
  ${NSD_GetState} $Uninstall_RemoveMappings $Uninstall_RemoveMappings
  ${NSD_GetState} $Uninstall_RemoveCertificates $Uninstall_RemoveCertificates
FunctionEnd

Function un.RemoveEIDCertificates
  ; This function removes certificates created by EID Authentication
  ; Checks for "EID:" prefix in certificate subject names

  DetailPrint "Removing EID certificates (those with 'EID:' prefix)..."

  ; Use PowerShell inline command to avoid NSIS escaping issues
  ; Single quotes in PowerShell preserve literal strings
  nsExec::ExecToLog 'powershell.exe -NoProfile -ExecutionPolicy Bypass -Command ' \
    '$ErrorActionPreference="SilentlyContinue"; ' \
    '$count=0; ' \
    '$stores=@(@("My","CurrentUser"),@("TrustedPeople","CurrentUser"),@("Root","CurrentUser"),@("Root","LocalMachine")); ' \
    'foreach($s in $stores) { ' \
    '  $store=New-Object System.Security.Cryptography.X509Certificates.X509Store($s[0],$s[1]); ' \
    '  $store.Open("ReadWrite"); ' \
    '  foreach($c in $store.Certificates) { ' \
    '    if($c.Subject -like "*EID:*") { ' \
    '      Write-Host ("Removing: " + $c.Subject + " - " + $c.Thumbprint); ' \
    '      $store.Remove($c); ' \
    '      $count++; ' \
    '    } ' \
    '  } ' \
    '  $store.Close(); ' \
    '}; ' \
    'Write-Host ("Total EID certificates removed: $count")' \
    ''

FunctionEnd

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

  ${EnableX64FSRedirection}

  ; Conditionally remove certificates created by the software (if checkbox was selected)
  ${If} $Uninstall_RemoveCertificates = 1
    DetailPrint "Removing EID Root Certificates..."
    Call un.RemoveEIDCertificates
  ${Else}
    DetailPrint "Skipping certificate removal (not selected)"
  ${EndIf}

  ; Conditionally remove EID credential mappings from LSA Private Data (if checkbox was selected)
  ${If} $Uninstall_RemoveMappings = 1
    ; This requires calling into the DLL since NSIS cannot directly manipulate LSA
    DetailPrint "Removing EID credential mappings from LSA..."
    ${DisableX64FSRedirection}
    ExecWait 'rundll32.exe "$SYSDIR\EIDAuthenticationPackage.dll",CleanupLsaCredentials' $1
    ${If} $1 != 0
      DetailPrint "Note: LSA cleanup returned code $1 (may be expected if not installed)"
    ${EndIf}
    ${EnableX64FSRedirection}
  ${Else}
    DetailPrint "Skipping LSA credential mapping removal (not selected)"
  ${EndIf}

  ; Delete Start Menu shortcuts and folder
  Delete "$SMPROGRAMS\EID Authentication\Configuration Wizard.lnk"
  Delete "$SMPROGRAMS\EID Authentication\Log Manager.lnk"
  Delete "$SMPROGRAMS\EID Authentication\Credential Migration (CLI).lnk"
  Delete "$SMPROGRAMS\EID Authentication\Credential Migration (GUI).lnk"
  Delete "$SMPROGRAMS\EID Authentication\Manage Users.lnk"
  Delete "$SMPROGRAMS\EID Authentication\Trace Consumer.lnk"
  Delete "$SMPROGRAMS\EID Authentication\Disable LSA Protection (manual).lnk"
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

  ; Remove Group Policy administrative templates
  Delete "$WINDIR\PolicyDefinitions\EIDAuthentication.admx"
  Delete "$WINDIR\PolicyDefinitions\en-US\EIDAuthentication.adml"

  ; Delete Program Files installation - DLLs
  Delete "$INSTDIR\EIDAuthenticationPackage.dll"
  Delete "$INSTDIR\EIDCredentialProvider.dll"
  Delete "$INSTDIR\EIDPasswordChangeNotification.dll"

  ; Delete Program Files installation - Executables
  Delete "$INSTDIR\EIDConfigurationWizard.exe"
  Delete "$INSTDIR\EIDConfigurationWizardElevated.exe"
  Delete "$INSTDIR\EIDLogManager.exe"
  Delete "$INSTDIR\EIDMigrate.exe"
  Delete "$INSTDIR\EIDMigrateUI.exe"
  Delete "$INSTDIR\EIDManageUsers.exe"
  Delete "$INSTDIR\EIDTraceConsumer.exe"
  Delete "$INSTDIR\cred_provider.ico"

  ; Delete administrator tools
  Delete "$INSTDIR\tools\Disable-LsaProtection.ps1"
  RMDir "$INSTDIR\tools"

  ; Delete uninstaller
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

  ; Restore Smart Card services to their Windows default (demand / trigger
  ; start). The installer forced these to auto-start; leave them stopped
  ; and revert to demand so Windows' trigger-start behaviour takes over.
  DetailPrint "Restoring Smart Card services to default startup..."
  nsExec::ExecToLog 'sc config SCardSvr start= demand'
  nsExec::ExecToLog 'sc config ScDeviceEnum start= demand'

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
