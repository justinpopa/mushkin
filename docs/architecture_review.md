# Mushkin Architecture Review — Multi-Model Consensus (Post-Migration)

**Date:** 2026-02-27
**Reviewers:** Gemini Pro 3.1, Claude Sonnet 4.6, Qwen 3.5 397B, MiniMax m2.5, GPT o4-mini (via Codex CLI), Kimi K2.5, Claude Opus 4.6 (orchestrator)

**Context:** All P0–P4 issues from the first architecture review have been resolved. This is a fresh review of the codebase in its current post-migration state.

## Priority Issues

Issues ranked by cross-model agreement. Agreement count = how many of the 6 reviewing models independently flagged the issue.

---

### P0 — Timer int type → enum class TimerType

**Agreement:** 5/6 (Gemini, Sonnet, Qwen, MiniMax, Kimi)
**Risk:** `Timer::type` is `int` but `TimerType` enum exists. Any integer can be stored without diagnostic; deserialized garbage silently accepted.

**Targets:**
- [x] `src/automation/timer.h:28` — change `int type` to `TimerType type`
- [x] Convert `TimerType` inner enum to `enum class TimerType : int` (Interval = 0, AtTime = 1)
- [x] Update all assignment sites to use enum values (15 files, ~60 replacements)

**Acceptance:** `Timer::type` is `TimerType`. Build + test pass.

---

### P1 — MXP maps use raw `new`/`qDeleteAll` (not C-interop)

**Agreement:** 3/6 (Gemini, Sonnet, MiniMax)
**Risk:** Leak on any exception/early-return path. Manual `delete` in `MXP_CloseTag`. Violates RAII mandate.

The migration notes mark this as "acceptable — C library interop pattern," but `AtomicElement`, `CustomElement`, `MXPEntity`, and `ActiveTag` are all plain C++ structs with no C-linkage requirement.

**Targets:**
- [x] `src/world/mxp_engine.h` — replace `QMap<QString, T*>` with `std::map<QString, std::unique_ptr<T>>` for all MXP maps
- [x] `src/world/mxp_engine.h` — replace `QList<ActiveTag*>` with `std::vector<std::unique_ptr<ActiveTag>>`
- [x] Remove `qDeleteAll` calls and manual `delete` in `MXP_CloseTag`, `CleanupMXP`

**Acceptance:** Zero raw `new`/`delete` in MXP code. Build + test pass.

---

### P2a — Unscoped enums → enum class

**Agreement:** 3–5/6 per enum (Gemini, Sonnet, Qwen, MiniMax, Kimi flagged various subsets)
**Risk:** Namespace pollution (e.g., `NONE` from `Phase`), implicit int conversion, invalid values accepted silently.

**Targets:**
- [x] `src/world/telnet_parser.h` — `enum Phase` → `enum class Phase` (3/6: Gemini, Sonnet, MiniMax)
- [x] `src/world/world_document.h` — `enum ActionSource` → `enum class` (3/6: Sonnet, Qwen, Kimi)
- [x] `src/world/world_document.h` — `enum HistoryStatus` → `enum class` (3/6: Sonnet, Qwen, Kimi)
- [x] `src/world/world_document.h` — `enum ScriptReloadOption` → `enum class` (2/6: Sonnet, Qwen)
- [x] `src/world/world_document.h` — anonymous `enum { eNoAutoConnect... }` → `enum class AutoConnect` (3/6: Sonnet, Qwen, MiniMax)
- [x] `src/world/world_document.h` — anonymous `enum { eMXP_Off... }` → `enum class MXPMode` (3/6: Sonnet, Qwen, MiniMax)

**Acceptance:** Zero unscoped enums in world_document.h and telnet_parser.h. Build + test pass.

---

### P2b — Weak typing: `send_to`, `match_type`, `colour_change_type`

**Agreement:** 2–3/6 (Gemini, Sonnet, Qwen)
**Risk:** `quint16` fields that hold enum values accept any integer. `send_to` exists in Trigger, Alias, AND Timer with no type constraint.

