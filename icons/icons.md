# EID Authentication - Icon Requirements

This document describes all icons required for the EID Authentication project.

## Icon Format Specifications

### Application Icons (.ico files)
All Windows application icons should be provided as `.ico` files containing multiple resolutions:
- **16x16** - Small icons (taskbar, title bar, list views)
- **32x32** - Standard size (desktop shortcuts, file explorer)
- **48x48** - Large icons (control panel, some dialogs)
- **256x256** - Extra large (Vista+ high DPI, detailed views)

Recommended: Use 32-bit PNG-compressed format for 256x256, standard BMP for smaller sizes.

### Credential Provider Tile Image (.bmp file)
**Special format requirement:** The credential provider tile image must be a **48x48 pixel 32-bit BMP** file.
- Format: Windows BMP (BITMAPFILEHEADER with BITMAPINFOHEADER)
- Bit depth: 32-bit (BGRA with alpha channel)
- Size: Exactly 48x48 pixels
- Background: Should have transparent or neutral background
- Note: PNG files must be converted to BMP for use in credential providers

---

## Application Icons (Window & Executable Icons)

These icons are embedded in the executable and displayed in:
- Window title bars
- Taskbar
- Task Manager
- Alt+Tab switcher
- File Explorer

### 1. EIDConfigurationWizard Icon
- **Filename:** `app_configuration_wizard.ico`
- **Used by:**
  - `EIDConfigurationWizard.exe`
  - `EIDConfigurationWizardElevated.exe`
- **Description:** Main configuration wizard for EID Authentication
- **Design suggestion:** Shield/gear combination, blue color scheme
- **Current status:** Not implemented (using default Windows icon)

### 2. EIDLogManager Icon
- **Filename:** `app_log_manager.ico`
- **Used by:** `EIDLogManager.exe`
- **Description:** Log viewer and management tool
- **Design suggestion:** Document/list icon with magnifying glass
- **Current status:** Has `EIDLogManager.ico` and `small.ico` (needs review)

### 3. EIDMigrate (CLI) Icon
- **Filename:** `app_migrate_cli.ico`
- **Used by:** `EIDMigrate.exe`
- **Description:** Command-line credential migration tool
- **Design suggestion:** Terminal/console icon with arrow
- **Current status:** Not implemented

### 4. EIDMigrateUI Icon
- **Filename:** `app_migrate_gui.ico`
- **Used by:** `EIDMigrateUI.exe`
- **Description:** GUI wizard for credential migration
- **Design suggestion:** Two computers with arrow between them, or document with export/import arrows
- **Current status:** Not implemented

### 5. EIDTraceConsumer Icon
- **Filename:** `app_trace_consumer.ico`
- **Used by:** `EIDTraceConsumer.exe`
- **Description:** ETW trace consumer utility
- **Design suggestion:** Pulse line or waveform icon
- **Current status:** Not implemented

### 6. Installer Icon
- **Filename:** `app_installer.ico`
- **Used by:** `EIDInstallx64.exe`
- **Description:** Main installer executable
- **Design suggestion:** Package/box icon, green color scheme to indicate installation
- **Current status:** Not implemented (using default NSIS icon)

---

## Credential Provider Icons

These icons appear on the Windows logon/lock screen.

### 7. Credential Provider Tile Image
- **Filename:** `cred_tile_image.bmp` (or `.png` with conversion)
- **Used by:** `EIDCredentialProvider.dll`
- **Resource ID:** `IDB_TILE_IMAGE` (101)
- **Current file:** `EIDCredentialProvider\SmartcardCredentialProvider.bmp`
- **Size:** 48x48 pixels (standard tile image size)
- **Description:** The icon displayed next to the EID Authentication credential tile on the Windows logon screen, lock screen, and UAC prompts
- **Design suggestion:** Smart card icon with user silhouette or badge - should be immediately recognizable as a smart card login method
- **Current status:** Uses existing `SmartcardCredentialProvider.bmp` (from Windows sample)
- **Important:** This is the PRIMARY user-facing icon - users see this every time they log in!
- **Format note:** Must be a 32-bit BMP for Windows credential provider compatibility. PNG must be converted.

---

## UI Element Icons

These icons are displayed within the application UI dialogs.

### 8. Shield Icon (UAC/Security)
- **Filename:** `ui_shield.ico`
- **Used in:**
  - EIDConfigurationWizard pages (IDC_06SHIELD, IDC_07SHIELD)
  - EIDMigrateUI pages (IDC_03_SHIELD, IDC_05_SHIELD, IDC_08_SHIELD, IDC_10_SHIELD)
- **Size:** 32x32 (displayed at 20x20 in dialogs)
- **Description:** Security indicator for administrative/sensitive operations
- **Design suggestion:** Standard Windows UAC shield icon (can reference system icon or include custom)
- **Current status:** Loading from system resources (empty path in RC files)

