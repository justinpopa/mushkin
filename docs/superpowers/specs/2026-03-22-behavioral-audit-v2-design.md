# Behavioral Audit v2: Source Re-Audit + Runtime Integration Testing

## Problem

The v1 behavioral audit (2026-03-20) compared Mushkin source against the original MUSHclient source function-by-function. It found 193 items and fixed 176. But a single debugging session on 2026-03-22 found 7 bugs that the source-level audit missed entirely:

1. **Package loaders destroyed by language modules** — MoonScript/YueScript/Teal/Fennel displaced the standard Lua file searcher; sandbox removed it
2. **Backslash paths not normalized at require time** — plugins append Windows-style paths to `package.path` at runtime
3. **C library and all-in-one loaders removed** — bundled `.so` modules couldn't load
4. **Font DPI mismatch** — `setPointSize()` at macOS 72 DPI produces 25% smaller fonts than Windows 96 DPI
5. **Global font defaults applied to existing worlds** — `applyGlobalFontDefaults()` ran in constructor, overwriting world file font sizes
6. **Backslash paths in preferences database** — `resolveDirectory()` didn't normalize backslashes from Wine's `mushclient_prefs.sqlite`, breaking plugin state load/save
7. **Plugin state loaded after script body** — `LoadState()` ran after `parseScript()`, so top-level `GetVariable()` calls always returned nil

All of these are integration/runtime issues that only manifest when running with real plugin files, a shared preferences database, and a macOS display. Source comparison alone cannot catch them.

## Solution

Two complementary tools feeding one worklist:

1. **Baseline capture plugin** — runtime comparison via a MUSHclient plugin that snapshots all observable API behavior
2. **Enhanced source re-audit** — full 16-subsystem source comparison with Opus agents and new criteria targeting the bug classes found on 2026-03-22

## Component 1: Baseline Capture Plugin (`mushkin_audit.xml`)

A MUSHclient plugin that captures a comprehensive snapshot of observable behavior and writes it to JSON. Runs identically in both Wine MUSHclient and Mushkin.

### What it captures

| Category | APIs | Discovery | Approximate items |
|:---|:---|:---|:---|
| GetInfo | GetInfo(1-310) | Hardcoded range | ~310 |
| GetOption/GetAlphaOption | All world options | Hardcoded name list extracted from `config_options.cpp` OptionsTable/AlphaOptionsTable | ~250 |
| Plugin enumeration | GetPluginList + GetPluginInfo(id, 1-28) | GetPluginList for discovery, codes 1-28 per plugin | ~55 plugins x 28 |
| Plugin state | GetVariable for known variable names per plugin | GetPluginInfo(id, 24) returns variable list | varies |
| Miniwindow metrics | WindowInfo(win, 1-18) | WindowList() for discovery | varies |
| Font metrics | WindowFontInfo(win, fontid, 1-20) | WindowList() then WindowFontList(win) for discovery | varies |
| Lua API availability | Check all 428 API functions exist as globals | Hardcoded list | 428 |
| Lua environment | _VERSION, jit, bit, bit32, utf8, package.path, package.cpath, #package.loaders, type(package.loaders), type(package.searchers) | Direct access | ~10 |
| Error codes | error_code table values | Iterate error_code table | ~70 |
| Error-path probes | GetOption("nonexistent"), GetInfo(99999), etc. | Hardcoded list of known-bad inputs | ~20 |

### Behavior

- Triggered by typing `audit` (registered as an alias)
- Collects all data into a Lua table
- Serializes to JSON via a self-contained JSON encoder embedded in the plugin (~60 lines, handles strings, numbers, booleans, nil, nested tables; sorts keys at all nesting levels for stable diffs)
- Does NOT depend on the host's `json.lua` (eliminates a variable from the comparison)
- Writes to `audit_baseline.json` in the app directory via `io.open`
- Includes metadata header: `capture_date`, `client_name`, `client_version`, `world_name`, `plugin_count`, `world_file_hash`
- API calls that error are recorded with a sentinel value `"__error__: <message>"` (errors are valuable diff data)
- Prints summary to output window (count of items captured, errors)
- Read-only — does not modify any state

### Option name enumeration

`GetOption(name)` and `GetAlphaOption(name)` require the option name upfront — there is no `GetOptionList()` API. The plugin embeds the full list of option names, extracted from `config_options.cpp` `OptionsTable[]` and `AlphaOptionsTable[]`. This list is generated once during plugin development and hardcoded.

