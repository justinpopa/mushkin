# Mushkin Feature Gap Analysis — MUSHclient Parity

**Date:** 2026-02-27
**Method:** Lua API diff (428 original vs 509 Mushkin) + Gemini Pro 3.1 codebase comparison
**Source:** `../mushclient-original` (stripped), `../mushclient_resources`

**Coverage:** 424/428 Lua API functions ported (~99.1%). Remaining: 4 non-API gaps (N1/N2).

---

## G1 — Mapper Subsystem (13 functions)

**Impact:** High — popular plugins (GMCP mapper, speedwalk mapper) depend on this
**Effort:** Large (new subsystem)
**Source:** `mapper.cpp`, `MapCmds.cpp`

- [x] `AddMapperComment` — add comment to current map position
- [x] `AddToMapper` — add a room/exit to the auto-mapper
- [x] `DeleteAllMapItems` — clear the entire map
- [x] `DeleteLastMapItem` — undo last map entry
- [x] `EnableMapping` — toggle auto-mapper on/off
- [x] `GetMapColour` — get color for a map terrain type
- [x] `GetMapping` — check if mapping is enabled
- [x] `GetMappingCount` — number of rooms in map
- [x] `GetMappingItem` — get a specific map room/exit
- [x] `GetMappingString` — get the full map as a string
- [x] `GetRemoveMapReverses` — check if reverse exits are auto-removed
- [x] `MapColour` / `MapColourList` — set/list map terrain colors
- [x] `SetMapping` — programmatically enable/disable mapping
- [x] `SetRemoveMapReverses` — toggle reverse exit removal

**Completed:** 2026-02-28 (commits cd0e135, cc06aad). Verified against original MUSHclient source — all functions match original behavior. `AddToMap` (internal auto-mapping on user movement) not yet implemented.

---

## G2 — Speedwalk System (5 flag constants) — DONE

**Impact:** Medium — speedwalk is a convenience feature, some plugins use it
**Effort:** N/A (already implemented as flag constants)
**Source:** `lua_common.h`, `lua_constants.cpp`
**Note:** These are not standalone functions — they are flag constants (`eAliasSpeedWalk`, `eTimerSpeedWalk`, `eAliasQueue`, `eAliasMenu`, `eTimerNote`) used with `AddAlias`/`AddTimer`. Already defined and exposed as Lua globals.

- [x] `eAliasSpeedWalk` (2048) — alias flag: treat response as speedwalk
- [x] `eTimerSpeedWalk` (64) — timer flag: treat response as speedwalk
- [x] `eAliasMenu` (8192) — alias flag: show in context menu
- [x] `eAliasQueue` (4096) — alias flag: queue response for paced sending
- [x] `eTimerNote` (128) — timer flag: display response as note

---

## G3 — World Management (4 functions) — DONE

**Impact:** Medium — multi-world plugins need these
**Effort:** Medium
**Source:** `world_utilities.cpp`
**Completed:** 2026-02-28

- [x] `GetWorld` — get a reference to another open world by name (current world only for now)
- [x] `GetWorldById` — get a reference to another open world by unique ID (current world only for now)
- [x] `Open` — open a world file programmatically (stub, returns false — needs MainWindow access)
- [x] `Reset` — reset MXP parser state (soft reset via MXP_Off)

---

## G4 — Config Introspection (3 functions) — DONE

**Impact:** Medium — plugins that manage settings need these
**Effort:** Small
**Source:** `methods_info.cpp`
**Completed:** 2026-02-27 (commit 82a1127)

- [x] `GetCurrentValue` — get the current value of any world option by name
- [x] `GetDefaultValue` — get the default value of any world option
- [x] `GetLoadedValue` — get the value as it was when the world was loaded (before runtime changes)

---

## G5 — Text Manipulation (3 functions) — DONE

**Impact:** Medium — output window control
**Effort:** Small–Medium
**Source:** `world_output.cpp`
**Completed:** 2026-02-28

- [x] `DeleteLines` — delete lines from end of output buffer (guards on m_bInSendToScript)
- [x] `DeleteOutput` — clear the output window (resets line list, selection, current line)
- [x] `SetSelection` — set the text selection range in the output window (1-based args)

---

## G6 — Display / UI (4 functions) — DONE

**Impact:** Low–Medium
**Effort:** Small–Medium
**Source:** `world_output.cpp`
**Completed:** 2026-02-28

- [x] `Bookmark` — set/clear bookmark flag on output line
- [x] `SetUnseenLines` — set the "unseen lines" count (activity indicator)
- [x] `ResetStatusTime` — reset the status bar timer to current time
- [x] `Transparency` — set window opacity via Qt (key param accepted for compat, ignored)

