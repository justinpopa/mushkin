# Global Preferences Audit

Discovered 2026-03-03. Global preferences were broken due to a "Split Truth" architecture — the Global Preferences dialog wrote to `GlobalOptions` (QSettings), but application logic read from `Database` (SQLite). User changes to global prefs were silently ignored for many settings.

## Architecture Problem — RESOLVED

~~The dialog updated QSettings, but the app read from SQLite. No sync layer existed.~~

**Fixed 2026-03-03:** `GlobalOptions` now reads/writes `mushclient_prefs.sqlite` via `Database`. QSettings removed from `global_options.cpp`. All P0 split-truth bugs resolved. `ReconnectOnLinkFailure` key-name bug fixed in `toggleReconnectOnDisconnect()`. All callsites in `main_window.cpp`, `activity_window.cpp`, `xml_serialization.cpp`, and `world_settings.cpp` migrated from direct DB reads to `GlobalOptions` getters.

## Settings Audit

### ~~P0 — Split Truth (dialog changes silently ignored)~~ RESOLVED

All P0 items fixed — `GlobalOptions` now backed by SQLite. Key-name bug (`ReconnectOnDisconnect` → `ReconnectOnLinkFailure`) fixed.

### P1 — Dead Code (stored/loaded but never applied to world documents)

| Setting | Default | Issue | Original MUSHclient Reference |
|:---|:---|:---|:---|
| ~~DefaultOutputFont~~ | ~~"Courier New"~~ | ~~Never applied to WorldDocument~~ | ~~`doc_construct.cpp:696-703`~~ |
| ~~DefaultOutputFontHeight~~ | ~~9~~ | ~~WorldDocument hardcodes 12~~ | ~~`doc_construct.cpp:696-703`~~ |
| ~~DefaultInputFont~~ | ~~"Courier New"~~ | ~~Never applied~~ | ~~`doc_construct.cpp:686-695`~~ |
| ~~DefaultInputFontHeight~~ | ~~9~~ | ~~WorldDocument hardcodes 12~~ | ~~`doc_construct.cpp:686-695`~~ |
| AllTypingToCommandWindow | false | No input routing logic | `App.m_bAllTypingToCommandWindow` |
| ~~AppendToLogFiles~~ | ~~true~~ | ~~`main_window.cpp` hardcodes `true` in `OpenLog()` call~~ | ~~`App.m_bAppendToLogFiles`~~ |
| ~~AutoLogWorld~~ | ~~false~~ | ~~Not implemented~~ | ~~`App.m_bAutoLogWorld`~~ |

~~Strikethrough~~ = fixed in `feat/ci-artifacts-and-font-research` branch.

### P2 — Unimplemented Behaviors

| Setting | Issue | Original MUSHclient Reference |
|:---|:---|:---|
| ~~AlwaysOnTop~~ | ~~Not applied at startup~~ | ~~`App.m_bAlwaysOnTop`~~ |
| ~~OpenWorldsMaximized~~ | ~~Child windows use default geometry~~ | ~~`App.m_bOpenWorldsMaximised`~~ |
| ~~OpenActivityWindow~~ | ~~Never checked on startup~~ | ~~`App.m_bOpenActivityWindow`~~ |
| ~~ConfirmBeforeClosingMushclient~~ | ~~closeEvent ignores it~~ | ~~`App.m_bConfirmBeforeClosingMushclient`~~ |
| ~~ConfirmBeforeClosingWorld~~ | ~~Not implemented~~ | ~~`App.m_bConfirmBeforeClosingWorld`~~ |
| ~~ConfirmLogFileClose~~ | ~~Not implemented~~ | ~~`App.m_bConfirmLogFileClose`~~ |
| ~~DefaultTriggersFile~~ | ~~Used by "Load Default Files" action, but not auto-applied on world creation~~ | ~~`doc_construct.cpp:664`~~ |
| ~~DefaultColoursFile~~ | ~~Used by "Load Default Files" action, but not auto-applied on world creation~~ | ~~`doc_construct.cpp:670`~~ |

### Applied (working correctly)

| Setting | Used Where |
|:---|:---|
| AlwaysOnTop | Applied at startup in MainWindow constructor |
| AppendToLogFiles | `toggleLogSession()` reads from GlobalOptions |
| ConfirmBeforeClosingMushclient | `closeEvent()` shows confirmation dialog |
| DefaultLogFileDirectory | Save/Open dialogs |
| DefaultWorldFileDirectory | Save/Open dialogs |
| DefaultInputFont/Height | `WorldDocument::applyGlobalFontDefaults()` (called from constructor) |
| DefaultOutputFont/Height | `WorldDocument::applyGlobalFontDefaults()` (called from constructor) |
| OpenActivityWindow | Shown at startup if preference set |
| ConfirmBeforeClosingWorld | `closeWorld()` confirms when connected |
| ConfirmLogFileClose | `toggleLogSession()` confirms before closing log |
| DefaultTriggersFile/AliasesFile/TimersFile/MacrosFile/ColoursFile | Auto-applied in `newWorld()` |
| OpenWorldsMaximized | `newWorld()` and `openWorld()` call `showMaximized()` |

