# Mushkin UI Dialog Review

**Date:** 2026-02-27
**Method:** Desktop automation (screen capture + interaction) at 1280x720 resolution
**Reviewer:** Claude Opus 4.6

## Bugs

---

### B1 — World Properties: Enter key closes dialog instead of navigating tree

**Severity:** Bug
**Location:** World Properties dialog (tree view + stacked widget)
**Description:** Pressing Enter while a tree item is focused activates the OK button (closing the dialog) instead of navigating to the selected page. The tree should consume the Enter key event or the OK button should not be the default button while the tree has focus.

**Targets:**
- [x] `src/ui/preferences/unified_preferences_dialog.cpp` — Event filter on tree widget consumes Enter/Return and navigates to selected page

**Acceptance:** Enter key in tree view switches to the selected page, does not close the dialog.

---

### B2 — Output page: ANSI color labels truncated

**Severity:** Medium
**Location:** World Properties > Appearance > Output — ANSI Color Palette section
**Description:** The left-side labels for ANSI colors (e.g., "Black", "Dark Blue", "Bright Green") are truncated because the label column is too narrow. Worse at low resolution but visible at all sizes.

**Targets:**
- [x] `src/ui/preferences/pages/output_page.cpp` — Replaced fixed `setMinimumWidth(100)` with `QSizePolicy::Minimum` so labels auto-size to fit

**Acceptance:** All 16 ANSI color names fully visible at 1280x720.

---

### B3 — Plugin Management: last column header truncated

**Severity:** Medium
**Location:** File > Plugins... dialog
**Description:** The rightmost column header is truncated to "F..." (likely "File" or "Filename"). Dialog needs a wider minimum width or better column sizing.

**Targets:**
- [x] `src/ui/dialogs/plugin_dialog.cpp` — Columns use `ResizeToContents` with Purpose column stretching to fill

**Acceptance:** All column headers fully visible without horizontal scrolling at 1280x720.

---

### B4 — Trigger/Alias/Timer button text truncated

**Severity:** Medium
**Location:** World Properties > Triggers/Aliases/Timers pages
**Description:** The "Disable" button text is truncated to "Disabl..." at 1280x720 resolution. Buttons need enough room for their labels.

**Targets:**
- [x] `src/ui/preferences/item_list_page_base.cpp` — Button groups stack vertically; all buttons use `QSizePolicy::Minimum` to prevent shrinking below text width

**Acceptance:** All button labels fully visible at 1280x720.

---

## Stubs / Unimplemented

---

### S1 — Global Preferences dialog not implemented

**Severity:** Medium
**Location:** File > Global Preferences (Opt+Cmd+G)
**Description:** Shows a message box: "Global preferences dialog is not yet implemented."

**Targets:**
- [x] `src/ui/main_window.cpp` — Replaced QMessageBox stub with `GlobalPreferencesDialog`
- [x] `src/storage/global_options.h/.cpp` — Added 12 missing fields (worldList, pluginList, parenMatchFlags, etc.)
- [x] `src/ui/dialogs/global_preferences_dialog.cpp` — Rewrote load/save to use `GlobalOptions` (QSettings) instead of `Database` (SQLite)

**Acceptance:** Menu item either opens a functional dialog or is hidden/grayed.

---

### S2 — Reload Defaults not implemented

**Severity:** Low
**Location:** File > Reload Defaults (Opt+Cmd+R)
**Description:** Shows a message box: "Reload defaults is not yet implemented."

**Targets:**
- [ ] Implement reload-defaults functionality or disable/hide the menu item

**Acceptance:** Menu item either works or is hidden/grayed.

---

### S3 — MXP / Pueblo page is empty stub

**Severity:** Low
**Location:** World Properties > Appearance > MXP / Pueblo
**Description:** Shows "This page is under construction." with no controls. Also has a redundant duplicate header ("MXP / Pueblo" appears as both page title and section header).

**Targets:**
- [ ] Implement MXP/Pueblo settings or show a more informative placeholder
- [ ] Remove the redundant inner "MXP / Pueblo" heading

**Acceptance:** Page either has controls or a clean placeholder without duplicate headers.

---

## Design Issues

---

### D1 — "Colour Picker" uses British spelling

**Severity:** Low
**Location:** Edit menu > "Colour Picker"
**Description:** British spelling "Colour" is inconsistent with American "Color" used elsewhere in the app (e.g., "Echo color" in Commands page, "ANSI Color Palette" in Output page, "Colors" tree item).

**Targets:**
- [x] `src/ui/main_window.cpp` — Renamed "Colour Picker" → "Color Picker", "ANSI Colours" → "ANSI Colors", "Custom Colours" → "Custom Colors", "Choose Colour" → "Choose Color"

**Acceptance:** Consistent American English spelling throughout.

---

### D2 — View menu: redundant full screen entries