**Targets:**
- [x] `src/automation/trigger.h`, `alias.h`, `timer.h` — `quint16 send_to` → `SendTo` enum class
- [x] `src/automation/trigger.h` — `match_type` stays `quint16` by design: it's a compound bitmask (color indices in bits 4-11, style flags in bits 12-15, color presence in bits 7/11). Not suitable for `enum class`. Documented in trigger.h:36-53. `TRIGGER_MATCH_*` constants consolidated as `inline constexpr` in trigger.h.
- [x] `src/automation/trigger.h` — `TRIGGER_COLOUR_CHANGE_*` constants → `enum class ColourChangeType`

**Acceptance:** All automation "type" fields use proper enum class. Build + test pass.

---

### P2c — NotepadWidget list / IInputView / IOutputView raw pointers

**Agreement:** 3/6 each (Gemini+Qwen+MiniMax for notepad, Qwen+MiniMax+Kimi for views)
**Risk:** `QList<NotepadWidget*>` can hold dangling pointers if widgets self-delete (WA_DeleteOnClose). `IInputView*` / `IOutputView*` have no lifetime tracking.

**Targets:**
- [x] `src/world/world_document.h` — `QList<NotepadWidget*>` → `QList<QPointer<NotepadWidget>>`
- [x] `src/world/world_document.h` — `IInputView*` / `IOutputView*` → documented non-owning contract (not QObject-derived, QPointer N/A)

**Acceptance:** No undocumented raw non-owning pointers for UI objects. Build + test pass.

---

### P2d — LuaDatabase holds raw `sqlite3*` without RAII

**Agreement:** 3/6 (Gemini, Qwen, Kimi)
**Risk:** Default copy/move can double-free. Destructor handles cleanup but no move-safety.

**Targets:**
- [x] `src/world/world_document.h` — delete copy/move on `LuaDatabase` (owns sqlite3* handle)

**Acceptance:** `LuaDatabase` is non-copyable/non-movable or uses RAII handle. Build + test pass.

---

### P2e — zlib stream RAII gap in TelnetParser

**Agreement:** 3/6 (Gemini, Qwen, MiniMax)
**Risk:** `initZlib()`/`cleanupZlib()` manually called. If parser partially destroyed, `inflateEnd` may not be called.

**Targets:**
- [x] `src/world/telnet_parser.h` — wrap `z_stream` in RAII `ZlibStream` guard struct with constructor/destructor

**Acceptance:** zlib cleanup guaranteed by RAII. Build + test pass.

---

### P2f — Subsystem coupling via back-reference

**Agreement:** 4/6 (Gemini, Qwen, MiniMax, Kimi)
**Risk:** All 5 extracted subsystems hold `WorldDocument&` back-references and access public members directly. Makes isolation testing impossible.

**Targets:**
- [x] Define interface (e.g., `IWorldContext`) that exposes only necessary services — `src/world/world_context.h` with 12 pure virtual methods (plugin dispatch, automation execution, state queries, logging, output, notepad)
- [x] Inject into subsystems instead of full `WorldDocument&` — Phase 1 complete: AutomationRegistry, SoundManager, NotepadWidget now take `IWorldContext&`/`IWorldContext*`.

**Phase 2 — deferred (not planned):** The remaining 4 subsystems each have 60-80+ direct accesses to WorldDocument fields and methods, making interface extraction impractical without essentially duplicating the entire WorldDocument public API:

| Subsystem | Accesses | Primary dependencies |
|:---|:---:|:---|
| ScriptEngine | 80+ | Lua callbacks, plugin dispatch, output, error reporting |
| ConnectionManager | 60+ | Socket state, UTF-8 parsing, line buffer, remote server config |
| TelnetParser | 60+ | ANSI codes, MXP engine, protocol flags, output pipeline |
| MXPEngine | 60+ | Style flags, colors, line output, sound, tag parsing |

These subsystems are companion objects by design — tightly coupled internal organs of WorldDocument. Further decoupling would require god-class decomposition (P4a) first to reduce the surface area they depend on.

**Acceptance:** Subsystems depend on abstract interface, not concrete WorldDocument. Aspirational — may require multiple passes.