---

## G7 — Scripting Helpers (5 functions)

**Impact:** Low–Medium
**Effort:** Small
**Source:** `methods_commands.cpp`, `methods_info.cpp`

- [x] `DoCommand` — execute a MUSHclient internal command (dispatch table)
- [x] `GetInternalCommandsList` — list all available internal commands
- [x] `Help` — show help for a topic (stub, returns eOK)
- [x] `GenerateName` — generate a random fantasy name
- [x] `ReadNamesFile` — load a name generation file (stub, built-in generator needs no file)

---

## G8 — XML/MXP Entities (3 functions)

**Impact:** Low — only relevant for MXP-heavy MUDs
**Effort:** Small
**Source:** `methods_commands.cpp`

- [x] `GetEntity` — get the value of an MXP entity by name
- [x] `GetXMLEntity` — get the value of an XML entity
- [x] `SetEntity` — define a custom MXP entity

---

## G9 — Logging (1 function)

**Impact:** Low
**Effort:** Small
**Source:** `methods_commands.cpp`

- [x] `OmitFromLogFile` — exclude current line from the log file

---

## G10 — Sound (1 function)

**Impact:** Low
**Effort:** Small
**Source:** `methods_commands.cpp`

- [x] `PlaySoundMemory` — play a sound from a memory buffer (stub, returns eCannotPlaySound)

---

## G11 — Text Transforms (2 functions)

**Impact:** Low
**Effort:** Small
**Source:** `methods_commands.cpp`

- [x] `LowercaseWildcard` — lowercase a specific wildcard match
- [x] `TranslateGerman` — translate German character encoding

---

## G12 — Colour (2 functions)

**Impact:** Low
**Effort:** Small
**Source:** `methods_info.cpp`

- [x] `GetCustomColourName` — get the name assigned to a custom color slot
- [x] `MapColourList` — list all map terrain colors (implemented with G1 mapper)

---

## B1 — GetInfo() Case Redefinition Collision (13 cases)

**Impact:** Critical — existing plugins calling GetInfo(294–306) get wrong data or wrong types
**Effort:** Small (code changes only, data already exists)
**Source:** `world_settings.cpp` L_GetInfo, confirmed by diff against `methods_info.cpp`

Cases 294–306 were assigned invented meanings that conflict with the original MUSHclient API. Must be restored to original semantics with cross-platform equivalents.

**Redefined cases (fixed):**
- [x] Case 294: Keyboard modifier/mouse button bitmask via `QGuiApplication::queryKeyboardModifiers()`
- [x] Case 295: Output window redraw count (`m_iOutputWindowRedrawCount`)
- [x] Case 297: Performance counter frequency (returns `1000.0`)
- [x] Case 298: `sqlite3_libversion_number()`
- [x] Case 299: ANSI code page (returns `65001` for UTF-8)
- [x] Case 300: OEM code page (returns `65001` for UTF-8)
- [x] Case 301: Connect time as DATE (epoch seconds from `m_tConnectTime`)
- [x] Case 302: Last log flush time as DATE (`m_LastFlushTime`)
- [x] Case 303: Script file mod time as DATE (`m_timeScriptFileMod`)
- [x] Case 304: Current time as DATE (`QDateTime::currentSecsSinceEpoch()`)
- [x] Case 305: Client start time as DATE (static capture at first call; TODO: true app start)
- [x] Case 306: World start time as DATE (`m_whenWorldStarted`)

**Easy stubs (fixed):**
- [x] Case 61: IP address string via `WorldSocket::peerAddress()`
- [x] Case 67: World file directory via `QFileInfo(m_strWorldFilePath).path()`
- [x] Case 111: World modified flag via `pDoc->isModified()`
- [x] Case 225: MXP custom elements count via `m_mxpEngine->m_customElementMap.size()`
- [x] Case 226: MXP custom entities count via `m_mxpEngine->m_customEntityMap.size()`
- [x] Case 228: IP address as integer via `QHostAddress::toIPv4Address()`

**Remaining stubs (need new infrastructure):**
- [x] Cases 57–58: Default world/log directories via `GlobalOptions` singleton
- [x] Cases 233–234: Trigger/alias regex timing via `std::chrono::high_resolution_clock` accumulators
- [x] Cases 236–237: Input selection start/end via `IInputView::selectionStart()/selectionEnd()`
- [x] Cases 249–250: Main window client height/width via `getToolBarInfo(which=0)`

