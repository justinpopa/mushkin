# Settings Migration TODO

## Goal
Use Mushkin's own settings location on all platforms, with one-time migration from MUSHclient on Windows.

## Current State
- All platforms use `Gammon/MUSHclient` for QSettings (for compatibility)
- This means settings are stored at:
  - macOS: `~/Library/Preferences/com.Gammon.MUSHclient.plist`
  - Linux: `~/.config/Gammon/MUSHclient.conf`
  - Windows: Registry `HKEY_CURRENT_USER\Software\Gammon\MUSHclient`

## Desired State
- Use `Mushkin/Mushkin` for QSettings on all platforms
- Settings stored at:
  - macOS: `~/Library/Preferences/com.Mushkin.Mushkin.plist`
  - Linux: `~/.config/Mushkin/Mushkin.conf`
  - Windows: Registry `HKEY_CURRENT_USER\Software\Mushkin\Mushkin`

## Windows Migration
On first run on Windows:
1. Check if `HKEY_CURRENT_USER\Software\Gammon\MUSHclient` exists
2. If yes, copy relevant settings to `HKEY_CURRENT_USER\Software\Mushkin\Mushkin`
3. Mark migration as complete (store a flag so we don't repeat)
4. Use Mushkin's registry location going forward

## Files to Modify
- `src/main.cpp` - Change `setApplicationName`/`setOrganizationName`
- `src/storage/global_options.cpp` - Update SETTINGS_ORG/SETTINGS_APP constants
- `src/ui/main_window.cpp` - Update hardcoded QSettings calls
- Add migration logic (probably in main.cpp or a new migration.cpp)

## Notes
- Linux/macOS never had original MUSHclient, so no migration needed there
- Consider: should we delete the old Gammon registry key after migration, or leave it?