**Severity:** Low (Won't Fix)
**Location:** View menu
**Description:** "Full Screen Mode" (Opt+Cmd+F) and "Enter Full Screen" (Globe+F) are two separate menu items that appear to do the same thing. However, "Enter Full Screen" is automatically added by macOS, not by Mushkin. Only one custom entry exists in the code.

**Targets:**
- [x] Investigated — "Enter Full Screen" is a macOS system-provided menu item (not our code). No action needed.

---

### D3 — No "About Mushkin" dialog

**Severity:** Low (Already Fixed)
**Location:** App menu (mushkin menu, left of File)
**Description:** macOS apps conventionally have an "About" dialog in the application menu showing version, credits, license. Mushkin is missing this.

**Targets:**
- [x] `src/ui/main_window.cpp:4037-4051` — About dialog already exists with version, description, and MUSHclient credits

**Acceptance:** mushkin > About Mushkin opens a dialog with version info.

---

### D4 — Colors page vs Output page ANSI palette confusion

**Severity:** Medium
**Location:** World Properties > Appearance > Colors vs Output
**Description:** The "Colors" page only shows 16 custom color slots. The actual ANSI color palette (the 16 standard terminal colors) lives on the "Output" page. Users looking for ANSI colors will naturally go to "Colors" first and be confused.

**Targets:**
- [x] Renamed "Colors" to "Custom Colors" in tree and page header to clarify scope
- [ ] Consider moving ANSI palette to the Custom Colors page, or adding a note directing users to Output for ANSI colors

**Acceptance:** User can find ANSI color settings without confusion.

---

### D5 — Trigger "Lines to match" enabled when not multi-line

**Severity:** Low (Already Fixed)
**Location:** Add/Edit Trigger dialog > General tab
**Description:** The "Lines to match" spinner is enabled even when "Multi-line trigger" is unchecked. It should be disabled/grayed when not applicable to avoid confusion.

**Targets:**
- [x] `src/ui/dialogs/trigger_edit_dialog.cpp:80,85` — Spinner starts disabled; `toggled` signal connection enables/disables it based on multi-line checkbox state

**Acceptance:** "Lines to match" spinner is grayed out when "Multi-line trigger" is unchecked.

---

### D6 — Trigger Appearance tab: no color preview on buttons

**Severity:** Low (Already Fixed)
**Location:** Add/Edit Trigger dialog > Appearance tab
**Description:** The Foreground/Background buttons show only text labels with no color swatch preview. Users can't see the currently selected color at a glance.

**Targets:**
- [x] `src/ui/dialogs/trigger_edit_dialog.cpp:530-537` — `updateColorButton()` sets background-color stylesheet on buttons; called during setup and after color selection

**Acceptance:** Buttons show the currently selected color visually.

---

### D7 — Logging enabled by default

**Severity:** Low
**Location:** World Properties > General > Logging
**Description:** "Enable automatic logging" is checked by default on new worlds. Most users probably don't want logging on immediately — it should default to off.

**Targets:**
- [x] `src/world/world_document.cpp` + `config_options.cpp` — Changed `m_bLogOutput` default from `true` to `false`

**Acceptance:** New worlds have logging disabled by default.

---

### D8 — "Show bold text" unchecked by default

**Severity:** Low
**Location:** World Properties > Appearance > Output
**Description:** "Show bold text" is unchecked by default. Bold text is a fundamental MUD display feature and should default to on.

**Targets:**
- [x] `src/world/world_document.cpp` + `config_options.cpp` — Changed `m_bShowBold` default from `false` to `true`

**Acceptance:** New worlds show bold text by default.

---

## Responsiveness / Low-Resolution

---

### R1 — World Properties fills entire screen at 1280x720

**Severity:** Low
**Location:** World Properties dialog
**Description:** The dialog takes the full screen at 1280x720, leaving no visible parent window. Should have a reasonable maximum size or be scrollable.

**Targets:**
- [x] `src/ui/preferences/unified_preferences_dialog.cpp` — Reduced minimum size to 800x500; `resize()` now adapts to screen size with 80px margin

**Acceptance:** Dialog fits within 1280x720 with some margin, or content is scrollable.

---

### R2 — Keypad page tight at low resolution

**Severity:** Low (Already Fixed)
**Location:** World Properties > Input > Keypad
**Description:** The Ctrl+Keypad section gets tight at low resolution. Content may need scrolling on smaller screens.

**Targets:**
- [x] `src/ui/preferences/pages/keypad_page.cpp:37-39,162-163` — Already wrapped in `QScrollArea` with `setWidgetResizable(true)`

**Acceptance:** All keypad sections accessible at 1280x720.

---

### R3 — Plugin dialog columns need better auto-sizing

**Severity:** Low
**Location:** File > Plugins dialog
**Description:** Column widths don't adapt to content. Horizontal scrollbar appears unnecessarily while there's unused space.

**Targets:**
- [x] `src/ui/dialogs/plugin_dialog.cpp` — Same fix as B3: `ResizeToContents` + `Stretch` on Purpose column

**Acceptance:** All columns visible without scrolling when dialog has sufficient width.

---

## Positive Notes

- World Properties tree navigation well-organized (General/Appearance/Automation/Input/Scripting)
- Consistent layout across Triggers/Aliases/Timers (shared table + operations pattern)
- Good placeholder text throughout (Server, Character name, Pattern fields)
- Commands page is dense but well-grouped with clear sections
- Paste / Send has a tabbed interface — nice touch
- Macros page has a search/filter field — helpful for power users
- Trigger dialog tabs (General/Response/Options/Appearance) cover all MUSHclient fields
- Plugin Management dialog has all essential operations (Add/Remove/Enable/Disable/Edit/Reinstall/Show Info)
