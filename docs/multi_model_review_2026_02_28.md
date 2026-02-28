# Multi-Model Codebase Review — 2026-02-28

**Models used:** Gemini 3.1 Pro (3 focused analyses via MCP), Grok 4, Qwen 3.5 (397B), Kimi K2.5, DeepSeek V3.2
**Scope:** Full mushkin codebase (src/, 100K LOC) + original MUSHclient comparison (reference/, 176K LOC)

---

## Executive Summary

The mushkin codebase is in strong shape. The C++26 migration is substantially complete, architecture is well-decomposed from the original MFC monolith, and ~91% of the Lua API surface is ported. However, this review identified **2 critical bugs**, **5 high-severity issues**, and several medium/low findings that should be addressed. Additionally, the feature gap analysis (`docs/feature_gaps.md`) is missing several items — most notably **GMCP protocol support** (critical for modern MUDs).

---

## P0 — Critical (Fix Immediately)

### ~~C1. Use-After-Free: Self-Deleting Triggers/Aliases/Timers~~ FIXED
**Consensus:** Gemini (safety), Kimi K2.5
**Files:** `src/world/lua_api/world_timers.cpp:256`, `src/world/world_alias_execution.cpp:92`

**Investigation:** `AutomationRegistry::deleteTrigger/deleteAlias/deleteTimer` already check `executing_script` and reject deletion. World-level `L_DeleteTrigger`/`L_DeleteAlias` route through the registry and are safe. Two remaining paths were vulnerable:
- **C1a:** `L_DeleteTimer` plugin path bypassed registry — no `executing_script` check (UAF)
- **C1b:** `executeAlias()` one-shot deletion accessed freed alias in subsequent debug log (UAF)

**Fixed:** Added `executing_script` guard to plugin timer path; moved debug log before one-shot deletion.

### ~~C2. Buffer Overflow in UTF-8 State Machine~~ FIXED
**Consensus:** Gemini (protocol), Grok 4
**File:** `src/world/telnet_parser.cpp:587-603` (`Phase_UTF8`)

The UTF-8 continuation byte handler used an unbounded `while` loop to find the first null byte in `m_UTF8Sequence` (`std::array<quint8, 8>`) and wrote to indices `i` and `i+1`. If the buffer was full, this wrote out of bounds. Bug was inherited from the original MUSHclient (where the buffer was only 5 bytes).

**Fixed:** Added upper bound (`size()-2`) to the `while` loop; bails with `OutputBadUTF8characters()` if buffer is full.

---

## P1 — High (Fix Before Release)

### H1. Reentrancy via `QApplication::processEvents()` in Lua Progress Dialog
**Source:** Gemini (safety)
**File:** `src/world/lua_api/lua_progress.cpp:43,65,85`

Calling `processEvents()` inside a Lua C-function allows the Qt event loop to run mid-script. This can cause nested trigger firing, network data processing by a different script, or deletion of the `WorldDocument` while a script is on the Lua stack.

**Fix:** Remove `processEvents()`. Use `DoAfter` chunking or worker threads for long-running scripts.

### H2. Non-Thread-Safe `setlocale()` in lrexlib
**Source:** Gemini (safety)
**File:** `src/world/lua_api/lrexlib.cpp:92,98`

`setlocale(LC_CTYPE, locale)` modifies locale for the entire process. In multi-threaded Qt, this causes data races corrupting string formatting and regex behavior.

**Fix:** Use `newlocale`/`uselocale` (POSIX) for thread-local locale changes.

### H3. Wildcard Replacement Recursive Expansion
**Source:** Gemini (architecture)
**File:** `src/world/world_trigger_execution.cpp:48`

`replaceWildcards` replaces placeholders sequentially from `%99` down to `%0`. If `%1` expands to text containing `%0`, the subsequent iteration for `i=0` re-expands the injected `%0`, corrupting output.

**Fix:** Use single-pass regex/token-based replacement.

### H4. Unbounded Memory Allocation (DoS)
**Source:** Gemini (protocol)
**Files:** `src/world/telnet_parser.cpp:498` (`m_IAC_subnegotiation_data`), `src/world/mxp_engine.cpp:468` (`m_strMXPstring`)

Multiple protocol buffers append incoming bytes without upper bounds. A malicious server can send an infinite subnegotiation or MXP tag (never sending `IAC SE` or `>`), consuming all system RAM.

**Fix:** Enforce reasonable maximum sizes (e.g., 64KB) on all protocol collection buffers.

### H5. GMCP Protocol Not Implemented (Feature Gap)
**Source:** Gemini (features)
**Impact:** Critical for modern MUDs — GMCP is the primary structured data protocol.

