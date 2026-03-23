# Behavioral Worklist

Machine-readable worklist for automated fix loop. Items from `behavioral_audit_2026-03-20.md`.

## Format
- `[ ]` = unresolved, `[x]` = fixed, `[~]` = wontfix/deferred
- Each item has: ID, severity, subsystem, description, original ref, mushkin ref

## HIGH

- [~] H1: trigger/alias — Script callbacks fire inline, not deferred after all matching + omit processing. Original: `ProcessPreviousLine.cpp` defers `strProcedure` to `triggerList`. Mushkin: `automation_registry.cpp`, `world_trigger_execution.cpp` (DEFERRED: architectural change spanning 4+ files, needs dedicated plan)
- [x] H2: trigger/alias — `eSendToCommandQueue` (send_to=8) is no-op. Original: queues for delayed send. Mushkin: `world_sendto.cpp:85-87`
- [x] H3: trigger/alias — `StopEvaluatingTriggers()` flag set but never read. Original: checked in eval loop. Mushkin: `world_triggers.cpp:813`, `automation_registry.cpp`
- [x] H4: connection — Auto-login missing (connect_now MUSH/Diku + connect_text). Original: `doc.cpp:6753-6795`. Mushkin: `connection_manager.cpp:onConnect()`
- [x] H5: connection — MXP_On() not called for always-on mode at connect. Original: `doc.cpp:6625`. Mushkin: `connection_manager.cpp:onConnect()`
- [x] H6: connection — `m_nBytesOut` never updated in SendPacket. Original: `doc.cpp:5600`. Mushkin: `world_protocol.cpp:SendPacket()`
- [x] H7: plugins — `OnPluginPacketReceived` is read-only (original allows plugin modification chain). Original: `doc.cpp:1768-1775`. Mushkin: `world_document.cpp:779`
- [x] H8: plugins — `OnPluginPlaySound` never fired. Original: `doc.cpp` sound path. Mushkin: `sound_manager.cpp`
- [~] H9: plugins — All 7 MXP callbacks missing (known gap, deferred with MXP). Mushkin: `mxp_engine.cpp`
- [x] H10: plugins — `SendToFirstPluginCallbacks` stops on return-true instead of first-callback-defined. Original: `plugins.cpp:1380-1385`. Mushkin: `world_document_plugins.cpp:833`
- [x] H11: output — Deferred note queue missing (notes during `m_bNotesNotWantedNow` silently dropped). Original: `methods_noting.cpp:49-71`. Mushkin: `output_formatter.cpp:47-50`
- [x] H12: global-prefs — AutoLogWorld append mode wrong (uses setting instead of hardcoded true). Original: `doc.cpp:6668`. Mushkin: `connection_manager.cpp:151`
- [x] H13: global-prefs — Directory resolution uses `AppPaths::getAppDirectory()` instead of CWD at startup. Original: `Utilities.cpp:2483`. Mushkin: `global_options.cpp`

## MEDIUM

- [x] M1: trigger/alias — `keep_evaluating=false` stops ALL phases, not just current plugin. Mushkin: `automation_registry.cpp`
- [~] M2: trigger/alias — One-shot trigger deletion premature + only one per line. Mushkin: `automation_registry.cpp` (DEFERRED: tied to H1 script deferral architecture)
- [x] M3: trigger/alias — `%C` (clipboard), `%N` (name), `%<namedCapture>` substitutions missing. Mushkin: `world_trigger_execution.cpp:44-58`
- [x] M4: trigger/alias — Color matching checks all style runs, not matched text's first char. Mushkin: `automation_registry.cpp:236-282`
- [x] M5: trigger/alias — `eSendToWorld` ignores echo/log flags. Mushkin: `world_sendto.cpp`
- [x] M6: trigger/alias — `@@` → `@` escape and `@!var` syntax missing. Mushkin: `world_trigger_execution.cpp` (FALSE POSITIVE: both already implemented in world_variables.cpp:164-177)
- [x] M7: connection — NAWS sends column width for BOTH width and height. Mushkin: `telnet_parser.cpp:sendWindowSizes()`
- [x] M8: connection — `ResetAllTimers()` not called on connect. Mushkin: `connection_manager.cpp:onConnect()`
- [x] M9: connection — `MXP_Off()` not called on disconnect. Mushkin: `connection_manager.cpp:onConnectionDisconnect()`
- [x] M10: connection — Auto-log not closed on disconnect. Mushkin: `connection_manager.cpp:onConnectionDisconnect()`
- [x] M11: connection — `TELOPT_MUD_SPECIFIC` missing in Phase_DO (Aardwolf). Mushkin: `telnet_parser.cpp`
- [x] M12: connection — Session counters not reset on connect. Mushkin: `connection_manager.cpp:onConnect()`
- [x] M13: connection — Socket read error doesn't trigger disconnect. Mushkin: `world_document.cpp:ReceiveMsg`
- [x] M14: plugins — Script error on plugin load is non-fatal (original fails load). Mushkin: `world_document_plugins.cpp:515-519`
- [x] M15: plugins — `OnPluginWorldSave` can double-fire. Mushkin: `xml_serialization.cpp:228`, `world_misc.cpp:2473`
- [x] M16: plugins — UnloadPlugin/ReloadPlugin no name fallback (GUID only). Mushkin: `world_plugins.cpp:654-741`
- [x] M17: plugins — `OnPluginPacketDebug` not fired. Mushkin: constant defined, no fire site
- [x] M18: output — `echo_colour` and `note_text_colour` TODO stubs. Mushkin: `world_document.cpp:2075-2079`
- [~] M19: output — No force-new-line before note on non-COMMENT line. Mushkin: `output_formatter.cpp:66-71` (DEFERRED: needs Tell/colourTell to set COMMENT flag on current line to avoid breaking Tell+Note sequences)
- [x] M20: output — Wrap column compares bytes not Unicode chars (CJK). Mushkin: `world_document.cpp:1779-1780`
- [x] M21: output — No Big5/GB2312 split detection in word-wrap. Mushkin: `world_document.cpp:1823-1830`
- [x] M22: output — Style not migrated during word-wrap. Mushkin: `world_document.cpp:1854-1869`
- [x] M23: output — StartNewLine doesn't check if triggers consumed line. Mushkin: `world_document.cpp:2018-2032`
- [x] M24: global-prefs — Dialog default buttons are Cancel; original defaults to OK. Mushkin: `main_window.cpp`
- [x] M25: global-prefs — No transaction wrapping in GlobalOptions::save(). Mushkin: `global_options.cpp`
- [x] M26: global-prefs — `m_bUseDefaultTriggers/Colours/etc.` flags not set. Mushkin: `main_window.cpp:2049`
- [x] M27: global-prefs — AlwaysOnTop menu checkmark not synced on startup. Mushkin: `main_window.cpp:976`
- [x] M28: global-prefs — Default files load order (colours should be first). Mushkin: `main_window.cpp:2049`
- [x] M29: global-prefs — `applyGlobalFontDefaults()` missing `font_italic`. Mushkin: `world_document.cpp`
- [x] M30: global-prefs — Per-world close confirmation may not fire during app exit. Mushkin: `main_window.cpp:closeEvent`

## HIGH (XML serialization audit — 2026-03-21)

- [x] H14: xml — Bare files without `<muclient>` root rejected (original treats root as container). Mushkin: `xml_serialization.cpp:481-492`
- [x] H15: xml — Password `_base64` sentinel not written/checked (cross-load garbles passwords). Mushkin: `xml_serialization.cpp:323-329,580-586`
- [x] H16: xml — `bIncluded` flag not set/checked for triggers/aliases (included items re-saved inline). Mushkin: `world_serialization.cpp:83-193`
- [x] H17: xml — `<colours>` section (ANSI normal/bold, custom palette) has no load/save implementation. Custom color schemes lost on save/reload.
- [~] H18: xml — Non-plugin `<include>` files silently dropped on load. Original loads them at phase 1. Mushkin: `xml_serialization.cpp:622-643` (DEFERRED: requires include list tracking, phase-1 loading, include ref re-save — 5+ files)

## MEDIUM (XML serialization audit — 2026-03-21)

- [x] M31: xml — `$PLUGINDIR` placeholder not supported in include path resolution. Mushkin: `xml_serialization.cpp:69-72`
- [x] M32: xml — Duplicate trigger detection (structural equality) missing — repeated imports create duplicates. Original: `xml_load_world.cpp:1373-1389`
- [x] M33: xml — RGB color value `0` (black) not applied on load — kept as constructor default. Mushkin: `xml_serialization.cpp:558-563`
- [x] M34: xml — Zero-interval timer not rejected on load — runaway timer risk. Original: `xml_load_world.cpp:1717-1718`

