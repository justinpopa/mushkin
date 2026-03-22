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

### HIGH (43)
- [ ] H56: getinfo -- MISMATCH getinfo.106`: original=`False` mushkin=`True
- [ ] H57: getinfo -- MISMATCH getinfo.108`: original=`False` mushkin=`True
- [ ] H58: getinfo -- MISMATCH getinfo.111`: original=`True` mushkin=`False
- [ ] H59: getinfo -- MISMATCH getinfo.118`: original=`True` mushkin=`False
- [ ] H60: getinfo -- MISMATCH getinfo.120`: original=`False` mushkin=`True
- [ ] H61: getinfo -- MISMATCH getinfo.21`: original=`#` mushkin=`
- [ ] H62: getinfo -- MISMATCH getinfo.271`: original=`-1` mushkin=`4278190080
- [ ] H63: getinfo -- MISMATCH getinfo.286`: original=`73` mushkin=`46
- [ ] H64: getinfo -- MISMATCH getinfo.287`: original=`3` mushkin=`0
- [ ] H65: getinfo -- MISMATCH getinfo.288`: original=`3875` mushkin=`101
- [ ] H66: getinfo -- MISMATCH getinfo.295`: original=`1514` mushkin=`0
- [ ] H67: getinfo -- MISMATCH getinfo.296`: original=`3712` mushkin=`2820
- [ ] H68: getinfo -- MISMATCH getinfo.297`: original=`10000000` mushkin=`1000000000
- [ ] H69: getinfo -- MISMATCH getinfo.298`: original=`3039002` mushkin=`3052000
- [ ] H70: getinfo -- MISMATCH getinfo.299`: original=`1252` mushkin=`65001
- [ ] H71: getinfo -- MISMATCH getinfo.300`: original=`437` mushkin=`65001
- [ ] H72: getinfo -- MISMATCH getinfo.301`: original=`1774209598` mushkin=`46103.835497685184
- [ ] H73: getinfo -- MISMATCH getinfo.302`: original=`1774209595` mushkin=`46103.835497685184
- [ ] H74: getinfo -- MISMATCH getinfo.304`: original=`1774209747` mushkin=`46103.83568287037
- [ ] H75: getinfo -- MISMATCH getinfo.305`: original=`1774209594` mushkin=`46103.83568287037
- [ ] H76: getinfo -- MISMATCH getinfo.306`: original=`1774209595` mushkin=`46103.835497685184
- [ ] H77: getinfo -- MISMATCH getinfo.310`: original=`69` mushkin=`56
- [ ] H78: getinfo -- MISMATCH getinfo.37`: original=`` mushkin=`say 
- [ ] H79: getinfo -- MISMATCH getinfo.53`: original=`` mushkin=`Ready
- [ ] H80: getinfo -- MISMATCH getinfo.54`: original=`C:\Program Files\MUSHclient\worlds\Aardwolf.mcl` mushkin=`/Users/justinpopa/Desktop/Mushkin/worlds/Aardwolf.mcl
- [ ] H81: getinfo -- MISMATCH getinfo.60`: original=`.\worlds\plugins\` mushkin=`/Users/justinpopa/Desktop/Mushkin/worlds/plugins/
- [ ] H82: getinfo -- MISMATCH getinfo.61`: original=`23.111.142.226` mushkin=`
- [ ] H83: getinfo -- MISMATCH getinfo.62`: original=`0.0.0.0` mushkin=`
- [ ] H84: getinfo -- MISMATCH getinfo.64`: original=`C:\Program Files\MUSHclient\` mushkin=`/Users/justinpopa/Desktop/Mushkin/
- [ ] H85: getinfo -- MISMATCH getinfo.72`: original=`5.07-pre` mushkin=`5.06-preview
- [ ] H86: getinfo -- MISMATCH getinfo.73`: original=`Oct 30 2022 00:33:01` mushkin=`Mar 22 2026 10:35:45
- [ ] H87: getinfo -- MISMATCH getinfo.74`: original=`C:\Program Files\MUSHclient\sounds\` mushkin=`/Users/justinpopa/Desktop/Mushkin/sounds/
- [ ] H88: getinfo -- MISMATCH getinfo.76`: original=`C:\Program Files\MUSHclient\\Dina.fon` mushkin=`
- [ ] H89: getinfo -- MISMATCH getinfo.77`: original=`` mushkin=`macOS Tahoe (26.4)
- [ ] H90: getinfo -- MISMATCH getinfo.79`: original=`C:\Program Files\MUSHclient\worlds/plugins/images/bg1.png` mushkin=`/Users/justinpopa/Desktop/Mushkin/worlds/plugins/images/bg1.png
- [ ] H91: getinfo -- MISMATCH getinfo.80`: original=`1.6.37` mushkin=`
- [ ] H92: getinfo -- MISMATCH getinfo.81`: original=
- [ ] H93: getinfo -- MISMATCH getinfo.82`: original=`C:\Program Files\MUSHclient\mushclient_prefs.sqlite` mushkin=`/Users/justinpopa/Desktop/Mushkin/mushclient_prefs.sqlite
- [ ] H94: getinfo -- MISMATCH getinfo.83`: original=`3.39.2` mushkin=`3.52.0
- [ ] H95: getinfo -- MISMATCH getinfo.84`: original=`C:\Program Files\MUSHclient\` mushkin=`
- [ ] H96: getinfo -- MISMATCH getinfo.85`: original=`.\worlds\plugins\state\` mushkin=`/Users/justinpopa/Desktop/Mushkin/worlds/plugins/state/
- [ ] H97: getinfo -- MISMATCH getinfo.87`: original=
- [ ] H98: getinfo -- MISMATCH getinfo.88`: original=`Aardwolf.mcl` mushkin=`

### MEDIUM (49)
- [ ] M100: error_codes -- MISSING error_codes.eCannotCreateChatSocket`: original=`30042
- [ ] M101: error_codes -- MISSING error_codes.eCannotLookupDomainName`: original=`30043
- [ ] M102: error_codes -- MISSING error_codes.eChatAlreadyConnected`: original=`30049
- [ ] M103: error_codes -- MISSING error_codes.eChatAlreadyListening`: original=`30047
- [ ] M104: error_codes -- MISSING error_codes.eChatIDNotFound`: original=`30048
- [ ] M105: error_codes -- MISSING error_codes.eChatPersonNotFound`: original=`30045
- [ ] M106: error_codes -- EXTRA error_codes.eFileNotOpened`: mushkin=`30076
- [ ] M107: error_codes -- EXTRA error_codes.eInvalidColourName`: mushkin=`30077
- [ ] M108: error_codes -- MISSING error_codes.eNoChatConnections`: original=`30044
- [ ] M109: error_codes -- EXTRA error_codes.eNoSuchNotepad`: mushkin=`30075
- [ ] M110: error_codes -- MISMATCH error_codes.ePluginCouldNotSaveState`: original=`30037` mushkin=`30038
- [ ] M111: miniwindows -- MISMATCH miniwindows`: original=
- [ ] M112: options -- MISMATCH options.alpha.auto_say_string`: original=`` mushkin=`say 
- [ ] M113: options -- EXTRA options.alpha.proxy_username`: mushkin=`
- [ ] M114: options -- MISMATCH options.alpha.script_editor_argument`: original=`%file` mushkin=`
- [ ] M115: options -- MISMATCH options.alpha.speed_walk_prefix`: original=`#` mushkin=`
- [ ] M116: options -- MISMATCH options.numeric.append_to_log_file`: original=`-1` mushkin=`1
- [ ] M117: options -- MISMATCH options.numeric.confirm_before_replacing_typing`: original=`0` mushkin=`1
- [ ] M118: options -- MISMATCH options.numeric.detect_pueblo`: original=`1` mushkin=`0
- [ ] M119: options -- MISMATCH options.numeric.echo_colour`: original=`2` mushkin=`1
- [ ] M120: options -- MISMATCH options.numeric.echo_hyperlink_in_output_window`: original=`1` mushkin=`0
- [ ] M121: options -- MISMATCH options.numeric.edit_script_with_notepad`: original=`0` mushkin=`1
- [ ] M122: options -- MISMATCH options.numeric.hyperlink_adds_to_command_history`: original=`1` mushkin=`0
- [ ] M123: options -- MISMATCH options.numeric.hyperlink_colour`: original=`16744448` mushkin=`33023
- [ ] M124: options -- MISMATCH options.numeric.input_font_charset`: original=`1` mushkin=`0
- [ ] M125: options -- MISMATCH options.numeric.keypad_enable`: original=`1` mushkin=`0
- [ ] M126: options -- MISMATCH options.numeric.line_information`: original=`0` mushkin=`1
- [ ] M127: options -- MISMATCH options.numeric.log_lines`: original=`-1` mushkin=`0
- [ ] M128: options -- MISMATCH options.numeric.mud_can_change_link_colour`: original=`1` mushkin=`0
- [ ] M129: options -- MISMATCH options.numeric.output_font_charset`: original=`1` mushkin=`0
- [ ] M130: options -- MISMATCH options.numeric.remote_access_enabled`: original=`-1` mushkin=`0
- [ ] M131: options -- MISMATCH options.numeric.remote_max_clients`: original=`-1` mushkin=`5
- [ ] M132: options -- MISMATCH options.numeric.remote_port`: original=`-1` mushkin=`0
- [ ] M133: options -- MISMATCH options.numeric.remote_scrollback_lines`: original=`-1` mushkin=`100
- [ ] M134: options -- MISMATCH options.numeric.show_bold`: original=`0` mushkin=`1
- [ ] M135: options -- MISMATCH options.numeric.show_connect_disconnect`: original=`1` mushkin=`0
- [ ] M136: options -- MISMATCH options.numeric.show_italic`: original=`0` mushkin=`1
- [ ] M137: options -- MISMATCH options.numeric.show_underline`: original=`0` mushkin=`1
- [ ] M138: options -- MISMATCH options.numeric.timestamp_input_text_colour`: original=`128` mushkin=`8388608
- [ ] M139: options -- MISMATCH options.numeric.timestamp_notes_text_colour`: original=`16711680` mushkin=`255
- [ ] M140: options -- MISMATCH options.numeric.treeview_aliases`: original=`0` mushkin=`1
- [ ] M141: options -- MISMATCH options.numeric.treeview_timers`: original=`0` mushkin=`1
- [ ] M142: options -- MISMATCH options.numeric.treeview_triggers`: original=`0` mushkin=`1
- [ ] M143: options -- MISMATCH options.numeric.underline_hyperlinks`: original=`1` mushkin=`0
- [ ] M144: options -- MISMATCH options.numeric.unpause_on_send`: original=`0` mushkin=`1
- [ ] M145: options -- MISMATCH options.numeric.use_custom_link_colour`: original=`1` mushkin=`0
- [ ] M146: options -- MISMATCH options.numeric.use_msp`: original=`-1` mushkin=`0
- [ ] M147: options -- MISMATCH options.numeric.use_mxp`: original=`0` mushkin=`2
- [ ] M148: options -- MISMATCH options.numeric.write_world_name_to_log`: original=`1` mushkin=`0