`TELOPT_GMCP_OPT` is defined in headers, and `register_gmcp_functions` exists in `lua_registration.h`, but there is no implementation. `telnet_parser.cpp` has no GMCP handler. `world_gmcp.cpp` does not exist. The Lua functions `GetGMCP`, `SendGMCP`, `GetGMCPList` are not functional.

**Fix:** Implement GMCP handler in telnet parser + create `world_gmcp.cpp` with Lua API functions.

---

## P2 — Medium

### M1. Raw `new` for QProgressDialog
**Source:** Gemini (safety)
**File:** `src/world/lua_api/lua_progress.cpp:125`

`QProgressDialog` allocated with `new` without Qt parent, stored as raw pointer in Lua userdata. Violates "every raw `new` is a bug" mandate. Leaks if Lua state is closed prematurely.

**Fix:** Use `std::unique_ptr` in userdata or pass `pDoc` as parent.

### M2. `malloc` Leak on Lua `longjmp` in lrexlib
**Source:** Gemini (safety)
**File:** `src/world/lua_api/lrexlib.cpp:22,118`

PCRE match vector allocated with `malloc`. If `lua_error` triggers `longjmp`, the C stack is unwound without freeing the buffer.

**Fix:** Use `lua_newuserdata` to let Lua GC manage the buffer.

### M3. Path Traversal in Sound Handling
**Source:** Gemini (protocol), Kimi K2.5
**File:** `src/world/sound_manager.cpp:551` (`resolveFilePath`)

MXP `<sound>` and MSP `!!SOUND()` accept arbitrary paths. No sanitization or jail checking. A server could probe local file existence or exploit audio decoder vulnerabilities.

**Fix:** Sanitize with `QDir::cleanPath` and ensure paths stay within the sound directory.

### M4. Path Traversal in Plugin State Files
**Source:** Kimi K2.5
**File:** `src/automation/plugin.cpp:623,730`

Plugin ID (`m_strID`) is concatenated into file paths without sanitization. A malicious plugin ID like `../../../etc/passwd` could escape the state directory.

**Fix:** Validate plugin IDs contain only safe characters.

### M5. SQL Injection in Database Helper
**Source:** Gemini (safety)
**File:** `src/storage/database.cpp:221`

Table name concatenated directly into `PRAGMA table_info` query. Currently only used with hardcoded strings, but a liability if the function is ever exposed.

**Fix:** Validate table names as alphanumeric + underscores only.

### M6. Integer Overflow in ANSI Parser
**Source:** Gemini (protocol)
**File:** `src/world/telnet_parser.cpp:103`

ANSI code parameters accumulated via `m_code *= 10; m_code += c - '0'` without bounds checking. Extremely long numeric sequences cause signed integer overflow (UB).

**Fix:** Clamp `m_code` to a reasonable maximum (e.g., 65535) before accumulating.

### M7. Regex Denial of Service
**Source:** Kimi K2.5
**File:** `src/world/lua_api/world_triggers.cpp:1491`

User-provided regex patterns compiled without complexity limits. Catastrophic backtracking possible with patterns like `(a+)+$`.

**Fix:** Set PCRE match/backtrack limits via `pcre_extra`.

### M8. MXP Tag Stack Explosion
**Source:** Gemini (protocol)
**File:** `src/world/mxp_engine.cpp:707`

Active tag list grows without limit. Server can flood with millions of unclosed tags.

**Fix:** Enforce maximum tag depth (e.g., 1000) and reject/close excess tags.

---

## P3 — Low

### L1. Unicode Loss in MXP Entities
**File:** `src/world/mxp_engine.cpp:848` — `toLatin1()` corrupts Unicode in entity replacements. Use `toUtf8()`.

### L2. Trigger Coloring Colors Entire Line
**File:** `src/world/world_trigger_execution.cpp:76-99` — Known limitation; should split `Style` runs to highlight only matched text.

### L3. Raw View Pointers (`m_pActiveInputView`, `m_pActiveOutputView`)
**File:** `src/world/world_document.h:821-822` — Non-owning raw pointers to views risk dangling if accessed after parent destruction. (Grok 4, Qwen 3.5)

### L4. Manual `m_CurrentPlugin` State Management
**File:** `src/automation/plugin.cpp:115,385` — Use `qScopeGuard` for exception-safe restoration.

### L5. `extern QRgb xterm_256_colours[256]` Should Be `std::array`
**File:** `src/world/world_document.h:75` — (Qwen 3.5)

### L6. Locale Buffer Overflow
**File:** `src/world/lua_api/lrexlib.cpp:94` — Fixed 256-byte `old_locale` buffer. Use `std::string`.