---

### P3a — SoundManager `m_audioEngine` raw owning pointer

**Agreement:** 2/6 (Gemini, Sonnet)
**Risk:** Manual `new`/`delete` in `initialize()`/`cleanup()`. Two `delete` sites.

**Targets:**
- [x] `src/world/sound_manager.h` — `QAudioEngine*` → `std::unique_ptr<QAudioEngine>`

**Acceptance:** No manual `delete` in SoundManager. Build + test pass.

---

### P3b — `m_timerCheckTimer` / `m_acceleratorManager` / `m_scriptFileWatcher` raw pointers

**Agreement:** 3/6 for timerCheckTimer (Sonnet, MiniMax, Kimi), 2/6 for acceleratorManager (Qwen, MiniMax), 1/6 for scriptFileWatcher (Sonnet — found double-delete risk)

**Risk:** Qt parent-child ownership but manual `delete` in destructor risks double-free. `m_scriptFileWatcher` has two separate `delete` sites.

**Targets:**
- [x] `src/world/world_document.h` — `QTimer* m_timerCheckTimer` → removed manual `delete` (Qt parent-child)
- [x] `src/world/world_document.h` — `AcceleratorManager*` → documented as Qt parent-child
- [x] `src/world/world_document.h` — `QFileSystemWatcher* m_scriptFileWatcher` → fixed double-free (removed Qt parent, sole manual ownership)

**Acceptance:** No double-delete risk. Build + test pass.

---

### P3c — SoundManager async lambda use-after-free (GPT-exclusive finding)

**Agreement:** 1/6 (GPT — deepest analysis, unique find)
**Risk:** MSP download lambdas in `sound_manager.cpp:481-525` capture `this` (SoundManager). During `~WorldDocument`, SoundManager is cleaned up before QNetworkReply objects (parented to document). If a download completes after cleanup, the lambda fires on a freed SoundManager → UAF.

**Targets:**
- [x] `src/world/sound_manager.cpp` — added `shared_ptr<bool>` alive guard in async lambdas (UAF fix)

**Acceptance:** No UAF risk from async network callbacks during destruction. Build + test pass.

---

### P3d — Timer offset calculation bug (GPT-exclusive finding)

**Agreement:** 1/6 (GPT — unique, potentially a real bug)
**Risk:** `automation_registry.cpp:654-660` calculates `fire_time = now + (intervalSecs - offsetSecs)`. The offset is supposed to align firings to a phase within the interval, but subtraction shortens the interval. If offset > interval, the duration goes negative and the timer retriggers every tick.

Additionally, fractional seconds (`every_second`, `offset_second` are `double`) are truncated to `qint64`, so sub-second timers degenerate to 1 Hz.

**Targets:**
- [x] `src/world/automation_registry.cpp` — clamped offset >= interval to prevent busy-loop; preserved interval-offset formula
- [x] `src/world/automation_registry.cpp` — all timer arithmetic now in milliseconds (sub-second timers work correctly)

**Acceptance:** Timer offsets align to phase. Sub-second timers work correctly. Build + test pass.

---

### P3e — Remaining `#define` constants

**Agreement:** 1–2/6 (Sonnet found multiple)
**Risk:** Namespace pollution, ODR hazard, no type safety.

**Targets:**
- [x] `src/world/config_options.h` — `OPT_*` defines → `inline constexpr`
- [x] `src/world/mxp_types.h` — `TAG_*` defines → `inline constexpr`
- [x] `src/world/config_options.cpp` — `#define RGB(r,g,b)` → `constexpr makeRgb()` function
- [x] `src/world/automation_registry.cpp` + `world_serialization.cpp` — `TRIGGER_MATCH_*` already moved to trigger.h by P2b

**Acceptance:** Zero `#define` for numeric constants outside third-party code. Build + test pass.

---

### P3f — `executeLua` takes `long&` but fields are `qint32`

**Agreement:** 1/6 (Sonnet — unique find)
**Risk:** On LP64 (macOS/Linux), `long` is 64-bit but `qint32` is 32-bit. Assigning `long` back to `qint32` field silently truncates on overflow.

