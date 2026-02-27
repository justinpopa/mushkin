# Mushkin Architecture Review — Multi-Model Consensus

**Date:** 2026-02-27
**Reviewers:** Gemini Pro 3.1, Claude Sonnet 4.6, Qwen 3.5 397B, Kimi K2.5, MiniMax m2.5, Claude Opus 4.6 (orchestrator)

## Priority Issues

Issues are ranked by cross-model agreement and risk. Each issue has a target and acceptance criteria so `/fix` can process them mechanically.

### P0 — quint16 booleans → bool

**Agreement:** 6/6 unanimous
**Risk:** Subtle bugs from non-0/1 values in serialized data; wasted memory; obscured intent.

~60+ fields across Trigger, Alias, and WorldDocument use `quint16` for boolean semantics (`enabled`, `ignore_case`, `use_regexp`, `repeat`, `keep_evaluating`, etc.). Timer already uses `bool` correctly — it's the reference pattern.

**Targets:**
- [x] `src/automation/trigger.h` — convert `quint16` boolean fields to `bool`
- [x] `src/automation/alias.h` — convert `quint16` boolean fields to `bool`
- [x] `src/world/world_document.h` — convert `quint16` boolean fields to `bool` (m_bEnable*, m_bLog*, m_bAuto*, etc.)

**Acceptance:** All `quint16` fields with boolean semantics are `bool`. Serialization code uses explicit casts at read/write boundaries. Build + test pass.

---

### P1 — WorldDocument God Object decomposition

**Agreement:** 6/6 unanimous
**Risk:** Merge conflicts, untestable subsystems, growing complexity. 1200+ line header, 200+ member fields.

**Targets:**
- [x] Extract `TelnetParser` — telnet negotiation state, IAC handling, MCCP
- [x] Extract `MXPEngine` — MXP parsing, element/entity management
- [x] Extract `SoundManager` — audio engine, spatial sound, sound buffers
- [x] Extract `AutomationRegistry` — trigger/alias/timer maps, sorted arrays, evaluation pipeline
- [x] Extract `ConnectionManager` — socket, connection phases, reconnect logic

**Acceptance:** Each extracted class is self-contained with a clear interface. WorldDocument delegates to these components. Build + test pass. No functional changes.

---

### P2a — Replace void* placeholders

**Agreement:** 5/6 models
**Risk:** Latent UB from uninitialized void*. Blocks feature completion.

20+ `void*` fields in `world_document.h` for incomplete MFC→Qt port:
- `m_font[16]`, `m_pLinePositions`, `m_ActionList`
- `m_DisplayFindInfo` and 7 more FindInfo fields
- `m_BackgroundBitmap`, `m_sockAddr`, `m_hNameLookup`
- `m_strMapList`, `m_MapFailureRegexp`, `m_Databases`
- `m_ColourTranslationMap`, `m_OutstandingLines`

**Targets:**
- [x] Audit all `void*` fields in `src/world/world_document.h`
- [x] For each: replace with concrete Qt type, or remove if unused, or initialize to `nullptr` with `// TODO` comment

**Acceptance:** Zero `void*` fields remain in world_document.h. Build + test pass.

---

### P2b — #define constants → constexpr / enum class

**Agreement:** 5/6 models
**Risk:** No type safety, namespace pollution, naming collisions.

70+ `#define` macros in `world_document.h` for Telnet opcodes, ANSI codes, MXP modes, flag bits, connection phases.

**Targets:**
- [x] `src/world/world_document.h` lines 64-325 — convert to `inline constexpr` or `enum class`
- [x] `src/automation/trigger.h` — `TRIGGER_COLOUR_CHANGE_*` and `MAX_WILDCARDS`
- [x] `src/automation/alias.h` — `MAX_WILDCARDS` (duplicate, ODR hazard)

**Acceptance:** Zero `#define` for numeric/flag constants in these headers (excluding include guards). Shared constants in a single header. Build + test pass.

---

### P3a — WorldSocket ownership

**Agreement:** 3/6 models
**Risk:** Ownership violation — `new WorldSocket(this, this)` stored as raw pointer.

**Targets:**
- [ ] `src/world/world_document.h:416` — `WorldSocket* m_pSocket` → document Qt parent-child ownership or convert to `std::unique_ptr` with custom deleter

**Acceptance:** No undocumented raw owning pointer for m_pSocket. Build + test pass.

---

### P3b — Test code RAII

**Agreement:** 2/6 models (Sonnet found deepest)
**Risk:** Test leaks hide production leaks; contradicts safety principles; bad example for contributors.

**Targets:**
- [ ] All `new WorldDocument()` in test fixtures → `std::make_unique`
- [ ] All `new Trigger()` / `new Line()` in tests → `std::make_unique`
- [ ] Remove manual `delete` calls
- [ ] Fix Timer memory leaks in `test_timer_fire_calculation_gtest.cpp`

**Acceptance:** No raw `new`/`delete` in test code (except documented Qt parent-child). ASan clean.

---

### P4a — Database error sentinels → enum class

**Agreement:** 2/6 models
**Risk:** Low — localized to database module. Integer sentinel values (-1 to -7) are the last non-typed error pattern.

**Targets:**
- [ ] `src/world/world_document.h` lines 95-101 — convert `const qint32` sentinels to `enum class DatabaseErrorType`

**Acceptance:** Database operations use `std::expected<T, DatabaseError>`. Build + test pass.

---

### P4b — std::meta reflection for serialization

**Agreement:** 4/6 models (aspirational — no compiler support yet)
**Risk:** None immediate. This is the long-term goal stated in CLAUDE.md.

**Targets:**
- [ ] Monitor Clang std::meta implementation status
- [ ] When available: replace manual XML serialization for Trigger, Alias, Timer

**Acceptance:** Deferred until compiler support lands.

---

## Additional Findings (non-priority)

| Finding | Source | Notes |
|---|---|---|
| `long invocation_count` output param anti-pattern (8 sites) | Sonnet | Should return value instead of pointer-to-long |
| `std::shared_ptr<Action>` — only shared_ptr in codebase | Sonnet | Suspicious — likely should be unique_ptr |
| `MAX_WILDCARDS` defined in both trigger.h and alias.h | Sonnet | ODR hazard — move to shared constants header |
| `compileRegexp()` failure silently dropped in world_trigger_matching.cpp | Sonnet | Error string from std::expected discarded |
| `packFlags()`/`unpackFlags()` bitset sync pattern is brittle | Gemini | Flag bits can desync from bool members |
| Timer `int type` should be `enum class TimerType` | Sonnet, Kimi, Qwen | Field declared as `int` but enum exists |
| `QVector`/`QMap` used where `std::vector`/`std::map` would be cleaner | Qwen, MiniMax | Cosmetic — Qt 6 aliases to std containers |
| Missing network-layer tests | Sonnet | No test_network_* files despite module marked complete |