### L7. UTF-8/UTF-16 Sync Fragility in Selection
**File:** `src/ui/output_view.cpp` (`drawLine`) — Manual byte position tracking between UTF-8 and UTF-16 could desync on malformed input.

---

## Code Quality (Qwen 3.5)

### Encapsulation
- `world_document.h` exposes **700+ lines** of public member variables (lines 377-1100). Should use accessors.
- `to_underlying()` utility at line 110 duplicates C++23 `std::to_underlying()`.

### Registration Boilerplate
- `lua_registration.cpp` has **436 lines** of `extern` forward declarations (28-464) and **271 lines** of repetitive `lua_pushcfunction`/`lua_setglobal` calls (999-1270). Should be data-driven.

### Missing Const
- `replaceWildcards()`, `changeLineColors()`, `executeTrigger()` should be `const` methods.

### Modern C++ Gaps
- `std::print`/`std::format` not adopted (uses `QString::arg()` throughout).
- `std::meta` reflection not used for `config_options.cpp` tables (700+ manual getter/setter macros).
- No ranges/views adoption yet.

---

## Feature Gaps NOT in `docs/feature_gaps.md`

### Protocol Gaps
| Protocol | Status | Notes |
|:---|:---|:---|
| **GMCP** | **Missing** | Header defined, no implementation. Critical for modern MUDs. |
| **MSDP** | **Missing** | Completely absent. Popular alternative to GMCP. |
| **MSSP** | **Missing** | Server status protocol. |
| **MCCP3** | **Missing** | Only MCCP 1/2 supported. |
| MXP | Implemented | Entity Unicode bug (L1 above) |
| ATCP | Implemented | |
| MCCP 1/2 | Implemented | |
| NAWS | Implemented | |
| CHARSET | Partial | UTF-8/US-ASCII only; original supports more codepages |

### Missing Lua API Functions (Not in Gap Doc)
- `GetGMCP(Variable)` — get GMCP variable value
- `SendGMCP(Message)` — send GMCP message to server
- `GetGMCPList()` — list known GMCP modules
- `AddSpellCheckWord(Word, Action, Replacement)` — spell checker dictionary
- `WindowLoadImageMemory(Win, Id, Buffer)` — load miniwindow image from memory

### Missing Non-API Features
| Feature | Description |
|:---|:---|
| **Spell Checker** | Integrated spell checking (stubbed to return empty) |
| **Print Support** | Print output buffer/notes (no Qt printing infrastructure) |
| **Plugin Wizard** | UI for generating plugin XML skeletons |
| **Integrated Script Editor** | Syntax-highlighting editor; currently uses Notepad or external |
| **Drag-and-Drop** | Drop `.mcl`/`.xml` files to open/install |
| **Global Plugins** | Auto-loading plugins from app directory for all worlds |
| **UDP Support** | `UdpSend`/`UdpListen` stubbed ("UDP support removed") |
| **COM/OLE (luacom)** | Mocked as stub table; breaks Windows plugins using COM |
| **Advanced Toolbars** | Macros, Plugins, Directions, Activity toolbars |
| **Status Bar Detail** | MCCP compression ratio, MXP status, logging lamp |
| **Context Menus** | Copy as HTML, Browse URL, Filter lines |
| **~30 Plugin Callbacks** | `OnPluginTelnetRequest`, `OnPluginDrawOutputWindow`, etc. |

### Behavioral Fidelity Issues
- `sendToMud()` converts line endings to `\n` regardless of protocol (original negotiated CRLF)
- `GetSysColor`/`GetSystemMetrics` imperfectly maps Windows constants to Qt
- `GetFrame()` returns `WId` (NSView pointer), not `HWND` — FFI plugins will break
- NumLock/CapsLock/ScrollLock toggle state always returns 0 (Qt limitation)
- `GetInfo(294)` cannot distinguish L/R modifier keys

---

## Summary by Severity

| Severity | Count | Status |
|:---|:---:|:---|
| **P0 Critical** | ~~2~~ 0 | ~~UAF in self-deleting triggers, UTF-8 buffer overflow~~ All fixed |
| **P1 High** | 5 | Reentrancy, thread-unsafe locale, wildcard expansion, DoS, GMCP missing |
| **P2 Medium** | 8 | Path traversal (×2), SQL injection, regex DoS, raw new, malloc leak, ANSI overflow, MXP stack |
| **P3 Low** | 7 | Unicode loss, coloring, raw pointers, scope guard, std::array, locale buffer, UTF-16 sync |
| **Feature Gaps** | 15+ | GMCP/MSDP/MSSP protocols, spell checker, print, plugin wizard, global plugins, etc. |