## Component 2: Diff Script (`scripts/audit_diff.py`)

Python script that compares two baseline JSON files and produces actionable output.

### Input

- `audit_baseline_original.json` (from Wine MUSHclient)
- `audit_baseline_mushkin.json` (from Mushkin)

### Output

- Console report grouped by category
- Each difference classified as: **MISMATCH** (different values), **MISSING** (present in original, absent in Mushkin), or **EXTRA** (present in Mushkin, absent in original)
- Known-acceptable differences filtered via allowlist
- Exit code 0 if no unexpected differences (CI-friendly)
- Findings written to `docs/audit_v2_runtime_findings.md` for human triage into the worklist (the diff script does not assign severity — that requires human judgment)

### Allowlist (`scripts/audit_allowlist.json`)

A JSON file listing keys to skip or normalize:

- **Path values** — normalize separators, strip platform-specific prefixes
- **Version strings** — skip (expected to differ)
- **Timestamps/uptime** — skip (non-deterministic)
- **Window pixel positions** — allow tolerance (within N pixels)
- **Platform-specific values** — OS type, executable path, etc.

### Metadata validation

Before diffing, the script validates that both JSON files have compatible metadata (same world name, similar plugin count). Warns if the world configurations appear different (prevents false positives from mismatched setups).

## Component 3: Enhanced `/auditloop` Skill

Same 16-subsystem structure, same worklist output format. Changes:

### Agent model

**Opus** for all 16 subsystems, dispatched in parallel. The v1 audit used Sonnet, which missed all of the bugs found on 2026-03-22. The remaining issues require deeper reasoning about init ordering, side effects, and cross-layer interactions.

### Subsystem queue reset

All 16 subsystems are marked as unaudited for v2. The `[audited]` markers in the skill file are reset. The new criteria require re-examining every subsystem, not just those that changed.

### New comparison criteria

Added to every subsystem audit:

1. **Init ordering** — verify operations happen in the correct sequence. Examples: LoadState before parseScript, default fonts only for new worlds, package.loaders saved before language modules load.
2. **Path handling** — verify all path strings go through backslash normalization before use. Check paths from preferences database, world files, plugin XML, and Lua API returns.
3. **DPI assumptions** — verify font sizes use the 96 DPI conversion (`pixelSize = pointSize * 96 / 72`), not platform-dependent `setPointSize()`.
4. **State round-trips** — verify SaveState and LoadState use the same resolved path, and that GetVariable returns what SetVariable stored.

## Workflow

### One-time setup

1. Build `mushkin_audit.xml` (with self-contained JSON encoder and hardcoded option name list)
2. Build `scripts/audit_diff.py` and `scripts/audit_allowlist.json`
3. Update `/auditloop` skill: reset subsystem queue, add new criteria, switch to Opus agents

### Baseline capture

1. Install `mushkin_audit.xml` in Wine MUSHclient plugins folder
2. Launch Wine MUSHclient, load Aardwolf world, wait for all plugins to load, type `audit`
3. Copy `audit_baseline_original.json` to Mushkin repo
4. Launch Mushkin with same world files and plugins (copied from Wine), type `audit`
5. Run: `python3 scripts/audit_diff.py audit_baseline_original.json audit_baseline_mushkin.json`
6. Review `docs/audit_v2_runtime_findings.md`, triage findings into worklist

### Source re-audit

1. Run `/auditloop` — dispatches 16 Opus agents with enhanced criteria
2. New findings appended to `docs/behavioral_worklist.md`

### Fix cycle

1. Run `/fixloop` or `/loop 5m /fixloop` to process worklist items
2. Re-run baseline capture in Mushkin periodically to verify fixes
3. Re-diff to confirm convergence toward zero differences

### Regression prevention

The baseline plugin + diff script are reusable. After any significant change:
1. Re-run `audit` in Mushkin
2. Diff against the original baseline
3. Investigate any new differences

## Success criteria

- Baseline diff produces zero unexpected differences (all differences are either fixed or in the allowlist with documented justification)
- Source re-audit worklist reaches zero unchecked items
- All 1003+ tests continue passing
- The Aardwolf plugin pack loads with the same plugin count, font sizes, window positions, and saved state as Wine MUSHclient