## LOW (XML serialization audit — 2026-03-21)

- [x] L25: xml — `temporary` attribute not loaded for triggers/aliases from XML. Mushkin: `world_serialization.cpp`
- [x] L26: xml — Variable `trim` attribute not supported. Mushkin: `world_serialization.cpp:803-841`
- [x] L27: xml — Variable name lowercased on load (original preserves case). Mushkin: `world_serialization.cpp:815`

## HIGH (Logging audit — 2026-03-21)

- [x] H19: logging — `OpenLog()` writes file preamble even from Lua API (original: preamble only from interactive/auto-connect). Mushkin: `world_logging.cpp:216-229`
- [x] H20: logging — `OpenLog()` always triggers retrospective log dump (original: interactive dialog only). Mushkin: `world_logging.cpp:234-237`

## MEDIUM (Logging audit — 2026-03-21)

- [x] M35: logging — Auto-log world name header (`write_world_name`) never written — field unused. Original: `doc.cpp:6691-6726`. Mushkin: `world_logging.cpp`
- [x] M36: logging — `LogCommand` missing `\r\n` strip and echo color `<font>` wrap in HTML mode. Original: `doc.cpp:6093-6160`. Mushkin: `world_document.cpp:1173-1202`
- [x] M37: logging — `WriteToLog` write errors silently ignored, no file-close or user alert. Original: `doc.cpp:3104-3112`. Mushkin: `world_logging.cpp:306-317`

## LOW (Logging audit — 2026-03-21)

- [x] L28: logging — Periodic close/reopen every 120s in `flushLogIfNeeded` (not in original). Mushkin: `world_document.cpp:2655-2671`
- [x] L29: logging — `WriteLog` doc comment says "no automatic newlines" but implementation adds `\n` (doc bug only). Mushkin: `world_logging.cpp:79-80`

## HIGH (Command processing audit — 2026-03-21)

- [x] H21: commands — No recursion depth guard in Execute() — infinite alias loops crash instead of returning eCommandsNestedTooDeeply. Mushkin: `world_document.cpp:Execute()`
- [x] H22: commands — Auto-say inside Execute() — Lua Execute() calls incorrectly apply auto-say prefix. Original only applies auto-say in SendView UI layer. Mushkin: `world_document.cpp:1233-1250`
- [x] H23: commands — ON_PLUGIN_COMMAND not fired per stacked sub-command — plugins cannot suppress individual stacked commands. Mushkin: `world_session.cpp:40-49`

## MEDIUM (Command processing audit — 2026-03-21)

- [x] M38: commands — Empty stacked commands not sent as blank line (Qt::SkipEmptyParts drops them). Mushkin: `world_document.cpp:SendMsg()`
- [x] M39: commands — QUIT macro detection missing — m_bDisconnectOK not set, auto-reconnect not suppressed. Mushkin: `world_document.cpp:Execute()`
- [~] M40: commands — Alias echo not deduplicated across multiple matching aliases (original echoes once per line). Mushkin: `world_alias_execution.cpp` (DEFERRED: needs shared state between evaluation loop and executeAlias)
- [x] M41: commands — Lua Execute() pollutes command history (original never adds history from Lua path). Mushkin: `world_document.cpp:Execute()`

## LOW (Command processing audit — 2026-03-21)

- [x] L30: commands — No display-line boundary before alias echo (style bleed). Mushkin: `world_alias_execution.cpp`
- [x] L31: commands — Macro exclusion from auto-say not implemented (TODO stub). Mushkin: `world_widget.cpp:435-440`
- [~] L32: commands — Speedwalk error shown as note() not modal dialog. Mushkin: `world_document.cpp` (WONTFIX: modal dialogs are intentionally avoided in Mushkin — output note is better UX)
- [x] L33: commands — History added at alias-match time not post-cycle. Mushkin: `world_alias_execution.cpp`

## HIGH (Speedwalk audit — 2026-03-21)

- [x] H24: speedwalk — Command queue drain timer completely missing. m_CommandQueue is written but never consumed. Queued commands are lost. Needs QTimer mirroring CTimerWnd::OnTimer. Mushkin: `connection_manager.cpp`

## MEDIUM (Speedwalk audit — 2026-03-21)

- [x] M42: speedwalk — EvaluateSpeedwalk returns `\n`-separated output; original returns `\r\n`. Mushkin: `speedwalk_engine.cpp`

## LOW (Speedwalk audit — 2026-03-21)

- [x] L34: speedwalk — RemoveBacktracks strips trailing space (.trimmed()). Original preserves it.
- [x] L35: speedwalk — GetMappingString count cap is 99; original caps at 98. (FALSE POSITIVE: both cap at 99 — original uses iCount<=98 then increments, Mushkin uses run<99 then increments, same result)

## HIGH (Miniwindow audit — 2026-03-21)

- [x] H25: miniwindow — WindowDrawImage modes 2 and 3 are swapped (stretch vs transparent-copy). Mushkin: `miniwindow.cpp:DrawImage()`
- [x] H26: miniwindow — WindowCircleOp actions 3/4/5 remapped (RoundRect/Chord/Pie in wrong positions). Mushkin: `miniwindow.cpp:CircleOp()` (FALSE POSITIVE: code is correct, audit compared doc comments not code)
- [x] H27: miniwindow — WindowFontInfo codes 9-21 completely different numbering scheme. Mushkin: `miniwindow.cpp:FontInfo()`
- [x] H28: miniwindow — WindowFont missing Charset and PitchAndFamily parameters (CJK broken). Mushkin: `miniwindow.cpp:Font()`
- [x] H29: miniwindow — WindowFilter operations renumbered from code 3 onward — all filter calls get wrong result. Mushkin: `miniwindow.cpp:Filter()` (FALSE POSITIVE: code has correct numbering 1-27 matching original, audit compared doc comments)
- [x] H30: miniwindow — WindowBlendImage all 65 blend modes implemented, mode numbers corrected. Mushkin: `miniwindow.cpp`, `blend_modes.h`

## MEDIUM (Miniwindow audit — 2026-03-21)

- [x] M43: miniwindow — WindowCreate KEEP_HOTSPOTS flag not honored — hotspots never cleared on re-create. Mushkin: `world_miniwindow_lifecycle.cpp`
- [x] M44: miniwindow — WindowRectOp action 5 (DrawEdge) Windows-specific, Qt behavior different. Mushkin: `miniwindow.cpp`
- [x] M45: miniwindow — WindowRectOp actions 4/6-7 (Draw3dRect, flood fill) implemented. Mushkin: `miniwindow.cpp`
- [x] M46: miniwindow — WindowImageInfo codes mismapped (code 1=bmType in original, width in Mushkin). Mushkin: `miniwindow.cpp`
- [x] M47: miniwindow — WindowImageOp pen validation and coordinate handling fixed. Mushkin: `miniwindow.cpp`

## LOW (Miniwindow audit — 2026-03-21)

- [x] L36: miniwindow — WindowInfo code 16 (mouse update count) returns nil. Mushkin: `miniwindow.cpp`
- [x] L37: miniwindow — WindowInfo code 21 (date installed) and 23 (plugin ID) missing. Mushkin: `miniwindow.cpp`
- [x] L38: miniwindow — WindowGradient mode 3 (texture fill) absent. Mushkin: `miniwindow.cpp`
- [x] L39: miniwindow — WindowAddHotspot plugin enforcement missing (eHotspotPluginChanged). Mushkin: `world_miniwindow_lifecycle.cpp`
- [x] L40: miniwindow — WindowDeleteHotspot doesn't clear mouseOver/mouseDown state. Mushkin: `world_miniwindow_lifecycle.cpp`

## HIGH (Sound/MSP audit — 2026-03-21)

- [x] H31: sound — `enable_trigger_sounds` flag not checked before playing trigger sounds. Mushkin: `world_trigger_execution.cpp:203-212`

## MEDIUM (Sound/MSP audit — 2026-03-21)

- [x] M48: sound — Adjust-playing-sound mode missing (empty filename with buffer adjusts vol/pan/loop). Mushkin: `sound_manager.cpp`
- [x] M49: sound — File path resolution uses working dir, not exe dir for sounds/ subdirectory. Mushkin: `sound_manager.cpp:552-569`
- [x] M50: sound — `PlaySoundMemory` always fails (stub returns eCannotPlaySound). Mushkin: `world_sounds.cpp`
- [x] M51: sound — `CancelSound` plugin callback missing (empty string OnPluginPlaySound). Mushkin: `world_sound.cpp`

## LOW (Sound/MSP audit — 2026-03-21)