**Required changes outside L_GetInfo (done):**
- [x] `connection_manager.cpp`: Assign `m_whenWorldStarted` in `onConnect()`
- [x] `world_socket.h/cpp`: Add `peerAddress()` forwarding method

---

## Non-API Gaps (from Gemini analysis)

These aren't Lua API functions but are architectural/feature gaps.

### N1 — SOCKS/HTTP Proxy Support

**Impact:** Medium — users behind firewalls need this
**Effort:** Medium
**Source:** `sockshdr.cpp`

- [x] SOCKS4/5 proxy support in ConnectionManager — via `QNetworkProxy::Socks5Proxy` on per-socket `setProxy()`
- [x] HTTP CONNECT proxy support — via `QNetworkProxy::HttpProxy` on per-socket `setProxy()`

**Completed:** ProxyConfig struct in WorldDocument (type/server/port/username/password), 5 config_options entries for XML serialization, proxy group box in Connection tab UI, legacy Lua API stubs (GetInfo case 62, GetOption case 229) wired to real fields.

### N2 — Chat System (peer-to-peer)

**Impact:** Low–Medium — zChat/MudMaster chat protocol
**Effort:** Large
**Source:** `chat.cpp`

- [ ] Peer-to-peer chat protocol implementation
- [ ] Chat API functions (ChatCall, ChatAccept, ChatMessage, etc.)

### N3 — DoCommand API (Done)

**Impact:** Medium — plugins use `DoCommand("Save")`, `DoCommand("Connect")`, etc.
**Effort:** Medium

- [x] `DoCommand` dispatch table: 252 commands (9 original + 17 direction/action + 83 macro/keypad/alt + ~143 UI commands via MainWindow callback)
- [x] `GetInternalCommandsList` returns all 252 supported command names

### N4 — LuaFileSystem (lfs) (Done)

**Impact:** Medium — many plugins use `lfs.dir()`, `lfs.mkdir()`, etc.
**Effort:** Small (bundle existing C library)

- [x] Bundle `lfs` v1.8.0 library (statically linked in third-party)
- [x] Register in Lua state (`lfs` global + `require("lfs")` support)

### N5 — Logging Settings Infrastructure

**Impact:** Low–Medium — log dialog fields are non-functional without these
**Effort:** Small

- [x] Add `m_nLogLines` (int) to WorldDocument — max output lines to include in log
- [x] Add `m_bAppendToLogFile` (bool) to WorldDocument — append vs overwrite on log open
- [x] Add log format combo wiring (reuses existing `m_bLogHTML`/`m_bLogRaw` flags — Text/HTML/Raw)
- [x] Wire log dialog load/save to these fields (`log_dialog.cpp:79,85,102,108`)
- [x] Wire log format combo in world properties (`world_properties_dialog.cpp:534`)
- [x] Implement `%D` delta time in output preamble (`output_view.cpp:673`) — track previous line timestamp through paint loop

---

## Summary

| Group | Functions | Impact | Effort | Status |
|:---|:---:|:---|:---|:---|
| **B1 GetInfo Collision** | **22 cases** | **Critical** | **Small** | Done (22/22) |
| G1 Mapper | 14 | High | Large | Done (cd0e135, cc06aad) |
| G2 Speedwalk | 5 | Medium | N/A | Done (flag constants, already implemented) |
| G3 World Management | 4 | Medium | Medium | Done (Open stubbed) |
| G4 Config Introspection | 3 | Medium | Small | Done (82a1127) |
| G5 Text Manipulation | 3 | Medium | Small–Med | Done |
| G6 Display/UI | 4 | Low–Med | Small–Med | Done |
| G7 Scripting Helpers | 5 | Low–Med | Small | Done (bfb6b40) |
| G8 XML/MXP Entities | 3 | Low | Small | Done (bfb6b40) |
| G9 Logging | 1 | Low | Small | Done (bfb6b40) |
| G10 Sound | 1 | Low | Small | Done (bfb6b40) |
| G11 Text Transforms | 2 | Low | Small | Done (bfb6b40) |
| G12 Colour | 2 | Low | Small | Done (bfb6b40) |
| N1 Proxy | — | Medium | Medium | Done (QNetworkProxy) |
| N2 Chat | — | Low–Med | Large | Not started |
| N3 DoCommand API | — | Medium | Medium | Done (252 commands) |
| N4 LuaFileSystem | — | Medium | Small | Done |
| N5 Logging Settings | 6 | Low–Med | Small | Done |
| **Total** | **47 + 4** | | | |