---

## Branding Icons

### 9. EID Authentication Brand Icon
- **Filename:** `brand_main.ico`
- **Used by:**
  - Start Menu folder icon
  - Uninstaller
  - About dialogs (future)
- **Description:** Main brand icon for EID Authentication suite
- **Design suggestion:** Smart card with lock, professional blue/gold color scheme
- **Current status:** Not implemented

---

## File Location

All icons should be placed in:
```
C:\Users\user\Documents\EIDAuthentication\icons\
```

---

## Automatic Icon Integration (build.ps1)

The `build.ps1` script **automatically copies icons** from the `icons/` folder to the appropriate project directories during the build process.

### How It Works

1. Place your icon files in the `icons/` directory with the correct filenames
2. Run `build.ps1`
3. Icons are automatically copied to their target locations
4. Projects are compiled with the new icons embedded

### Icon Mappings

| Source File (in icons/) | Destination | Project |
|------------------------|-------------|---------|
| `cred_tile_image.bmp` | `EIDCredentialProvider\SmartcardCredentialProvider.bmp` | Credential Provider |
| `app_configuration_wizard.ico` | `EIDConfigurationWizard\app.ico` | Config Wizard |
| `app_log_manager.ico` | `EIDLogManager\EIDLogManager.ico` | Log Manager |
| `app_migrate_cli.ico` | `EIDMigrate\app.ico` | Migrate CLI |
| `app_migrate_gui.ico` | `EIDMigrateUI\app.ico` | Migrate GUI |
| `app_installer.ico` | `Installer\installer.ico` | Installer |

### Graceful Degradation

If an icon file is **not found** in the `icons/` directory:
- The build continues without errors
- The application uses default Windows icons (via `SetIcon()` fallback in EIDCardLibrary)
- A warning is displayed showing which icons were skipped

### Building

```powershell
# Standard build - copies any available icons
.\build.ps1

# Icons are copied before compilation
# See the "Processing Icons" section in build output
```

---

## Implementation Notes

### Icon Resource Implementation

All projects use a standardized icon resource ID:

```cpp
#define IDI_APP_ICON 101  // Consistent across all projects
```

The `SetIcon()` function in `EIDCardLibrary\Package.cpp` follows this priority:
1. Try loading `IDI_APP_ICON` (101) from the application's resources
2. Fall back to Windows system icon (imageres.dll, resource 58)

### Adding Icons to Projects (Manual Method)

To add an icon to a Visual C++ project:

1. **Add icon resource to .rc file:**
   ```rc
   // Replace existing or add new icon resource
   IDI_MAIN_ICON ICON "icons/app_configuration_wizard.ico"
   ```

2. **Add to resource.h:**
   ```cpp
   #define IDI_MAIN_ICON  101
   ```

3. **In WinMain or window creation:**
   ```cpp
   // Set window class icon
   wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
   wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
   ```

### Project Files Requiring Updates

| Project | Resource File | Icon Resource ID | Status |
|---------|--------------|------------------|--------|
| EIDCredentialProvider | EIDCredentialProvider.rc | IDB_TILE_IMAGE (101) | Implemented |
| EIDConfigurationWizard | EIDConfigurationWizard.rc | IDI_APP_ICON (101) | Implemented |
| EIDLogManager | EIDLogManager.rc | IDI_APP_ICON (101) | Implemented |
| EIDMigrate | EIDMigrate.rc | IDI_APP_ICON (101) | Implemented |
| EIDMigrateUI | EIDMigrateUI.rc | IDI_APP_ICON (101) | Implemented |
| Installer | Installerx64.nsi | `Icon "installer.ico"` | Implemented |
| EIDTraceConsumer | (needs .rc file) | (needs adding) | Not implemented |

---

## Priority Order

1. **CRITICAL Priority:**
   - cred_tile_image.bmp (Windows logon screen - EVERY user sees this daily)

2. **High Priority:**
   - app_configuration_wizard.ico (main user-facing app)
   - app_migrate_gui.ico (migration wizard)
   - brand_main.ico (overall branding)

3. **Medium Priority:**
   - app_log_manager.ico (has existing, needs update)
   - app_migrate_cli.ico (CLI tool)
   - ui_shield.ico (security indicator)

4. **Low Priority:**
   - app_installer.ico (installer, seen briefly)
   - app_trace_consumer.ico (utility tool)

---

## Design Guidelines

- **Color Scheme:** Professional blue (#0078D4 Windows blue) as primary
- **Style:** Flat, modern Windows 11 style
- **Background:** Transparent for all sizes except 256x256 (can use PNG transparency)
- **Contrast:** Ensure visibility at 16x16 scale
- **Consistency:** All app icons should share visual elements (color, shape language)

---

## References

For design inspiration, refer to:
- Windows 11 Fluent Design System icons
- Existing Windows Security icons
- Smart card/eID industry standards