**Targets:**
- [x] `src/world/script_engine.h` — changed `long& invocation_count` to `qint32&`
- [x] Updated all 10 call sites

**Acceptance:** No narrowing conversions. Build + test pass.

---

### P3g — `qint32` error codes in internal C++ APIs

**Agreement:** 3/6 (Gemini, Sonnet, MiniMax)
**Risk:** `queue()`, `discardQueue()`, and various WorldDocument methods return raw `qint32` error codes instead of `std::expected`.

Note: Lua API boundary functions intentionally return integers (Lua convention). Only internal C++ APIs should be converted.

**Targets:**
- [x] `src/world/connection_manager.h` — `queue()` → `std::expected<void, WorldError>` (`discardQueue()` returns count, not error — kept as `qint32`)
- [x] Audit remaining `qint32`-returning internal methods for conversion — **Result:** All 72 error-code-returning methods are Lua API boundaries (called from `lua_api/` wrappers). No purely internal C++ methods return `qint32` error codes. No further conversions needed.

**Acceptance:** Internal C++ APIs use `std::expected`. Lua API wrappers may continue returning integers. Build + test pass.

---

### P4a — WorldDocument remains ~600-field god class

**Agreement:** 2/6 (Sonnet, Qwen)
**Risk:** Despite decomposition into 5 subsystems, WorldDocument still owns ~100+ direct fields (display, logging, scripting, UI state, flags, serialization). Constructor is ~650 lines.

**Targets:**
- [x] Group related fields into config structs — 15 structs extracted (~180 fields total), all with default member initializers and snake_case naming: `RemoteAccessConfig` (7 → `m_remote`), `LoggingConfig` (19 → `m_logging`), `ProxyConfig` (5 → `m_proxy`), `ScriptConfig` (22 → `m_scripting`), `DisplayConfig` (21 → `m_display`), `AutomationDefaultsConfig` (13 → `m_automation_defaults`), `PasteSendConfig` (17 → `m_paste`), `AutoSayConfig` (7 → `m_auto_say`), `InputConfig` (19 → `m_input`), `OutputConfig` (19 → `m_output`), `SpamPreventionConfig` (6 → `m_spam`), `CommandWindowConfig` (11 → `m_command_window`), `SpeedwalkConfig` (4 → `m_speedwalk`), `ColorConfig` (8 → `m_colors`), `SoundConfig` (6 → `m_sound`), `MappingConfig` (6 → `m_mapping`).
- [ ] Extract URL linkification (~152 lines) — `DetectAndLinkifyURLs()` + `SplitStyleForURL()` are pure text-processing algorithms with no WorldDocument state coupling. Move to a `URLLinkifier` utility class.
- [ ] Move MCCP decompression (~76 lines from `ReceiveMsg()`) into `TelnetParser` — the zlib inflate loop already uses TelnetParser's stream state but lives in WorldDocument. TelnetParser should own its own decompression.

**Status:** world_document.cpp is now ~2,855 lines. After extractions above, ~2,600. Remaining code is genuinely WorldDocument orchestration (constructor, command pipeline, line completion hub, text accumulation). Diminishing returns beyond this point.

---

### P4b — `std::shared_ptr<Action>` — only shared_ptr in codebase

**Agreement:** 1/6 (MiniMax)
**Risk:** Suspicious — should likely be `unique_ptr`. Multiple styles share the same action, so shared ownership may be intentional.

**Targets:**
- [x] Audited `std::shared_ptr<Action>` — shared ownership is genuine (multiple Styles share one Action per hyperlink span; documented in style.h and world_document.h)

**Acceptance:** Documented why shared ownership is needed.

---

### P4c — Volume/pan clamping logic (GPT-exclusive finding)

**Agreement:** 1/6 (GPT)
**Risk:** `sound_manager.cpp:245-251` collapses any out-of-range volume to 0 (full blast) instead of clamping to nearest valid value. Passing `-200` (should be muted) plays at full volume.

