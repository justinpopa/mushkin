---
name: fix
description: Parse architecture_review.md and fix issues by priority.
---
# /fix [target]

Parses `docs/architecture_review.md` and fixes architectural issues systematically.

## Arguments
- `<target>` — one of: `p0`, `p1`, `p2a`, `p2b`, `p3a`, `p3b`, `p4a`, or a specific issue keyword (e.g., `quint16`, `void-star`, `defines`, `socket`, `tests`, `db-errors`)

If no target is given, display the current status of all issues (checked vs unchecked boxes from the doc).

## Pipeline

### Step 0: Parse Status
Read `docs/architecture_review.md`. Extract the target priority section. Show the user which sub-items are done (`[x]`) and which remain (`[ ]`).

### Step 1: Plan (per target)

**p0 / quint16:**
Spawn a **Sonnet subagent** (Task tool, `model: sonnet`) with the code generation persona:
> Read `src/automation/trigger.h`, `src/automation/alias.h`, and identify all `quint16` fields that have boolean semantics (fields named like: enabled, ignore_case, use_regexp, repeat, multi_line, keep_evaluating, expand_variables, sound_if_inactive, omit_from_output, omit_from_log, echo_alias, omit_from_command_history, menu, etc.). Change their type from `quint16` to `bool`. Then grep for all files that assign integer literals (0, 1) to these fields and update to `false`/`true`. Also update any comparisons like `!= 0` to direct bool usage.

For `world_document.h` — the same pattern but for all `m_b*` prefixed `quint16` fields. This is a LARGE file; split into batches if needed.

**p1 / decompose:**
This is architectural work. Spawn a **Sonnet subagent** to extract ONE subsystem at a time:
1. Read `src/world/world_document.h` and identify all members/methods related to the target subsystem
2. Create a new header/source file for the extracted class
3. Update WorldDocument to hold the new class as a member and delegate
4. Run `/build` and `/test` after each extraction

Order: TelnetParser → MXPEngine → SoundManager → AutomationRegistry → ConnectionManager. Do ONE per `/fix p1` invocation.

**p2a / void-star:**
Spawn a **Sonnet subagent** to:
1. Read `src/world/world_document.h` and list all `void*` fields
2. For each, determine from context whether it should be: a concrete Qt type, removed (if unused), or initialized to nullptr with a TODO
3. Apply changes. Run `/build` and `/test`.

**p2b / defines:**
Spawn a **Sonnet subagent** to:
1. Read `src/world/world_document.h` lines 64-325 and `src/automation/trigger.h` `#define` constants
2. Group by domain: Telnet opcodes, ANSI codes, MXP modes, flags, trigger constants
3. Convert each group to `inline constexpr` values or `enum class` as appropriate
4. Move `MAX_WILDCARDS` to a shared constants header, remove duplicates
5. Run `/build` and `/test`.

**p3a / socket:**
Spawn a **Sonnet subagent** to:
1. Read the WorldSocket creation in `world_document.cpp` constructor
2. Determine if Qt parent-child ownership is in effect (check constructor args)
3. Either: document the ownership model with a comment, OR convert to unique_ptr
4. Run `/build` and `/test`.

**p3b / tests:**
Spawn a **Sonnet subagent** with the code generation persona to:
1. Find all `new WorldDocument()`, `new Trigger()`, `new Alias()`, `new Timer()`, `new Line()` in `tests/`
2. Convert to `std::make_unique` where the test owns the object
3. Remove corresponding `delete` calls
4. Fix Timer leaks in `test_timer_fire_calculation_gtest.cpp`
5. Run `/build` and `/test`.

**p4a / db-errors:**
Spawn a **Sonnet subagent** to:
1. Read the database error constants in `world_document.h`
2. Create `enum class DatabaseErrorType` and a `DatabaseError` struct
3. Convert database functions to return `std::expected<T, DatabaseError>`
4. Run `/build` and `/test`.

### Step 2: Execute
The Sonnet agent applies all changes directly using Edit/Write tools.

### Step 3: Verify
Run `/build` and `/test`. If failures occur, the Sonnet agent fixes them.

### Step 4: Update Tracking
After successful build + test:
1. Update the checkbox in `docs/architecture_review.md` from `[ ]` to `[x]` for completed items
2. Report results to user

## Notes
- The Overseer (Opus) coordinates but does NOT do the implementation work.
- For P1 (decomposition), only extract ONE subsystem per invocation to keep changes reviewable.
- For P0 (quint16), do trigger.h and alias.h together (small), then world_document.h separately (large).
- Always preserve serialization compatibility — `quint16` → `bool` conversions need explicit casts at serialization boundaries.
- If a target has no remaining unchecked items, report "already complete" and exit.