### LOW (21)
- [ ] L67: api_availability -- MISMATCH api_availability.BoldColour`: original=`nil` mushkin=`function
- [ ] L68: api_availability -- MISMATCH api_availability.CustomColourBackground`: original=`nil` mushkin=`function
- [ ] L69: api_availability -- MISMATCH api_availability.CustomColourText`: original=`nil` mushkin=`function
- [ ] L70: api_availability -- MISMATCH api_availability.EchoInput`: original=`nil` mushkin=`function
- [ ] L71: api_availability -- MISMATCH api_availability.GetWorldWindowPositionX`: original=`nil` mushkin=`function
- [ ] L72: api_availability -- MISMATCH api_availability.InfoColor`: original=`nil` mushkin=`function
- [ ] L73: api_availability -- MISMATCH api_availability.LogInput`: original=`nil` mushkin=`function
- [ ] L74: api_availability -- MISMATCH api_availability.LogNotes`: original=`nil` mushkin=`function
- [ ] L75: api_availability -- MISMATCH api_availability.LogOutput`: original=`nil` mushkin=`function
- [ ] L76: api_availability -- MISMATCH api_availability.LowercaseWildcard`: original=`nil` mushkin=`function
- [ ] L77: api_availability -- MISMATCH api_availability.MoveWorldWindowX`: original=`nil` mushkin=`function
- [ ] L78: api_availability -- MISMATCH api_availability.NormalColour`: original=`nil` mushkin=`function
- [ ] L79: api_availability -- MISMATCH api_availability.NoteColour`: original=`nil` mushkin=`function
- [ ] L80: api_availability -- MISMATCH api_availability.NoteColourBack`: original=`nil` mushkin=`function
- [ ] L81: api_availability -- MISMATCH api_availability.NoteColourFore`: original=`nil` mushkin=`function
- [ ] L82: api_availability -- MISMATCH api_availability.OmitFromLogFile`: original=`nil` mushkin=`function
- [ ] L83: api_availability -- MISMATCH api_availability.SpeedWalkDelay`: original=`nil` mushkin=`function
- [ ] L84: api_availability -- MISMATCH api_availability.Trace`: original=`nil` mushkin=`function
- [ ] L85: lua_environment -- MISMATCH lua_environment.jit_version`: original=`LuaJIT 2.1.0-beta3` mushkin=`LuaJIT 2.1.1765228720
- [ ] L86: lua_environment -- MISMATCH lua_environment.package_loaders_count`: original=`4` mushkin=`5
- [ ] L87: plugins -- MISMATCH plugins`: original=
