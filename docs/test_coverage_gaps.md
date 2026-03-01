# Test Coverage Gap Analysis

**Date:** 2026-03-01
**Method:** Cross-reference of all `src/` files (>200 lines) against `tests/` test files. Counts are direct GTest `TEST`/`TEST_F` only.
**Current baseline:** 1003 tests across 54 executables.

## Critical — Zero Tests, High Risk

| Source File | Lines | Module | Rationale |
|:---|---:|:---|:---|
| `ui/main_window.cpp` | 5461 | ui | Entire app shell, menu handling, world lifecycle dispatch |
| `world/lua_api/world_output.cpp` | 1767 | world | **51 tests added** (Tell, ANSI, AnsiNote, Simulate, GetLineInfo, GetStyleInfo, NoteColour, NoteStyle, NoteHr, entities, DeleteLines/Output, Bookmark, info bar, SetSelection, Hyperlink) — moved to Well-Covered |
| `world/world_serialization.cpp` | 1427 | world | **20 tests added** (timer at-time, variables, ExportXML/ImportXML, accelerator/macro/keypad, temporary items, CDATA, option snapshots) — moved to Well-Covered |
| `world/lua_api/lua_registration.cpp` | 1429 | world | **19 tests added** (completeness count, spot checks, global aliases, constant tables, modules, getLuaFunctionNames) — moved to Well-Covered |

## High — Zero or Near-Zero Tests

| Source File | Lines | Module | Existing Tests | Gap |
|:---|---:|:---|:---|:---|
| `ui/views/output_view.cpp` | 2057 | ui | 0 | Rendering, hit-testing, mouse selection, URL clicks |
| `ui/dialogs/global_preferences_dialog.cpp` | 1540 | ui | 0 | World property marshalling |
| `ui/dialogs/plugin_wizard.cpp` | 1206 | ui | 0 | Plugin XML generation, `savePluginXml` std::expected path |
| `ui/views/output_view_miniwindows.cpp` | 1039 | ui | 0 | Miniwindow overlay compositing |
| `world/world_protocol.cpp` | 880 | world | **49 tests added** (ProcessIncomingByte dispatch, ANSI style/cancel/color codes, 256-color, truecolor, GetStyleRGB, RememberStyle, xterm palette, SendPacket, UTF-8, MXP phase transitions) — moved to Well-Covered |
| `world/world_document_plugins.cpp` | 1084 | world | 33 (partial) | Plugin install/load/hot-reload path; file loading not tested |
| `network/ssh_server_session.cpp` | 458 | network | 0 | SSH channel handling, exec/shell dispatch — new feature |
| `network/remote_access_server.cpp` | 320 | network | 0 | SSH server listen/accept loop |
| `network/ansi_formatter.cpp` | 249 | network | **39 tests added** — moved to Well-Covered |

## Medium — Thin Coverage or Zero Tests

| Source File | Lines | Module | Existing Tests | Gap |
|:---|---:|:---|:---|:---|
| `world/script_engine.cpp` | 1518 | world | 38 (partial) | Lua VM lifecycle partially tested; callback dispatch and error recovery not tested |
| `world/lua_api/world_settings.cpp` | 2327 | world | 24 (partial) | GetOption/SetOption paths partially tested; complex validation untested |
| `world/lua_api/world_misc.cpp` | 2583 | world | 17 (partial) | Bucket file for ~40 misc Lua functions; thin coverage |
| `world/lua_api/world_timers.cpp` | 1488 | world | 24 (partial) | API-level coverage; internal firing logic has 0 direct tests |
| `world/lua_api/world_miniwindow_images.cpp` | 1463 | world | ~10 est. | Image load, blit, blend operations barely touched |
| `world/lua_api/world_triggers.cpp` | 1377 | world | 13 (partial) | Add/Delete/Enable tested; complex multi-line and colour triggers not |
| `ui/views/world_widget.cpp` | 656 | ui | 0 | `loadFromFile`/`saveToFile` std::expected paths untested |
| `world/sound_manager.cpp` | 590 | world | 9 | 9 tests vs 590 lines; only API surface tested |
| `world/accelerator_manager.cpp` | 541 | world | 3 (indirect) | Accelerator/macro/keypad round-trip tested via world serialization suite |
| `world/automation_registry.cpp` | 844 | world | 14 (indirect) | No direct unit test for the registry class itself |
| `world/world_trigger_execution.cpp` | 443 | world | 5 | Very thin — only basic match-then-execute path |
| `world/lua_api/world_mapper.cpp` | 454 | world | 0 | All mapper Lua API functions, zero direct tests |
| `world/script_engine_callbacks.cpp` | 319 | world | 0 | Callback dispatch into ScriptEngine |
| `world/lua_dialog_callbacks.cpp` | 251 | world | 0 | Lua dialog bridge |
| `world/macro_keypad_compat.cpp` | 254 | world | 2 (indirect) | Macro/keypad round-trip tested via world serialization suite |
| `world/world_sendto.cpp` | 226 | world | 6 | eSendToLogFile path not verified |
| `world/world_recall.cpp` | 137 | world | 0 | Recall buffer management |
| `storage/global_options.cpp` | 297 | storage | 0 (indirect) | No direct load/save test |
| `ui/main_window_commands.cpp` | 291 | ui | 0 | DoCommand dispatch layer |
| `ui/lua_dialog_registration.cpp` | 224 | ui | 0 | Lua dialog bridge registration |