- [x] L41: sound — `Sound("")` returns eCannotPlaySound instead of eNoNameSpecified. Mushkin: `world_sounds.cpp`
- [~] L42: sound — `play_in_background` flag serialized but has no effect. Mushkin: `sound_manager.cpp` (WONTFIX: Qt Spatial Audio handles background playback via OS audio policy — DirectSound's DSBCAPS_GLOBALFOCUS has no Qt equivalent)
- [x] L43: sound — MSP `T=` (type) and `P=` (priority) parameters silently dropped. Mushkin: `telnet_parser.cpp`

## HIGH (Notepad audit — 2026-03-21)

- [x] H32: notepad — SendToNotepad does create-or-replace instead of always-create (original creates duplicates). Mushkin: `world_notepad.cpp`
- [x] H33: notepad — ReplaceNotepad returns error when notepad not found (original creates new). Mushkin: `world_notepad.cpp`
- [x] H34: notepad — GetNotepadText returns `\n` line endings (original `\r\n`) and nil when not found (original `""`). Mushkin: `world_notepads.cpp`
- [x] H35: notepad — GetNotepadWindowPosition returns comma string (original returns Lua table with named keys). Mushkin: `world_notepads.cpp`
- [x] H36: notepad — SaveNotepad replaceExisting defaults to true (original false — conservative). Mushkin: `world_notepads.cpp`

## MEDIUM (Notepad audit — 2026-03-21)

- [x] M52: notepad — Multi-arg concatenation missing (AppendToNotepad/SendToNotepad/ReplaceNotepad ignore args 3+). Mushkin: `world_notepads.cpp`
- [x] M53: notepad — GetNotepadLength counts `\n` as 1 char (original counts `\r\n` as 2). Mushkin: `world_notepads.cpp`
- [x] M54: notepad — NotepadFont charset parameter required (original optional, default 0). Mushkin: `world_notepads.cpp`
- [x] M55: notepad — NotepadReadOnly default is false when arg missing (original true). Mushkin: `world_notepads.cpp`
- [x] M56: notepad — CloseNotepad querySave prompt not implemented (TODO stub). Mushkin: `world_notepads.cpp`
- [x] M57: notepad — GetNotepadText not-found returns nil vs original empty string "". Mushkin: `world_notepads.cpp` (fixed as part of H34)

## LOW (Notepad audit — 2026-03-21)

- [x] L44: notepad — NotepadSaveMethod missing invalid-value rejection (accepts any int). Mushkin: `world_notepad.cpp`
- [~] L45: notepad — NotepadColour/NotepadFont return error codes instead of bool. Mushkin: `world_notepads.cpp` (WONTFIX: error codes are more informative than bool; consistent with Mushkin's API style)
- [~] L46: notepad — GetNotepadList includeAll multi-world not implemented. Mushkin: `world_notepads.cpp` (DEFERRED: multi-world is a future feature)

## HIGH (Arrays audit — 2026-03-21)

- [x] H37: arrays — ArrayListAll/ArrayListKeys/ArrayListValues return empty table {} when empty (original returns nil). Mushkin: `world_arrays.cpp:344-420`
- [x] H38: arrays — ArrayImport missing table-import overload (only string form). Mushkin: `world_arrays.cpp:565+`

## MEDIUM (Arrays audit — 2026-03-21)

- [x] M58: arrays — ArrayImport delimiter is required (original optional, default ",")" Mushkin: `world_arrays.cpp`

## LOW (Arrays audit — 2026-03-21)

- [x] L47: arrays — ArrayExport/ArrayExportKeys error-check ordering (delimiter before array). Mushkin: `world_arrays.cpp`

## HIGH (Database audit — 2026-03-21)

- [x] H39: database — DatabaseColumnValues returns {} on error/no-row (original returns nil — breaks if-guards). Mushkin: `world_database.cpp:853-859`
- [x] H40: database — DatabaseColumnNames returns {} on error/no-statement (original returns nil). Mushkin: `world_database.cpp`

## MEDIUM (Database audit — 2026-03-21)

- [x] M59: database — DatabaseError returns "" for DB-not-found (original returns translated error strings for codes -1 to -7). Mushkin: `world_database.cpp:623-637`
- [x] M60: database — DatabaseColumnValue BLOB uses raw bytes (original uses sqlite3_column_text for text encoding). Mushkin: `world_database.cpp`

## LOW (Database audit — 2026-03-21)

- [x] L48: database — DatabaseList returns {} when no DBs (original returns nil). Mushkin: `world_database.cpp`
- [x] L49: database — DatabaseColumnValue INTEGER uses lua_pushnumber not lua_pushinteger. Mushkin: `world_database.cpp`

## HIGH (GetInfo/Options audit — 2026-03-21)

- [x] H41: options — GetOption returns nil for unknown options (original returns -1, changing truthiness). Mushkin: `world_settings.cpp:1630-1635`

## MEDIUM (GetInfo/Options audit — 2026-03-21)

- [x] M61: options — SetOption silently clamps out-of-range values (original returns eOptionOutOfRange). Mushkin: `world_settings.cpp:1583-1591`
- [x] M62: options — SetOption/SetAlphaOption missing OPT_FIX_*/OPT_UPDATE_* side effects (font reload, wrap resize, view repaint). Mushkin: `world_settings.cpp`
- [x] M63: info — GetInfo codes 301-306 (dates) return Unix epoch seconds instead of OLE DATE doubles. Mushkin: `world_settings.cpp:1480-1529`

## LOW (GetInfo/Options audit — 2026-03-21)

- [x] L50: info — GetInfo code 232 (hi-res timer) uses epoch ms instead of boot-relative. Mushkin: `world_info.cpp`
- [~] L51: info — GetInfo code 294 (keyboard modifiers) L/R keys always 0 on non-Windows. Mushkin: `world_info.cpp` (DEFERRED: Qt platform limitation — queryKeyboardModifiers() doesn't distinguish L/R. Needs platform-specific APIs: CGEvent on macOS, XKB on Linux)
- [x] L52: info — GetInfo code 297 (timer frequency) returns 1000 instead of ~10MHz. Mushkin: `world_info.cpp`

## HIGH (Accelerators audit — 2026-03-21)

- [x] H42: accel — Accelerator() default sendTo is 12 (eSendToScript) not 10 (eSendToExecute). Mushkin: `world_ui_control.cpp:113-115`

## MEDIUM (Accelerators audit — 2026-03-21)

- [x] M64: accel — AcceleratorTo missing send_to range validation. Mushkin: `world_ui_control.cpp`
- [x] M65: accel — AcceleratorList includes user/macro/keypad entries (original only script-registered). Mushkin: `world_ui_control.cpp`
- [x] M66: accel — AcceleratorList sendTo omit condition uses 12 instead of 10. Mushkin: `world_ui_control.cpp` (fixed as part of H42)
- [x] M67: accel — Numpad0-9 mapped to regular digit keys, not numpad keys. Mushkin: `accelerator_manager.cpp`
- [~] M68: accel — Macro `insert` type collapsed into `replace` (sendTo distinction lost). Mushkin: `macro_keypad_compat.cpp` (DEFERRED: needs "append to input" mode in InputView, not just a sendTo change)
- [x] M69: accel — m_keypad_enable flag not wired (exists but unchecked). Mushkin: `world_document.h`

## LOW (Accelerators audit — 2026-03-21)

- [x] L53: accel — AcceleratorList key string uses stored casing, not reconstructed. Mushkin: `world_ui_control.cpp`
- [x] L54: accel — QShortcut context WidgetWithChildrenShortcut vs global Win32 table. Mushkin: `accelerator_manager.cpp`

## HIGH (Colors audit — 2026-03-21)

- [x] H43: colors — ColourNameToRGB returns white for unknown names (original returns -1). Mushkin: `world_colors.cpp:124-125`
- [x] H44: colors — RGBColourToName returns "" for unrecognized colors (original returns "#RRGGBB"). Mushkin: `world_colors.cpp:216`
- [x] H45: colors — Color name table missing ~100 CSS/X11 names (only ~50 vs original's 148). Mushkin: `world_colors.cpp`

## MEDIUM (Colors audit — 2026-03-21)

- [x] M70: colors — SetCustomColourName out-of-range returns 30009 instead of 30026 (eOptionOutOfRange). Mushkin: `world_document.cpp:1563`
- [x] M71: colors — NoteColourName missing palette→RGB state transition before overwriting. Mushkin: `world_colors.cpp`
- [x] M72: colors — SetBackgroundImage missing .bmp/.png extension check and eFileNotFound/eCouldNotOpenFile. Mushkin: `world_settings.cpp`

## MEDIUM (UI Dialogs audit — 2026-03-21)

- [x] M73: ui — ANSI color picker in Output tab not wired to WorldDocument colors. Mushkin: `world_properties_dialog.cpp`
- [x] M74: ui — Echo color combo not wired to m_output.echo_colour. Mushkin: `world_properties_dialog.cpp`
- [x] M75: ui — Scripting tab missing all entry point fields (OnWorldOpen, etc.). Mushkin: `world_properties_dialog.cpp`
- [~] M76: ui — 8 world prefs pages absent (Macros, Keypad, MXP, Notes, Variables, Auto-say, Printing, Info). Mushkin: `world_properties_dialog.cpp` (DEFERRED: requires creating 8 new tab pages with many widgets each — dedicated UI sprint needed)
- [x] M77: ui — Input tab missing speed walk, command stacking, spam prevention fields. Mushkin: `world_properties_dialog.cpp`
- [x] M78: ui — Output tab missing ~20 fields (wrap, UTF-8, maxlines, beeps, NAWS, terminal ID). Mushkin: `world_properties_dialog.cpp`
- [x] M79: ui — RecallDialog missing Save/Save As functionality. Mushkin: `recall_dialog.cpp`

## LOW (UI Dialogs audit — 2026-03-21)

- [x] L55: ui — Log dialog missing "not logging output" confirmation warning. Mushkin: log dialog
- [x] L56: ui — Plugin Show Info opens popup instead of notepad pane. Mushkin: `plugin_dialog.cpp`
- [x] L57: ui — Plugin Remove adds confirmation not in original. Mushkin: `plugin_dialog.cpp`
- [x] L58: ui — World Properties OK always marks modified (original checks for changes). Mushkin: `world_properties_dialog.cpp`
- [x] L59: ui — Recall Search accepts with all line types unchecked. Mushkin: recall search

## HIGH (MXP Engine audit — 2026-03-21)

- [x] H46: mxp — secure_once mode never resets (MXP_Restore_Mode checks eMXP_perm_secure instead of eMXP_secure_once). Mushkin: `mxp_engine.cpp:986`
- [x] H47: mxp — `<send>` classified as TAG_OPEN (should be secure-only — blocks secure servers). Mushkin: `mxp_engine.cpp` element init
- [x] H48: mxp — Entity table missing ~170 HTML entities (ISO-8859-1 Latin set). Mushkin: `mxp_engine.cpp` InitializeMXPEntities
- [x] H49: mxp — Entity names case-insensitive (original is case-sensitive). Mushkin: `mxp_engine.cpp` (FALSE POSITIVE: entity lookup already case-sensitive, toLower only on element names)
- [~] H50: mxp — `</var>` close does not set MXP variable (TODO stub). Mushkin: `mxp_engine.cpp` (DEFERRED: requires MXP tag close text capture architecture — tied to broader MXP gap)

## MEDIUM (MXP Engine audit — 2026-03-21)

- [x] M80: mxp — Color state not reset on MXP_Off (stale colors persist). Mushkin: `mxp_engine.cpp`
- [x] M81: mxp — Open→secure mode transition does not close open tags (TODO commented out). Mushkin: `mxp_engine.cpp:396-399`
- [x] M82: mxp — MXP_Secure() returns true for locked mode (should be false). Mushkin: `mxp_engine.cpp`
- [x] M83: mxp — MXP_CloseOpenTags at EOL closes secure tags (original stops at secure). Mushkin: `mxp_engine.cpp`
- [x] M84: mxp — `<FONT color=Red,Blink>` comma-separated style parsing missing. Mushkin: `mxp_engine.cpp`
- [x] M85: mxp — VERSION/SUPPORT responses implemented (AFK still pending — needs idle tracking). Mushkin: `mxp_engine.cpp`
- [x] M86: mxp — Missing elements: em, strong, filter, option, recommend_option, xch_page, xch_pane. Mushkin: `mxp_engine.cpp`
- [x] M87: mxp — Positional argument lookup missing (only named lookup). Mushkin: `mxp_engine.cpp`

## HIGH (Scripting Engine audit — 2026-03-21)

- [x] H51: scripting — `os.exit()` not replaced with stub — plugins can terminate the application. Mushkin: `script_engine.cpp`
- [~] H52: scripting — `luaopen_bc` (big number library) not registered — plugins using bc.* fail. Mushkin: `script_engine.cpp` (DEFERRED: requires integrating GNU bc library — lbc.c + number.c/h into build)

## MEDIUM (Scripting Engine audit — 2026-03-21)

- [x] M88: scripting — `DisableDLLs` sandbox not applied (package.loadlib, io.popen, debug.debug live). Mushkin: `script_engine.cpp`
- [x] M89: scripting — Per-plugin m_iScriptTimeTaken never updated — GetPluginInfo("scripttime") returns 0. Mushkin: `script_engine_callbacks.cpp:297-299`
- [x] M90: scripting — Sandbox script (m_strLuaScript from global prefs) not executed at init. Mushkin: `script_engine.cpp`
- [x] M91: scripting — Script timing uses nanoseconds but reporting may assume CPU ticks. Mushkin: `script_engine_callbacks.cpp`

## LOW (Scripting Engine audit — 2026-03-21)

- [~] L60: scripting — No modal error dialog for first script error (always output window). Mushkin: `script_engine_callbacks.cpp` (DEFERRED: requires new CScriptErrorDlg dialog + m_bScriptErrorsToOutputWindow state. LOW impact — errors still display in output window)
- [x] L61: scripting — Error color is darkorange instead of orangered. Mushkin: `script_engine_callbacks.cpp`
- [~] L62: scripting — Include file line numbers offset in error messages (prepended to script). Mushkin: `world_document_plugins.cpp` (DEFERRED: inherent to script concatenation — original has same behavior. Would need separate luaL_loadbuffer calls per include file)

## HIGH (World File Edge Cases audit — 2026-03-21)

- [x] H53: defaults — `log_output` default is false (original true — new worlds silently don't log). Mushkin: `config_options.cpp:211`
- [x] H54: defaults — World ID empty at construction (original generates immediately — GetWorldID returns ""). Mushkin: `world_document.cpp:221`

## MEDIUM (World File Edge Cases audit — 2026-03-21)

- [x] M92: defaults — `show_bold` default true (original false — bold text renders differently). Mushkin: `config_options.cpp:254`
- [x] M93: defaults — `unpause_on_send` default false (original true — send doesn't unpause). Mushkin: `world_document.cpp:246`
- [x] M94: defaults — `m_bScrollBarWanted` init false (original true — scrollbar hidden). Mushkin: `world_document.cpp:491`
- [x] M95: defaults — `tool_tip_visible_time` ctor overrides table (30000 vs 5000). Mushkin: `world_document.cpp:255`
- [x] M96: defaults — `m_bDisconnectOK` init false (original true — may trigger premature reconnect). Mushkin: `world_document.cpp:396`

## LOW (World File Edge Cases audit — 2026-03-21)

- [x] L63: defaults — `m_bRecallCommands/Output/Notes` init false (original true). Mushkin: `world_document.cpp`
- [x] L64: xml — Base64-encoded world file loading not supported. Mushkin: `xml_serialization.cpp` (FALSE POSITIVE: already implemented — _base64 sentinel + QByteArray::fromBase64 decoding in place)
- [x] L65: defaults — `terminal_identification` default "mushkin" (original "mushclient"). Mushkin: `config_options.cpp`

## MEDIUM (Cleanup — error code constants)

- [x] M97: cleanup — Extract error code enum to shared `src/utils/error_codes.h`. Fixed 6 wrong error codes + 14 bare magic numbers. `miniwindow.cpp`, `world_document.cpp`, `accelerator_manager.cpp`, `world_variables.cpp`, `world_document_plugins.cpp`, `world_logging.cpp`
- [x] M98: database — DatabaseError translates SQLITE_ROW/SQLITE_DONE to "row ready"/"finished" matching original. Mushkin: `world_database.cpp`
## v2 Runtime Audit Findings (2026-03-22)

### HIGH (4)
- [~] H56: getinfo -- GetInfo(21) speed_walk_prefix: world file config difference, not code bug. Mushkin: src/world/lua_api/world_settings.cpp
- [x] H57: getinfo -- GetInfo(271) background_colour returns 4278190080 instead of -1. Mushkin: src/world/lua_api/world_settings.cpp
- [x] H58: getinfo -- GetInfo(37) auto_say_string default changed from "say " to "" to match original. Mushkin: src/world/world_document.h
- [x] H59: error_codes -- ePluginCouldNotSaveState Lua value changed from 30038 to 30037 to match original. Mushkin: src/utils/error_codes.h

### MEDIUM (2)
- [~] M100: getinfo -- GetInfo(53) "Ready" was runtime state from a plugin, not a code bug. Default is correctly empty.. Mushkin: src/world/lua_api/world_settings.cpp
- [~] M101: lua_environment -- package.loaders count 5 is by design (backslash normalizer searcher). No plugins check loader count.. The backslash normalizer adds an extra searcher. Verify this doesn't break plugins that check loader count.

### LOW (0)
(error_codes MISSING items are all chat-related codes — chat system not implemented, deferred)
(error_codes EXTRA items are intentional Mushkin additions)
(lua_environment jit_version difference is expected — different LuaJIT builds)

## v2 Audit: XML Serialization (2026-03-22)

### HIGH
- [x] H60: xml -- Missing "enabled" attribute now preserves constructor default (TRUE) for triggers/aliases/timers. Mushkin: xml_serialization.cpp:234,509
- [x] H61: xml -- keep_evaluating default changed from TRUE to FALSE for triggers and aliases. Triggers continue evaluating instead of stopping. Mushkin: trigger.cpp:44, xml_serialization.cpp:246
- [x] H62: xml -- Variable duplicate handling changed from SKIP to OVERWRITE matching original. State restore fails to update existing variables. Mushkin: xml_serialization.cpp:870-880
- [~] H63: xml -- Non-plugin include files not loaded. DEFERRED: requires recursive XML loading with mask flags, deduplication, and two-pass system. Needs dedicated implementation plan.
- [x] H64: xml -- XML structure fixed: triggers/aliases/timers/variables/colours now siblings of <world>. Original MUSHclient cannot read Mushkin-saved files. Mushkin: xml_serialization.cpp:373-428
- [x] H65: xml -- Trigger/alias/timer internal names now lowercased for case-insensitive lookup. Case-insensitive lookup by name (GetTriggerInfo, DeleteTrigger) may fail. Mushkin: xml_serialization.cpp:232-233,504-506

### MEDIUM
- [x] M102: xml -- Named color parsing now supported via ColourNameToRGB (handles 150+ named colors + hex). Mushkin: xml_serialization.cpp:63-79
- [~] M103: xml -- Pre-v3.35 legacy attributes. DEFERRED: 22+ year old format, extremely unlikely to encounter.. Mushkin: xml_serialization.cpp (absent)
- [~] M104: xml -- Version attribute "mushkin_version" is intentional branding. world_file_version still correct for compatibility.. Mushkin: xml_serialization.cpp:266
- [x] M105: xml -- Trigger/alias with empty match text now skipped with warning on load. Mushkin: xml_serialization.cpp:196-420
- [x] M106: xml -- IsArchiveXML now checks all 17 signature patterns matching original. Mushkin: xml_serialization.cpp:151-152
- [x] M107: xml -- date_saved now written to world file (respects omit_date_from_save_files). Mushkin: xml_serialization.cpp:261-267
- [x] M108: xml -- m_bLoaded now set to true after successful LoadWorldXML. Mushkin: world_document.cpp:286
- [x] M109: xml -- PluginListChanged now called once after all plugins loaded (batch mode). Mushkin: world_document_plugins.cpp:568
- [~] M110: xml -- Load order depends on file element order. DEFERRED: only matters for hand-edited files; well-formed MUSHclient files are always in correct order.. Mushkin: xml_serialization.cpp

### LOW
- [ ] L67: xml -- Timer seconds saved with 4 decimal places instead of 2. Mushkin: xml_serialization.cpp:625
- [ ] L68: xml -- Boolean/number attributes always written vs only-when-non-default. Mushkin: xml_serialization.cpp:97-181
- [ ] L69: xml -- XML encoding is UTF-8 instead of iso-8859-1. Mushkin: xml_serialization.cpp:253
- [ ] L70: xml -- Base64 clipboard XML detection missing. Mushkin: (absent)
- [ ] L71: xml -- IsArchiveXML uses case-insensitive contains() instead of case-sensitive prefix match. Mushkin: xml_serialization.cpp:154-159

## v2 Audit: Logging (2026-03-22)

### HIGH
- [x] H66: logging -- eSendToLogFile now adds newline and checks !log_raw. Mushkin: src/world/world_sendto.cpp:140-144
- [x] H67: logging -- Retrospective logging now checks both LOG_LINE and NOTE_OR_COMMAND flags. Mushkin: src/world/world_logging.cpp:743
- [x] H68: logging -- Trigger omit_from_log now checks !multi_line matching original. Mushkin: src/world/world_trigger_execution.cpp:214

### MEDIUM
- [x] M111: logging -- FormatTime percent escaping fixed: % in values no longer doubled. Mushkin: src/world/world_logging.cpp:77-138
- [x] M112: logging -- HTML world name header now escaped via FixHTMLString. Mushkin: src/world/world_logging.cpp:244
- [~] M113: logging -- LogLineInHTMLcolour single Line. DEFERRED: multi-line paragraph iteration requires understanding Mushkin line buffer architecture vs original CLine linked list.. Mushkin: src/world/world_logging.cpp:504-602
- [~] M114: logging -- Retrospective HTML tag structure differs (<span> vs <font>). Cosmetic difference, functionally equivalent.. Mushkin: src/world/world_logging.cpp:785-787

### LOW
- [ ] L72: logging -- CSS spacing in HTML colour span tags differs (no spaces after colons). Mushkin: src/world/world_logging.cpp:561
- [ ] L73: logging -- WriteLog newline: Mushkin correctly detects trailing newline; original always appends for 2+ char messages. Mushkin: src/world/world_logging.cpp:392
- [ ] L74: logging -- flushLogIfNeeded flushes every call instead of every 120 seconds. Mushkin: src/world/world_document.cpp:2966-2973
- [ ] L75: logging -- LogSend bypasses alias processing (calls sendToMud directly). Mushkin: src/world/lua_api/world_logging_opts.cpp:273

## v2 Audit: Command Processing (2026-03-22)

### HIGH
- [x] H69: commands -- L_Send() now routes through SendMsg() for full pipeline (echo/log/spam/callbacks). Mushkin: src/world/lua_api/world_network.cpp:44
- [x] H70: commands -- L_LogSend() now routes through SendMsg() for full pipeline. Mushkin: src/world/lua_api/world_logging_opts.cpp:273
- [x] H71: commands -- Queue prefix encoding now uses E/N/I/W scheme matching original doc.h:241-249. Mushkin: src/world/world_document.cpp:962-977
- [x] H72: commands -- Execute() now saves/restores m_CurrentPlugin with RAII scope guard. Mushkin: src/world/world_document.cpp:1331

### MEDIUM
- [x] M115: commands -- ON_PLUGIN_SENT now fires before echo/log/send matching original. Mushkin: src/world/world_document.cpp:1232-1235
- [x] M116: commands -- SendMsg() now has m_bPluginProcessingSent re-entrancy guard. Mushkin: src/world/world_document.cpp:934
- [x] M117: commands -- m_bNoEcho now checked in SendMsg before queue encoding. Mushkin: src/world/world_document.cpp:1189
- [x] M118: commands -- Backslash escape sequences now applied for typed commands when enabled. Mushkin: src/ui/views/world_widget.cpp:508
- [x] M119: commands -- m_iExecutionDepth now reset to 0 for all hand-typed commands. Mushkin: src/ui/views/world_widget.cpp:508
- [x] M120: commands -- Removed hardcoded "/" immediate prefix from Execute() (not in original). Mushkin: src/world/world_document.cpp:1458-1463

### LOW
- [ ] L76: commands -- German umlaut translation not implemented in SendMsg. Mushkin: src/world/world_document.cpp:934
- [ ] L77: commands -- addToCommandHistory rejects whitespace-only strings; original accepts them. Mushkin: src/world/world_document.cpp:2399
- [ ] L78: commands -- DoSendMsg sends \n instead of \r\n. Mushkin: src/world/world_document.cpp:903,1218
- [ ] L79: commands -- Command history stores processed command instead of original typed command. Mushkin: src/world/world_document.cpp:1493
- [ ] L80: commands -- Execute() doesn't check empty lines for reconnect prompt. Mushkin: src/world/world_document.cpp:1438

## v2 Audit: Speedwalks (2026-03-22)

### HIGH
- [x] H73: speedwalks -- eSendToSpeedwalk now routes through SendMsg with echo/log flags and error handling. Mushkin: src/world/world_sendto.cpp:169-174

### MEDIUM
- [x] M121: speedwalks -- SetSpeedWalkDelay now clamps to 0-30000 range. Mushkin: src/world/connection_manager.cpp:380-394
- [x] M122: speedwalks -- ReverseSpeedwalk now handles all directions (n/s/e/w/u/d + full names + diagonals). Mushkin: src/world/speedwalk_engine.cpp:309-334
- [x] M123: speedwalks -- Speed walk pipeline now checks connection before processing. Mushkin: src/world/world_document.cpp:1394-1395
- [~] M124: speedwalks -- Speed walk error shows note() instead of modal dialog. Intentional UX improvement — inline errors are less disruptive than modals.. Mushkin: src/world/world_document.cpp:1389-1390

### LOW
- [ ] L81: speedwalks -- GetSpeedWalkDelay returns lua_Integer instead of short. Mushkin: src/world/lua_api/world_logging_opts.cpp:297-301
- [ ] L82: speedwalks -- Speed walk prefix empty guard prevents all commands being parsed as speedwalks. Mushkin: src/world/world_document.cpp:1379-1380
- [ ] L83: speedwalks -- RemoveBacktracks cannot cancel custom parenthesized directions. Mushkin: src/world/speedwalk_engine.cpp:409-439

## v2 Audit: Miniwindows (2026-03-22)

### HIGH
- [x] H74: miniwindows -- WindowInfo case 6 now returns temporarilyHide instead of !show. Mushkin: world_miniwindow_lifecycle.cpp:418
- [x] H75: miniwindows -- WindowResize now preserves existing content by copying old image back. Mushkin: miniwindow.cpp:168-192
- [x] H76: miniwindows -- WindowText return value now clipped to min(textWidth, rectWidth). Mushkin: miniwindow.cpp:1103
- [x] H77: miniwindows -- WindowFont now deletes font when name="" and size=0. Mushkin: miniwindow.cpp:1043-1058
- [x] H78: miniwindows -- CircleOp chord/pie now use GDI point-based params converted to Qt angles via atan2. Mushkin: miniwindow.cpp:517-525
- [x] H79: miniwindows -- CircleOp action 6 now returns eUnknownOption (use WindowArc instead). Mushkin: miniwindow.cpp:608-611
- [~] H80: miniwindows -- BlendImage source-rect clamping order matches original (both clamp before FixRight; same behavior). Verified as false positive.. Mushkin: miniwindow.cpp:1377-1396

### MEDIUM
- [x] M125: miniwindows -- WindowInfo cases 12/13 now add +1 for Qt QRect to Windows CRect compatibility. Mushkin: world_miniwindow_lifecycle.cpp:442-447
- [x] M126: miniwindows -- RectOp action 2 (fill) uses brushColor/penColor fallback instead of always Colour1. Mushkin: miniwindow.cpp:291-296
- [x] M127: miniwindows -- RectOp action 3 (InvertRect) XORs with penColor instead of white. Mushkin: miniwindow.cpp:299-302
- [x] M128: miniwindows -- Pen style validation missing. Mushkin: miniwindow.cpp:629-646
- [~] M129: miniwindows -- WindowFont weight returns Qt QFont::Weight scale instead of Windows FW_ values. Mushkin: miniwindow.cpp:1157 (NOT A BUG: Qt 6 Weight enum uses CSS-compatible values that match Windows FW_* exactly: Normal=400, Bold=700)
- [x] M130: miniwindows -- Gradient bounds check rejects valid coordinates that original would clip. Mushkin: miniwindow.cpp:973-974
- [~] M131: miniwindows -- Filter MonoNoise (op 2) applies independent noise per channel instead of same noise. Mushkin: miniwindow.cpp:2631-2647 (NOT A BUG: code already uses single noise value for all 3 channels, matching original)
- [x] M132: miniwindows -- Filter blur (ops 3,25,26) ignores Options parameter for h/v direction. Mushkin: miniwindow.cpp:2649-2680
- [ ] M133: miniwindows -- Filter sharpen/edge/emboss use single-pass instead of separable convolution. Mushkin: miniwindow.cpp:2682-2772
- [x] M134: miniwindows -- DeleteAllHotspots/DeleteHotspot don't clear callbackPlugin. Mushkin: world_miniwindow_images.cpp:906-948
- [x] M135: miniwindows -- WindowInfo case 21 returns current time instead of installation time. Mushkin: world_miniwindow_lifecycle.cpp:488-493

### LOW
- [ ] L84: miniwindows -- SetPixel returns eBadParameter for out-of-bounds instead of silently succeeding. Mushkin: miniwindow.cpp:790-792
- [ ] L85: miniwindows -- GetPixel returns 0 for out-of-bounds instead of -1 (CLR_INVALID). Mushkin: miniwindow.cpp:813-814
- [ ] L86: miniwindows -- WindowFont size=0 creates 1px font instead of defaulting to 10pt. Mushkin: miniwindow.cpp:1047
- [ ] L87: miniwindows -- LoadImage accepts all Qt formats instead of only .bmp/.png. Mushkin: miniwindow.cpp:1217-1234
- [ ] L88: miniwindows -- Arc atan2 conversion may draw wrong arc segment for some point configs. Mushkin: miniwindow.cpp:662-704

## v2 Audit: Sound/MSP (2026-03-22)

### HIGH
- [x] H81: sound -- GetSoundStatus now returns -3 when audio engine not initialized. Mushkin: src/world/sound_manager.cpp:414-438
- [x] H82: sound -- GetSoundStatus now returns 1 for looping sounds (bug-compatible with original). Mushkin: src/world/sound_manager.cpp:432-437
- [x] H83: sound -- PlaySoundMemory now passes buffer, loop, volume, pan to playSound; uses unique temp files. Mushkin: src/world/lua_api/world_sounds.cpp:140-170
- [x] H84: sound -- PlaySoundMemory now uses per-buffer unique temp files (fixed in H83). Mushkin: src/world/lua_api/world_sounds.cpp:158-159

### MEDIUM
- [x] M136: sound -- PlaySound filename is required (luaCheckQString); original allows nil for sound adjustment. Mushkin: src/world/lua_api/world_sounds.cpp:47
- [ ] M137: sound -- play_sounds_in_background option parsed but never applied. Mushkin: src/world/sound_manager.cpp
- [ ] M138: sound -- MXP sound tag ignores volume, loops, priority, URL parameters. Mushkin: src/world/mxp_engine.cpp:2039-2067
- [ ] M139: sound -- MSP MUSIC hardcodes buffer 1; MSP STOP stops ALL buffers instead of per-channel. Mushkin: src/world/telnet_parser.cpp:1150-1157
- [ ] M140: sound -- releaseInactiveSoundBuffers is no-op; sounds stay marked playing forever. Mushkin: src/world/sound_manager.cpp:199-204

### LOW
- [ ] L89: sound -- PlaySound filename path length limit (127 chars) not enforced. Mushkin: src/world/sound_manager.cpp:298-301

## v2 Audit: Notepad (2026-03-22)

### HIGH
- [x] H85: notepad -- CloseNotepad now returns boolean (true=success). Mushkin: src/world/world_notepad.cpp:216-248
- [x] H86: notepad -- NotepadColour now returns boolean. Mushkin: src/world/world_notepad.cpp:322-341
- [x] H87: notepad -- NotepadFont now returns boolean. Mushkin: src/world/world_notepad.cpp:301-312
- [x] H88: notepad -- NotepadReadOnly now returns boolean. Mushkin: src/world/world_notepad.cpp:350-360
- [x] H89: notepad -- NotepadSaveMethod now returns boolean. Mushkin: src/world/world_notepad.cpp:369-385
- [x] H90: notepad -- SaveNotepad now returns boolean. Mushkin: src/world/world_notepad.cpp:258-269

### MEDIUM
- [x] M141: notepad -- ReplaceNotepad returns boolean; original returns nil. Mushkin: src/world/lua_api/world_notepads.cpp:74-80
- [x] M142: notepad -- NotepadColour rejects empty color strings; original treats as no-op. Mushkin: src/world/world_notepad.cpp:335-336
- [x] M143: notepad -- New notepads ignore global notepad color preferences. Mushkin: src/world/notepad_widget.cpp:30-35

### LOW
- [ ] L90: notepad -- GetNotepadList(true) does not enumerate other worlds' notepads (deferred, single-world). Mushkin: src/world/world_notepad.cpp:277-289

## v2 Audit: Arrays (2026-03-22)

### MEDIUM
- [x] M144: arrays -- ArrayExport delimiter is required; original defaults to ",". Mushkin: src/world/lua_api/world_arrays.cpp:459
- [x] M145: arrays -- ArrayExportKeys delimiter is required; original defaults to ",". Mushkin: src/world/lua_api/world_arrays.cpp:517
- [x] M146: arrays -- ArrayImport from Lua table silently converts integer keys to strings; original raises error. Mushkin: src/world/lua_api/world_arrays.cpp:592-593

### LOW
- [ ] L91: arrays -- ArrayExport/ArrayExportKeys check delimiter validity before empty check (different error for empty+bad delimiter). Mushkin: src/world/lua_api/world_arrays.cpp:470-481
- [ ] L92: arrays -- ArrayImport from table always returns eOK; original returns eImportedWithDuplicates. Mushkin: src/world/lua_api/world_arrays.cpp:599

## v2 Audit: Database (2026-03-22)

### HIGH
- [x] H91: database -- DatabaseClose now nulls pointers after manual finalize/close to prevent double-free in destructor. Mushkin: src/world/lua_api/world_database.cpp:127-136

### MEDIUM
- [x] M147: database -- DatabaseGetField does not reset bValidRow/iColumns state on database struct. Mushkin: src/world/lua_api/world_database.cpp:913-957
- [x] M148: database -- DatabaseError missing switch cases for DATABASE_ERROR_* negative codes. Mushkin: src/world/lua_api/world_database.cpp:642-656

### LOW
- [ ] L93: database -- DatabaseColumnValue uses int64 instead of int32 (strictly better but different). Mushkin: src/world/lua_api/world_database.cpp:746
- [ ] L94: database -- DatabaseOpen does not check for NULL db handle after sqlite3_open_v2. Mushkin: src/world/lua_api/world_database.cpp:83

## v2 Audit: GetInfo/GetOption/SetOption (2026-03-22)

### HIGH
- [~] H92: options -- OPT_PLUGIN_CANNOT_READ flag defined but no options use it in original. No behavioral impact.. Mushkin: world_settings.cpp:1653-1683
- [x] H93: options -- GetOption no longer falls through to alpha options; returns -1 for non-numeric option names. Mushkin: world_settings.cpp:1671-1678
- [x] H94: options -- SetOption/GetOption now applies +1/-1 OPT_CUSTOM_COLOUR adjustment for echo_colour. Mushkin: world_settings.cpp:1613
- [x] H95: options -- GetInfo dates (301-306) now return Unix timestamps (toSecsSinceEpoch). Mushkin: world_settings.cpp:1495-1546
- [x] H96: options -- SetOption OPT_FIX_OUTPUT_BUFFER now calls trimLineBuffer() to trim excess lines immediately. Mushkin: world_settings.cpp:1618-1621

### MEDIUM
- [x] M149: options -- SetOption missing OPT_FIX_INPUT_WRAP side effect. Mushkin: world_settings.cpp:1616
- [x] M150: options -- SetOption missing OPT_FIX_SPEEDWALK_DELAY side effect. Mushkin: world_settings.cpp:1616
- [x] M151: options -- SetOption missing OPT_USE_MXP side effect. Mushkin: world_settings.cpp:1616
- [x] M152: options -- SetOption missing OPT_FIX_TOOLTIP side effects. Mushkin: world_settings.cpp:1616
- [x] M153: options -- GetInfo(212) returns configured font height instead of rendered pixel height. Mushkin: world_settings.cpp:992
- [x] M154: options -- GetInfo(236/237) command selection start/end values differ from original. Mushkin: world_settings.cpp:1116-1128
- [x] M155: options -- SetOption does not call SetModifiedFlag when option changes. Mushkin: world_settings.cpp:1613

### LOW
- [ ] L95: options -- Default font names "Courier New" instead of "FixedSys". Mushkin: config_options.cpp:333,361
- [ ] L96: options -- GetInfo(55) returns SetTitle value instead of document title. Mushkin: world_settings.cpp:612
- [ ] L97: options -- Extra numeric/alpha options in Mushkin (proxy, remote, log_lines). Mushkin: config_options.cpp

## v2 Audit: Accelerators (2026-03-22)

### HIGH
- [x] H97: accelerators -- Dispatch now uses eSendToExecute (10) instead of hardcoded 12. Mushkin: src/ui/views/world_widget.cpp:162
- [x] H98: accelerators -- Dispatch now suppresses auto-say during execution. Mushkin: src/ui/views/world_widget.cpp:158-170
- [x] H99: accelerators -- Dispatch now sets m_iCurrentActionSource to eUserAccelerator. Mushkin: src/world/world_document.h:168-178
- [x] H100: accelerators -- Dispatch now sets/restores m_CurrentPlugin for plugin-registered accelerators. Mushkin: src/ui/views/world_widget.cpp:158-170

### MEDIUM
- [ ] M156: accelerators -- AcceleratorList key name format differs (Qt vs MUSHclient names). Mushkin: src/world/accelerator_manager.cpp:268-272
- [x] M157: accelerators -- No maximum accelerator count limit (original caps at 1000). Mushkin: src/world/accelerator_manager.cpp:274-317
- [x] M158: accelerators -- Dispatch now passes omit_from_output=true and omit_from_log=true (fixed with H98). Mushkin: src/ui/views/world_widget.cpp:167
- [x] M159: accelerators -- Dispatch now passes "Accelerator" description string (fixed with H98). Mushkin: src/ui/views/world_widget.cpp:167

### LOW
- [ ] L98: accelerators -- Extra key name aliases accepted (e.g., "Escape" in addition to "Esc"). Mushkin: src/world/accelerator_manager.cpp:62-63

## v2 Audit: Colors (2026-03-22)

### MEDIUM
- [x] M160: colors -- FilterPixel mode 1 (Noise) uses same random value for all channels (identical to MonoNoise). Mushkin: src/world/lua_api/world_misc.cpp:2279-2288
- [x] M161: colors -- FilterPixel mode 20 (Perceptual Grayscale) missing /3 divisor; results 3x brighter. Mushkin: src/world/lua_api/world_misc.cpp:2350-2353
- [x] M162: colors -- FilterPixel gamma operations (modes 9/12/15/18) missing negative Options clamping. Mushkin: src/world/lua_api/world_misc.cpp:2302-2306

### LOW
- [ ] L99: colors -- ColourNameToRGB missing "rebeccapurple" (0x663399). Mushkin: src/world/lua_api/world_colors.cpp:34-185
- [ ] L100: colors -- ColourNameToRGB has extra non-standard names (darkyellow, lightmagenta, lightred). Mushkin: src/world/lua_api/world_colors.cpp:75,110,112
- [ ] L101: colors -- ColourNameToRGB empty-string returns white instead of undefined. Mushkin: src/world/lua_api/world_colors.cpp:27-29

## v2 Audit: UI Dialogs (2026-03-22)

### HIGH
- [~] H101: ui -- Custom Colours page missing. DEFERRED: requires new preference page with 32 colour pair pickers + name fields.. Mushkin: no equivalent
- [~] H102: ui -- MXP settings page missing. DEFERRED: requires new preference page. (MXP enable mode, Pueblo, debug, hyperlink settings). Mushkin: no equivalent
- [~] H103: ui -- Auto-say settings page missing. DEFERRED: requires new preference page.. Mushkin: no equivalent
- [x] H104: ui -- Alias edit dialog now has ignore_case, temporary, one_shot, menu checkboxes. Mushkin: src/ui/dialogs/alias_edit_dialog.h
- [x] H105: ui -- Connecting page now has connect method combo and connect text field. Mushkin: src/ui/dialogs/world_properties_dialog.cpp

### MEDIUM
- [ ] M163: ui -- Macros page (function key bindings) missing from world properties. Mushkin: no equivalent
- [ ] M164: ui -- Keypad page (numpad bindings) missing. Mushkin: no equivalent
- [ ] M165: ui -- Notes page (per-world notes field) missing. Mushkin: no equivalent
- [ ] M166: ui -- Info/Statistics page missing (buffer/connection/trigger stats). Mushkin: no equivalent
- [ ] M167: ui -- World properties dialog has NO input validation. Mushkin: src/ui/dialogs/world_properties_dialog.cpp:921-926
- [ ] M168: ui -- Logging page missing ~12 settings (preamble/postamble, per-line formatting). Mushkin: src/ui/dialogs/world_properties_dialog.cpp:325-355
- [ ] M169: ui -- Input tab missing ~15 settings (auto-repeat, spell check, spam prevention, etc.). Mushkin: src/ui/dialogs/world_properties_dialog.cpp:254-322
- [ ] M170: ui -- Output tab missing ~15 settings (beeps, compression, GA conversion, etc.). Mushkin: src/ui/dialogs/world_properties_dialog.cpp:142-252
- [ ] M171: ui -- Scripting page missing MXP callbacks, script prefix, editor path. Mushkin: src/ui/dialogs/world_properties_dialog.cpp:357-416
- [ ] M172: ui -- Trigger edit dialog missing ignore_case and sound file fields. Mushkin: src/ui/dialogs/trigger_edit_dialog.h
- [ ] M173: ui -- Timer edit dialog missing temporary and variable fields. Mushkin: src/ui/dialogs/timer_edit_dialog.h

### LOW
- [ ] L102: ui -- Printing page missing (Windows-specific, limited cross-platform value). Mushkin: no equivalent
- [ ] L103: ui -- Use-default-X checkboxes missing throughout preferences. Mushkin: no equivalent
- [ ] L104: ui -- Import/Export XML buttons missing from trigger/alias/timer list dialogs. Mushkin: no equivalent
- [ ] L105: ui -- Filter functionality missing from list dialogs. Mushkin: no equivalent

## v2 Audit: MXP Engine (2026-03-22)

### HIGH
- [x] H106: mxp -- Security model fixed: secure-only tags now rejected in non-secure mode. Mushkin: mxp_engine.cpp:1279-1284
- [x] H107: mxp -- !ELEMENT and !ENTITY definitions now require secure mode. Mushkin: mxp_engine.cpp:1476-1485
- [~] H108: mxp -- !ATTLIST not implemented. DEFERRED: requires porting BuildArgumentList + element attribute list append. Needs dedicated MXP plan.. Mushkin: mxp_engine.cpp:1476-1485
- [x] H109: mxp -- MXP_Off now closes ALL tags including secure ones via closeAll parameter. Mushkin: mxp_engine.cpp:568-569
- [x] H110: mxp -- SEND tag now falls back to xch_cmd/xch_hint for Pueblo compatibility. Mushkin: mxp_engine.cpp:2003-2018
- [x] H111: mxp -- Hyperlink <A> tag now converts xch_cmd to SEND action for Pueblo. Mushkin: mxp_engine.cpp:2021-2036
- [~] H112: mxp -- </send> - [ ] H112: mxp -- </send> close does not populate &text; replacement (most MUDs use <send>text</send>)text; replacement. DEFERRED: requires MXP style stack (H114) to apply link actions to already-rendered text. Needs dedicated MXP plan.. Mushkin: mxp_engine.cpp:2428-2443
- [~] H113: mxp -- <var> tag close no-op. DEFERRED: requires MXP variable system (variable capture, entity creation, script callbacks). Needs dedicated MXP plan.. Mushkin: mxp_engine.cpp:2190-2198
- [~] H114: mxp -- No MXP style stack. DEFERRED: requires style marker system (save/restore per-tag). Architectural, needs dedicated plan.. Mushkin: mxp_engine.cpp:2414-2425

### MEDIUM
- [ ] M174: mxp -- Entity lookup is case-sensitive; original is partially case-insensitive for custom entities. Mushkin: mxp_engine.cpp:1189-1213
- [ ] M175: mxp -- MXP_On always clears custom elements/entities; original preserves on manual toggle. Mushkin: mxp_engine.cpp:511-538
- [ ] M176: mxp -- <reset> tag only closes open tags, doesn't reset ANSI/paragraph mode. Mushkin: mxp_engine.cpp:2207-2210
- [ ] M177: mxp -- <mxp off> keyword not handled (no-op stub). Mushkin: mxp_engine.cpp:2212-2216
- [ ] M178: mxp -- SUPPORT response does not list sub-attributes. Mushkin: mxp_engine.cpp:2218-2255
- [ ] M179: mxp -- <li> tag does not render bullet/number prefix. Mushkin: mxp_engine.cpp:2293-2298
- [ ] M180: mxp -- <image> tag does not render placeholder text/link. Mushkin: mxp_engine.cpp:2069-2084
- [ ] M181: mxp -- No OnMXP_Error/OnMXP_OpenTag/OnMXP_CloseTag/OnMXP_SetVariable callbacks. Mushkin: TODO stubs
- [ ] M182: mxp -- COLOR tag does not check m_bIgnoreMXPcolourChanges. Mushkin: mxp_engine.cpp:1923-1948
- [ ] M183: mxp -- Hyperlinks missing underline and custom link color. Mushkin: mxp_engine.cpp:2003-2036
- [ ] M184: mxp -- MXP color table has only 15 named colors vs original's 147. Mushkin: mxp_engine.cpp:2545-2598

### LOW
- [ ] L106: mxp -- Extra elements defined (strikeout, sbr) not in original. Mushkin: mxp_engine.cpp:78,88
- [ ] L107: mxp -- <p> tag defined as TAG_COMMAND (self-closing) instead of block element. Mushkin: mxp_engine.cpp:85
- [ ] L108: mxp -- user/password tags have TAG_OPEN flag (conflicts with inverted security). Mushkin: mxp_engine.cpp:134-138
- [ ] L109: mxp -- <send> element missing xch_cmd/xch_hint in argument spec. Mushkin: mxp_engine.cpp:111

## v2 Audit: Scripting Engine (2026-03-22)

### HIGH
- [x] H115: scripting -- Trigger script now supports dotted function names via table traversal. Mushkin: world_trigger_execution.cpp:324
- [x] H116: scripting -- Trigger script now sets m_iCurrentActionSource to eTriggerFired. Mushkin: world_trigger_execution.cpp
- [x] H117: scripting -- Trigger script now uses callLuaWithTraceBack for stack traces. Mushkin: world_trigger_execution.cpp:420
- [x] H118: scripting -- Trigger script errors now display in output window via colourNote. Mushkin: world_trigger_execution.cpp:431-435

### MEDIUM
- [ ] M185: scripting -- Trigger/alias does not reset m_iNoteStyle to NORMAL before callbacks. Mushkin: world_trigger_execution.cpp
- [ ] M186: scripting -- Trigger invocation_count incremented even on script error. Mushkin: world_trigger_execution.cpp:441
- [ ] M187: scripting -- Trigger style runs table (4th callback arg) is always empty. Mushkin: world_trigger_execution.cpp:413-414
- [ ] M188: scripting -- parseLua() does not track per-plugin script timing. Mushkin: script_engine.cpp:1236-1238
- [ ] M189: scripting -- ePluginCouldNotSaveState = 30038 instead of 30037 (duplicates ePluginDoesNotSaveState). Mushkin: script_engine.cpp:912

### LOW
- [ ] L110: scripting -- Error display color is darkorange instead of orangered. Mushkin: script_engine.cpp:1176
- [ ] L111: scripting -- executeLua uses qCDebug instead of m_doc->Trace() for trace output. Mushkin: script_engine_callbacks.cpp:241
- [ ] L112: scripting -- error_code table has extra constants not in original (30055, 30075-30077). Mushkin: script_engine.cpp:922,942-944

## v2 Audit: World File Edge Cases (2026-03-22)

### HIGH
- [~] H119: world-edge -- OnWorldOpen callback. DEFERRED: needs integration with world loading sequence.. Mushkin: src/ui/views/world_widget.cpp:304-346
- [~] H120: world-edge -- Global plugins not loaded. DEFERRED: requires global plugin loading system.. Mushkin: no equivalent code
- [~] H121: world-edge -- Non-plugin includes. DEFERRED: same as H63, requires recursive XML loading.. Mushkin: src/world/xml_serialization.cpp:645-673
- [x] H122: world-edge -- Plugin SaveState() now called for all plugins during world save. Mushkin: src/world/xml_serialization.cpp:233-235

### MEDIUM
- [ ] M190: world-edge -- PluginListChanged called per-plugin instead of once after all loaded. Mushkin: src/world/world_document_plugins.cpp:568
- [ ] M191: world-edge -- No welcome message displayed on world open. Mushkin: src/ui/views/world_widget.cpp:342
- [ ] M192: world-edge -- m_bStartPaused never applied to output view. Mushkin: src/world/world_document.cpp:168
- [ ] M193: world-edge -- Duplicate world ID not detected when opening same file twice. Mushkin: no equivalent
- [ ] M194: world-edge -- m_bSaveWorldAutomatically not honored on close. Mushkin: src/ui/main_window.cpp:2227-2252
- [ ] M195: world-edge -- Variables-only-changed save prompt missing on close. Mushkin: src/ui/main_window.cpp:2227-2252
- [ ] M196: world-edge -- Password prompt dialog not triggered on connect. Mushkin: src/world/connection_manager.cpp:146-165

### LOW
- [ ] L113: world-edge -- No unused attribute/element warnings during XML load. Mushkin: src/world/xml_serialization.cpp
- [ ] L114: world-edge -- Tab-to-space conversion not performed during XML parsing. Mushkin: handled by QXmlStreamReader
- [ ] L115: world-edge -- m_bLoaded flag never set to true after loading. Mushkin: src/world/world_document.cpp:286
- [ ] L116: world-edge -- XML file size limit (100MB) not enforced. Mushkin: src/world/xml_serialization.cpp:477-488

## v2 Audit: Enum/Constant Integrity (2026-03-22)

### HIGH
- [x] H123: enums -- Systematic audit complete: all enum/constant values match original. ActionSource was the only mismatch (already fixed).: compare ALL enum/constant numeric values in Mushkin against original doc.h/OtherTypes.h/errors.h. ActionSource was completely wrong (invented values). Other enums may have the same problem. Check: SendTo, error codes, connect phases, MXP modes, colour types, line flags, style flags, telnet states.