## How the Original MUSHclient Works

The original uses a single-truth architecture with `mushclient_prefs.sqlite` as the sole persistent store:

1. **One-time migration:** `PopulateDatabase()` copies Windows Registry values into the SQLite `prefs` table on first launch. The Registry is never read again after this.
2. **Startup:** `LoadGlobalsFromDatabase()` reads all `prefs` rows into in-memory `CApp` members (e.g., `m_bAutoConnectWorlds`).
3. **Runtime reads:** `GetGlobalOption()` reads CApp members — never hits SQLite directly.
4. **Writes:** The dialog updates CApp members then calls `SaveGlobalsToDatabase()` to flush to SQLite. Menu toggles (`AutoConnectWorlds`, `ReconnectOnLinkFailure`, `AlwaysOnTop`) write individually via `db_write_int()`.

The database filename is `mushclient_prefs.sqlite` (`stdafx.h:125`), which is the same file Mushkin's `Database` class already reads.

## Recommendation

**Option 1: Make GlobalOptions read/write Database, then remove QSettings.**

GlobalOptions becomes a typed C++ API over the existing SQLite `prefs` table. QSettings is removed entirely — it replicates a Registry migration path the original already completed. Any active MUSHclient user's prefs are already in `mushclient_prefs.sqlite`.

This gives us:
- **Drop-in compatibility:** Mushkin reads the user's existing `mushclient_prefs.sqlite` on day one — no configuration needed.
- **Single truth:** Dialog, app logic, and Lua `GetGlobalOption` all read/write the same SQLite store.
- **No QSettings dependency:** Platform-native config files (.plist/registry/.conf) are no longer created or consulted.

### Implementation plan — DONE

1. ~~Rewrite `GlobalOptions::load()` to read from `Database::getPreference()`/`getPreferenceInt()` instead of `QSettings`.~~
2. ~~Rewrite `GlobalOptions::save()` to write via `Database::setPreference()`/`setPreferenceInt()` instead of `QSettings`.~~
3. ~~Remove all `#include <QSettings>` and QSettings usage from `global_options.cpp`.~~
4. ~~Fix the `ReconnectOnLinkFailure` key-name bug — `main_window.cpp` reads `"ReconnectOnDisconnect"` but the original and GlobalOptions both use `"ReconnectOnLinkFailure"`.~~
5. ~~Verify all P0 settings resolve (dialog writes and app reads now hit the same store).~~
6. Optional: one-time QSettings→SQLite migration for users who ran early Mushkin builds that wrote prefs to QSettings only.

### Remaining QSettings usage (not global prefs — separate concern)

| File | Usage | Notes |
|:---|:---|:---|
| `main_window.cpp` `readSettings()`/`writeSettings()` | Window geometry (position, size, state) | Could move to Database `control` table |
| `plugin_dialog.cpp` | Dialog geometry state | Could move to Database `control` table |

Removing these would allow dropping the `QSettings` include and Qt Core5Compat dependency entirely.

### Separate bugs (not fixed by backend unification)

- **`SetGlobalOption` missing from Lua API** — only `GetGlobalOption` is registered. The original MUSHclient also does not expose `SetGlobalOption` to Lua (confirmed: no such function exists in the original). Not a gap.
- **P1 dead code** — settings stored but never applied (AllTypingToCommandWindow, AppendToLogFiles, AutoLogWorld) require separate implementation work.
- **P2 unimplemented behaviors** — settings that need runtime logic added (AlwaysOnTop at startup, confirmation dialogs, etc.).

## Key Files

- `src/storage/global_options.h/cpp` — GlobalOptions (SQLite backend via Database)
- `src/storage/database.h/cpp` — Database with getPreferenceInt/setPreferenceInt
- `src/ui/dialogs/global_preferences_dialog.cpp` — Dialog that writes GlobalOptions
- `src/ui/main_window.cpp` — App logic that reads GlobalOptions
- `src/world/world_document.cpp` — World construction (font defaults fixed)
- `reference/mushclient-original/doc_construct.cpp` — How original applies globals
- `reference/mushclient-original/scripting/methods/methods_defaults.cpp` — ReloadDefaults