**Targets:**
- [x] `src/world/sound_manager.cpp` — fixed clamping with `std::clamp` (volume: -100..0, pan: -100..100)

**Acceptance:** Invalid volume/pan values clamp to nearest legal value. Build + test pass.

---

### P5a — `offsetof()` on non-standard-layout type (233 warnings)

**Agreement:** Build audit (compiler warning `-Winvalid-offsetof`)
**Risk:** `WorldDocument` inherits `QObject` (non-standard-layout). Using `offsetof()` in `config_options.cpp` to build the options table is technically undefined behavior per the C++ standard. Works in practice on all compilers, but a conforming implementation could break it.

**Targets:**
- [x] `src/world/config_options.cpp` — 233 uses of `offsetof(WorldDocument, m_field)` replaced with captureless lambda getter/setter function pointers
- [x] Options table redesigned: `iOffset`/`iLength` replaced with typed `getter`/`setter` function pointers; all `reinterpret_cast` chains eliminated from 15 consumer sites across `world_settings.cpp` and `xml_serialization.cpp`

**Acceptance:** Zero `-Winvalid-offsetof` warnings. Build + test pass.

---

### P5b — `[[nodiscard]]` return values ignored in tests (37 warnings)

**Agreement:** Build audit (compiler warning `-Wunused-result`)
**Risk:** Tests calling `std::expected`-returning functions without checking the result. These tests could be silently passing despite actual failures.

**Targets (9 test files):**
- [x] `tests/test_xml_serialization_gtest.cpp`
- [x] `tests/test_xml_roundtrip_gtest.cpp`
- [x] `tests/test_trigger_matching_gtest.cpp`
- [x] `tests/test_trigger_execution_gtest.cpp`
- [x] `tests/test_triggers_aliases_gtest.cpp`
- [x] `tests/test_alias_execution_gtest.cpp`
- [x] `tests/test_alias_integration_gtest.cpp`
- [x] `tests/test_plugin_state_gtest.cpp`
- [x] `tests/test_sendto_integration_gtest.cpp`

**Acceptance:** Zero `-Wunused-result` warnings. All `[[nodiscard]]` return values checked with `EXPECT_TRUE(result.has_value())` or equivalent. Build + test pass.

---

### P5c — `quint16` compared to `-1` (always false)

**Agreement:** Build audit (compiler warning `-Wtautological-constant-out-of-range-compare`)
**Risk:** `world_triggers.cpp:313` — comparison is always false, indicating dead code or a type error. Related to uncompleted P2b (`match_type` still `quint16`).

**Targets:**
- [x] `src/world/lua_api/world_triggers.cpp:313` — removed dead `trigger->colour == -1 ? -1 : trigger->colour` ternary (quint16 can never equal -1; simplified to `trigger->colour`)

**Acceptance:** Zero `-Wtautological` warnings. Build + test pass.

---

### P5d — Missing `override` on QObject destructors (4 sites) — DONE

**Agreement:** clang-tidy `modernize-use-override`
**Risk:** Missing `override` on virtual destructors in QObject-derived classes. If the base class destructor signature changed, the derived destructor would silently become a separate function instead of a compile error.

**Targets:**
- [x] `src/network/world_socket.h:24` — `~WorldSocket()`
- [x] `src/network/remote_access_server.h:31` — `~RemoteAccessServer()`
- [x] `src/world/miniwindow.h:74` — `~MiniWindow()`
- [x] `src/world/world_document.h:357` — `~WorldDocument()`

**Acceptance:** Zero `modernize-use-override` findings. Build + test pass.

---

### P6 — clang-tidy remaining findings (56 findings, style only)

**Agreement:** Full clang-tidy run across all src/ files (2026-02-27)
**Risk:** None of these are correctness issues. All are style/modernization suggestions.