## Low — Small Files or Low Risk

| Source File | Lines | Module | Notes |
|:---|---:|:---|:---|
| `utils/name_generator.cpp` | ~50 | utils | Pure utility, low risk |
| `utils/app_paths.cpp` | 32 | utils | Thin wrapper |
| `utils/logging.cpp` | 27 | utils | QLoggingCategory wrapper |
| `network/ssh_host_key_manager.cpp` | 143 | network | 3 tests (thin but covers keygen/persist) |

## Well-Covered (Reference)

| Source File | Lines | Test File | Test Count |
|:---|---:|:---|---:|
| `world/mxp_engine.cpp` | 2338 | `test_mxp_gtest.cpp` | 70 |
| `world/telnet_parser.cpp` | 1205 | `test_telnet_parser_gtest.cpp` | 33 |
| `world/lua_api/world_database.cpp` | 1055 | `test_database_gtest.cpp`, `test_lua_database_gtest.cpp` | 40 |
| `world/lua_api/world_notepad.cpp` | 421 | `test_notepad_api_gtest.cpp` | 30 |
| `world/lua_api/world_commands.cpp` | 692 | `test_command_execution_gtest.cpp`, `test_docommand_gtest.cpp` | 27 |
| `world/speedwalk_engine.cpp` | 485 | `test_speedwalk_gtest.cpp` | 20 |
| `utils/url_linkifier.cpp` | 143 | `test_url_detection_gtest.cpp` | 13 |
| `world/world_serialization.cpp` | 1427 | `test_world_serialization_gtest.cpp` + 3 existing | 20 + 21 |
| `world/xml_serialization.cpp` | 1103 | `test_xml_serialization_gtest.cpp`, `test_xml_roundtrip_gtest.cpp`, `test_world_serialization_gtest.cpp` | 35 |
| `network/ansi_formatter.cpp` | 249 | `test_ansi_formatter_gtest.cpp` | 39 |
| `world/lua_api/lua_registration.cpp` | 1429 | `test_lua_registration_gtest.cpp` | 23 |
| `world/lua_api/world_output.cpp` | 1767 | `test_world_output_gtest.cpp` | 51 |
| `world/world_protocol.cpp` | 880 | `test_world_protocol_gtest.cpp` | 49 |

## Recommended Next Targets

Prioritized by testability (no UI needed) and risk:

1. ~~**`world_serialization.cpp`** — DONE. 20 tests added covering timer at-time, variables, ExportXML/ImportXML, accelerator/macro/keypad, temporary items, CDATA, option snapshots.~~
2. ~~**`ansi_formatter.cpp`** — DONE. 39 tests added covering null/empty lines, text attributes (bold/underline/italic/inverse/strikeout), ANSI color indices (standard/bright/256), RGB truecolor, custom palette colors, multi-style lines, state caching, and edge cases.~~
3. ~~**`lua_registration.cpp`** — DONE. 23 tests covering world table completeness (428 functions), 90+ spot checks, 35 global aliases, 9 constant tables, utils/rex modules, getLuaFunctionNames helper, and British/American spelling aliases. Duplicate registrations (Trim, SetStatus) cleaned up.~~
4. ~~**`world_output.cpp`** — DONE. 51 tests covering Tell, ANSI, AnsiNote, Simulate, GetLineInfo, GetStyleInfo, NoteColour getters/setters, NoteStyle, NoteHr, entity functions, DeleteLines/DeleteOutput, Bookmark, info bar (Info/InfoClear/InfoColour/InfoBackground/InfoFont/ShowInfoBar), SetSelection, SetUnseenLines, ResetStatusTime, Hyperlink. Also fixed NoteHr bug: HORIZ_RULE flag was overwritten by StartNewLine's iFlags parameter.~~
5. ~~**`world_protocol.cpp`** — DONE. 49 tests covering ProcessIncomingByte dispatch (text, newline, CR, tab, ESC, IAC, MXP, UTF-8), InterpretANSIcode (reset, bold, underline, blink, inverse, strikeout, cancel codes, foreground/background colors), Interpret256ANSIcode (256-color foreground/background, 24-bit truecolor), GetStyleRGB (ANSI/custom/RGB mode resolution, HILITE bold table, nullptr), RememberStyle, xterm 256-color palette (7 spot checks), SendPacket null guard.~~
