# Mushkin Feature Gap Analysis — MUSHclient Parity

**Date:** 2026-02-27
**Method:** Lua API diff (428 original vs 509 Mushkin) + Gemini Pro 3.1 codebase comparison
**Source:** `../mushclient-original` (stripped), `../mushclient_resources`

**Coverage:** 384/428 Lua API functions ported (~90%). 44 missing + 22 GetInfo case bugs, categorized below.

---

## G1 — Mapper Subsystem (13 functions)

**Impact:** High — popular plugins (GMCP mapper, speedwalk mapper) depend on this
**Effort:** Large (new subsystem)
**Source:** `mapper.cpp`, `MapCmds.cpp`

- [ ] `AddMapperComment` — add comment to current map position
- [ ] `AddToMapper` — add a room/exit to the auto-mapper
- [ ] `DeleteAllMapItems` — clear the entire map
- [ ] `DeleteLastMapItem` — undo last map entry
- [ ] `EnableMapping` — toggle auto-mapper on/off
- [ ] `GetMapColour` — get color for a map terrain type
- [ ] `GetMapping` — check if mapping is enabled
- [ ] `GetMappingCount` — number of rooms in map
- [ ] `GetMappingItem` — get a specific map room/exit
- [ ] `GetMappingString` — get the full map as a string
- [ ] `GetRemoveMapReverses` — check if reverse exits are auto-removed
- [ ] `MapColour` / `MapColourList` — set/list map terrain colors
- [ ] `SetMapping` — programmatically enable/disable mapping
- [ ] `SetRemoveMapReverses` — toggle reverse exit removal

**Note:** Many modern MUD players use external mappers (Mudlet mapper, web-based). Consider whether a built-in mapper is worth the effort vs. exposing GMCP map data to plugins and letting them handle it.

---

## G2 — Speedwalk System (5 functions)

**Impact:** Medium — speedwalk is a convenience feature, some plugins use it
**Effort:** Medium
**Source:** `methods_commands.cpp`, `speedwalk.cpp`

- [ ] `AliasSpeedWalk` — execute a speedwalk alias (e.g., `3n2e4s`)
- [ ] `TimerSpeedWalk` — execute speedwalk with timer delays between steps
- [ ] `AliasMenu` — show a context menu for an alias
- [ ] `AliasQueue` — queue an alias for execution
- [ ] `TimerNote` — attach a note to a timer (display in timer list)

---

## G3 — World Management (4 functions)

**Impact:** Medium — multi-world plugins need these
**Effort:** Medium
**Source:** `methods_info.cpp`, `methods_commands.cpp`

- [ ] `GetWorld` — get a reference to another open world by name
- [ ] `GetWorldById` — get a reference to another open world by unique ID
- [ ] `Open` — open a world file programmatically
- [ ] `Reset` — reset world state (timers, triggers, variables)

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

## G5 — Text Manipulation (3 functions)

**Impact:** Medium — output window control
**Effort:** Small–Medium
**Source:** `methods_commands.cpp`

- [ ] `DeleteLines` — delete specific lines from the output buffer
- [ ] `DeleteOutput` — clear the output window
- [ ] `SetSelection` — set the text selection range in the output window

---

## G6 — Display / UI (4 functions)

**Impact:** Low–Medium
**Effort:** Small–Medium
**Source:** `methods_info.cpp`, `methods_commands.cpp`

- [ ] `Bookmark` — bookmark a line in the output for quick navigation
- [ ] `SetUnseenLines` — set the "unseen lines" count (activity indicator)
- [ ] `ResetStatusTime` — reset the status bar timer display
- [ ] `Transparency` — set window transparency/opacity

---

## G7 — Scripting Helpers (5 functions)

**Impact:** Low–Medium
**Effort:** Small
**Source:** `methods_commands.cpp`, `methods_info.cpp`

- [ ] `DoCommand` — execute a MUSHclient internal command (slash-command)
- [ ] `GetInternalCommandsList` — list all available internal commands
- [ ] `Help` — show help for a topic
- [ ] `GenerateName` — generate a random fantasy name
- [ ] `ReadNamesFile` — load a name generation file