| Check | Count | Files | Notes |
|:---|:---:|:---|:---|
| `modernize-use-trailing-return-type` | 40 | world_document.h, trigger.h, alias.h, timer.h, sendto.h, script_language.h | Style preference (`auto f() -> T`). Consider disabling this check. |
| `performance-enum-size` | 8 | sendto.h, script_language.h, timer.h, trigger.h, world_document.h, world_error.h | Enums using `int` where `uint8_t` suffices. Minor memory savings. |
| `modernize-use-default-member-init` | 6 | variable.h, world_document.h (LuaDatabase struct) | Move initializers from constructor to declaration. |
| `modernize-use-using` | 3 | variable.h (2), world_document.h (1) | `typedef` → `using`. Trivial. |
| `cppcoreguidelines-avoid-c-arrays` | 2 | style.h, world_document.h | C-style arrays → `std::array`. Already tracked in P2b. |

**Acceptance:** Informational only — fix opportunistically when touching these files.

---

## Additional Findings (single-model, non-priority)

| Finding | Source | Notes |
|---|---|---|
| `void* pAction = nullptr` dead code in world_protocol.cpp:454,753 | Sonnet | Delete dead `void*` locals |
| `AddStyle()` returns raw `Style*` into styleList — dangling risk | Sonnet | Return nothing or document lifetime |
| `void*` Qt item data payloads in plugin_dialog, plugin_wizard, activity_window | Sonnet | Use `QVariant::fromValue<T*>` directly |
| `Timer::next_create_sequence` non-atomic mutable static | Sonnet | Use `std::atomic<quint32>` |
| `Line::INITIAL_BUFFER_SIZE` is `static const` not `constexpr` | Sonnet | Change to `static constexpr` |
| Line flags are `const int` globals, not `inline constexpr` | Sonnet, MiniMax | Change to `inline constexpr` |
| `m_recentLines` holds non-owning `Line*` that could dangle | Sonnet | Document invariant or use indices |
| `QMap<Timer*, QString>` uses pointer as key — implicit stability | Sonnet | Document or use opaque ID |
| MCCP decompression on UI thread | Gemini | Move to worker thread (aspirational) |
| BGR color space mismatch with Qt's QRgb (ARGB) | Gemini | Ensure all render sites convert correctly |
| `std::deque<QString> m_recentLines` no mutex | MiniMax | Document single-thread model |
| `Timer::dispid` is QVariant, inconsistent with Trigger/Alias qint32 | Kimi | Unify dispatch ID type |

---

## Reviewer Notes

| Model | Tool | Issues Found | Unique Contributions |
|---|---|---|---|
| Gemini Pro 3.1 | gemini-cli (direct) | 16 | BGR color mismatch, MCCP threading |
| Claude Sonnet 4.6 | Task(sonnet) | 23 | scriptFileWatcher double-delete, executeLua narrowing, void* Qt data, config_options #defines |
| Qwen 3.5 397B | OpenRouter | 28 | Broadest coverage; FLAGS1/FLAGS2 enum suggestion |
| MiniMax m2.5 | OpenRouter | ~30 | shared_ptr<Action> audit, std::observer_ptr suggestions |
| GPT o4-mini | Codex CLI | 4 | SoundManager async UAF (deepest single find), timer offset bug, fractional seconds, volume clamp |
| Kimi K2.5 | OpenRouter | 13 | QVariant dispid inconsistency |
| Claude Opus 4.6 | Orchestrator | — | Synthesis and consensus ranking |

---

## DRY Audit (2026-02-28)

**Reviewer:** Gemini Pro 3.1 (via OpenRouter)
**Scope:** Full codebase — boilerplate, duplication, developer experience

---

### D1 — Lua API boilerplate (~75% of 27K LOC) [HIGH]

**Risk:** ~428 `L_*` functions across `src/world/lua_api/*.cpp` follow near-identical patterns: `doc(L)` lookup, `luaL_check*` argument extraction, call through to WorldDocument, `luaReturnOK`/`luaReturnError`. ~75% of this code is mechanical boilerplate, only ~25% is actual logic.

A declarative binding/macro system could reduce ~27K LOC to ~5K while improving consistency and making it easier for new contributors to add API functions.

**Targets:**
- [ ] Design declarative Lua binding macro/template system
- [ ] Prototype with one API category (e.g., `world_variables.cpp` — simple get/set pattern)
- [ ] Migrate remaining categories incrementally

