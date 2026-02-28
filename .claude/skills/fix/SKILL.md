---
name: fix
description: Parse docs/*.md gap/review files and fix issues by target.
---
# /fix [target]

Scans all `docs/*.md` files for open checklist items (`- [ ]`) and fixes them systematically.

## Sources
- `docs/architecture_review.md` — architectural issues (p2f, p3g, p4a)
- `docs/feature_gaps.md` — missing features and API gaps (n1, n2, n5)
- `docs/ui_review.md` — UI issues (ui-mxp)

## Arguments
- `<target>` — one of the targets below, or a keyword alias

| Target | Source | Description |
|:---|:---|:---|
| `p2f` | architecture_review.md | Interface decoupling (IWorldContext) |
| `p3g` | architecture_review.md | Audit qint32 → std::int32_t |
| `p4a` | architecture_review.md | God class decomposition (config structs, long-term split) |
| `n1` | feature_gaps.md | SOCKS/HTTP proxy support |
| `n2` | feature_gaps.md | Chat system (peer-to-peer) |
| `n5` | feature_gaps.md | Logging settings infrastructure |
| `ui-mxp` | ui_review.md | MXP/Pueblo settings page |

If no target is given, display the current status of ALL open items across all docs.

## Pipeline

### Step 0: Parse Status
Read all three docs. Grep for `- [ ]` and `- [x]` lines. Show the user a summary table of open vs done items per source file and per target.

### Step 1: Plan (per target)

**p2f / interface-decoupling:**
Spawn a **Sonnet subagent** to:
1. Read `src/world/world_document.h` and identify which methods subsystems actually call
2. Define `IWorldContext` interface exposing only necessary services
3. Inject into subsystems instead of full `WorldDocument&`
4. Run `/build` and `/test` after each change

**p3g / qint32-audit:**
Spawn a **Sonnet subagent** to:
1. Grep for `qint32` across `src/` (excluding third-party)
2. Audit internal methods — convert return types to `std::int32_t` where appropriate
3. Keep `qint32` at Qt API boundaries (signals, slots, QVariant)
4. Run `/build` and `/test`

**p4a / god-class:**
Spawn a **Sonnet subagent** to:
1. Read `src/world/world_document.h` and group related fields into config structs (`DisplayConfig`, `ScriptConfig`, `LoggingConfig`, etc.)
2. Apply ONE struct grouping per invocation to keep changes reviewable
3. Long-term: further decomposition (ScriptEngine manager, UI state manager) — only if explicitly requested
4. Run `/build` and `/test`

**n1 / proxy:**
Spawn a **Sonnet subagent** to:
1. Read `src/network/connection_manager.cpp` and `src/network/world_socket.cpp`
2. Implement SOCKS4/5 proxy support using `QNetworkProxy`
3. Implement HTTP CONNECT proxy support
4. Add proxy config fields to WorldDocument and wire to preferences UI
5. Run `/build` and `/test`

**n2 / chat:**
Spawn a **Sonnet subagent** to:
1. Study the zChat/MudMaster chat protocol from `reference/mushclient-original/chat.cpp`
2. Implement peer-to-peer chat server/client
3. Implement chat API functions (ChatCall, ChatAccept, ChatMessage, etc.)
4. Run `/build` and `/test`

**n5 / logging:**
Spawn a **Sonnet subagent** to:
1. Add `m_nLogLines` (int), `m_bAppendToLogFile` (bool), log format enum to WorldDocument
2. Wire log dialog load/save to these fields (`log_dialog.cpp:79,85,102,108`)
3. Wire log format combo in world properties (`world_properties_dialog.cpp:534`)
4. Implement `%D` delta time in output preamble (`output_view.cpp:673`)
5. Run `/build` and `/test`

**ui-mxp / mxp-settings:**
Spawn a **Sonnet subagent** to:
1. Read `src/ui/dialogs/world_properties_dialog.cpp` MXP/Pueblo tab
2. Replace placeholder with functional settings (MXP mode, Pueblo support toggle, entity display)
3. Wire settings to WorldDocument MXP fields
4. Run `/build` and `/test`

### Step 2: Execute
The Sonnet agent applies all changes directly using Edit/Write tools.

### Step 3: Verify
Run `/build` and `/test`. If failures occur, the Sonnet agent fixes them.

### Step 4: Update Tracking
After successful build + test:
1. Update the checkbox in the relevant `docs/*.md` file from `[ ]` to `[x]` for completed items
2. Report results to user

## Notes
- The Overseer (Opus) coordinates but does NOT do the implementation work.
- For p4a (god class), only extract ONE config struct per invocation to keep changes reviewable.
- For n1 (proxy), prefer Qt's built-in `QNetworkProxy` over manual SOCKS implementation.
- For n2 (chat), this is a large effort — plan before implementing.
- Always preserve serialization compatibility at world file load/save boundaries.
- If a target has no remaining unchecked items, report "already complete" and exit.