---

## G8 — XML/MXP Entities (3 functions)

**Impact:** Low — only relevant for MXP-heavy MUDs
**Effort:** Small
**Source:** `methods_commands.cpp`

- [ ] `GetEntity` — get the value of an MXP entity by name
- [ ] `GetXMLEntity` — get the value of an XML entity
- [ ] `SetEntity` — define a custom MXP entity

---

## G9 — Logging (1 function)

**Impact:** Low
**Effort:** Small
**Source:** `methods_commands.cpp`

- [ ] `OmitFromLogFile` — exclude current line from the log file

---

## G10 — Sound (1 function)

**Impact:** Low
**Effort:** Small
**Source:** `methods_commands.cpp`

- [ ] `PlaySoundMemory` — play a sound from a memory buffer (not file)

---

## G11 — Text Transforms (2 functions)

**Impact:** Low
**Effort:** Small
**Source:** `methods_commands.cpp`

- [ ] `LowercaseWildcard` — lowercase a specific wildcard match
- [ ] `TranslateGerman` — translate German character encoding

---

## G12 — Colour (2 functions)

**Impact:** Low
**Effort:** Small
**Source:** `methods_info.cpp`

- [ ] `GetCustomColourName` — get the name assigned to a custom color slot
- [ ] `MapColourList` — list all map terrain colors (also in G1)

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
- [ ] Cases 57–58: Default world/log directories (need `GlobalOptions` wiring)
- [ ] Cases 233–234: Trigger/alias regex timing (needs perf counter in regexp matching)
- [ ] Cases 236–237: Input selection start/end column (needs `QLineEdit` hook)
- [ ] Cases 249–250: Main window client height/width (needs `MainWindow` reference)

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

- [ ] SOCKS4/5 proxy support in ConnectionManager
- [ ] HTTP CONNECT proxy support

### N2 — Chat System (peer-to-peer)

**Impact:** Low–Medium — zChat/MudMaster chat protocol
**Effort:** Large
**Source:** `chat.cpp`

- [ ] Peer-to-peer chat protocol implementation
- [ ] Chat API functions (ChatCall, ChatAccept, ChatMessage, etc.)

### N3 — Internal Slash-Commands

**Impact:** Medium — power users expect `/help`, `/save`, `/quit`, etc.
**Effort:** Medium

- [ ] Command parser for `/` prefix commands
- [ ] `/help`, `/save`, `/quit`, `/reload`, etc.

### N4 — LuaFileSystem (lfs)

**Impact:** Medium — many plugins use `lfs.dir()`, `lfs.mkdir()`, etc.
**Effort:** Small (bundle existing C library)

- [ ] Bundle `lfs` library and register in Lua state

---

## Summary

| Group | Functions | Impact | Effort | Status |
|:---|:---:|:---|:---|:---|
| **B1 GetInfo Collision** | **22 cases** | **Critical** | **Small** | 20/22 done (4 stubs remain) |
| G1 Mapper | 14 | High | Large | Not started |
| G2 Speedwalk | 5 | Medium | Medium | Not started |
| G3 World Management | 4 | Medium | Medium | Not started |
| G4 Config Introspection | 3 | Medium | Small | Done (82a1127) |
| G5 Text Manipulation | 3 | Medium | Small–Med | Not started |
| G6 Display/UI | 4 | Low–Med | Small–Med | Not started |
| G7 Scripting Helpers | 5 | Low–Med | Small | Not started |
| G8 XML/MXP Entities | 3 | Low | Small | Not started |
| G9 Logging | 1 | Low | Small | Not started |
| G10 Sound | 1 | Low | Small | Not started |
| G11 Text Transforms | 2 | Low | Small | Not started |
| G12 Colour | 2 | Low | Small | Not started |
| N1 Proxy | — | Medium | Medium | Not started |
| N2 Chat | — | Low–Med | Large | Not started |
| N3 Slash-Commands | — | Medium | Medium | Not started |
| N4 LuaFileSystem | — | Medium | Small | Not started |
| **Total** | **47 + 4 + B1** | | | |