**Acceptance:** New Lua API functions can be added with <10 lines of boilerplate. Existing functions compile and pass tests after migration.

---

### D2 — Test infrastructure duplication [HIGH] ✅ DONE

**Risk:** 49 separate test executables, many with duplicated `QCoreApplication` setup, similar `WorldDocument` fixture construction, and repeated helper utilities. Each executable has its own `main()` with identical GoogleTest + Qt initialization.

Consolidating to fewer binaries with shared test fixtures would reduce build times, simplify CI, and make it easier to add new tests.

**Targets:**
- [x] Create shared test fixture library (`tests/fixtures/`) with common `WorldDocument` setup
- [x] Create shared `main()` — `test_main_core` (QCoreApplication) and `test_main_gui` (QApplication) OBJECT libraries
- [x] Consolidate related test executables where sensible (bit + lfs + progress → `test_lua_libraries_gtest`)

**Acceptance:** Shared fixture library exists. New tests can be added without duplicating setup code. Build + test pass. 785/785 tests pass.

---

### D3 — Config option O(N) linear scan [HIGH]

**Risk:** `GetOption`/`SetOption` in `config_options.cpp` performs linear search through option arrays for every lookup. With ~233 options, this is measurably slow for scripts that query options frequently.

**Targets:**
- [ ] Replace linear scan with `std::unordered_map<QString, OptionEntry>` or similar O(1) lookup
- [ ] Maintain backward compatibility with existing option name strings

**Acceptance:** Option lookup is O(1). Build + test pass.

---

### D4 — Forwarding wrapper inconsistency ✅ DONE

**Risk:** WorldDocument has 16+ inline forwarding wrappers to companion objects (OutputFormatter, ConnectionManager, etc.). Some callers use the wrapper, others go directly to the companion — inconsistent access patterns confuse new contributors.

**Targets:**
- [x] Audit all forwarding wrappers and their call sites
- [x] Establish convention: callers always go through WorldDocument wrappers (companions are implementation details)
- [x] Document convention in CLAUDE.md or a CONTRIBUTING guide

**Resolution:** Added 21 `[[nodiscard]]` const accessor wrappers to WorldDocument (connectPhase, bytesIn/Out, isEchoOff, isCompressing, mccpType, compression stats, MXP state, automation statistics). Migrated ~26 callsites across 8 source files. Established pragmatic convention in CLAUDE.md: actions/mutations go through wrappers, state queries use accessors, collection iteration and ScriptEngine may access companions directly.

**Acceptance:** Consistent access pattern documented and followed.

---

### D5 — Repeated `std::expected` error handling in Lua API [MEDIUM]

**Risk:** Every `L_*` function that calls an `std::expected`-returning method repeats the same unwrap-and-return pattern:
```cpp
auto result = pDoc->doSomething(...);
if (!result) return luaReturnError(L, result.error().message());
```

A helper macro or function could reduce this to a single line.

**Targets:**
- [ ] Create `luaUnwrapOrReturn(L, expr)` macro/helper
- [ ] Apply to existing `L_*` functions opportunistically

**Acceptance:** Common unwrap pattern has a reusable helper. Build + test pass.

---

### D6 — God-include chain (`world_document.h` — 43 includes) [MEDIUM]

**Risk:** `world_document.h` transitively pulls in 43 headers. Most translation units that include it get far more than they need, slowing compilation and increasing coupling.

**Targets:**
- [ ] Forward-declare where possible instead of including
- [ ] Move implementation details to `.cpp` files
- [ ] Consider opaque pointer (pimpl) for rarely-accessed subsystems

**Acceptance:** Measurable reduction in include count. Build + test pass.

---

### D7 — Duplicate `BGR()` macro [LOW]

**Risk:** `BGR()` defined in both `color_utils.h` and `world_protocol.cpp`. Minor ODR/maintenance hazard.

**Targets:**
- [ ] Remove duplicate from `world_protocol.cpp`, use `color_utils.h` version

**Acceptance:** Single definition of `BGR()`. Build + test pass.
