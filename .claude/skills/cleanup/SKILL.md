---
name: cleanup
description: Remove Hungarian notation from struct fields or local variables.
---
# /cleanup <target>

Renames Hungarian-prefixed identifiers to snake_case across the codebase.

## Arguments
- `<target>` — one of: `trigger`, `alias`, `timer`, `pdoc`

## Naming Convention
Drop the Hungarian prefix, split on case boundaries, lowercase with underscores:
- `bEnabled` -> `enabled`
- `strLabel` -> `label`
- `iSendTo` -> `send_to`
- `bKeepEvaluating` -> `keep_evaluating`
- `nInvocationCount` -> `invocation_count`
- `tWhenMatched` -> `when_matched`
- `fAtSecond` -> `at_second`
- `iOtherForeground` -> `other_foreground`

**Leave alone:** existing camelCase fields without Hungarian prefixes (e.g., `scriptLanguage`, `namedWildcards`, `owningPlugin`). These are already clean.

## Pipeline (struct targets: trigger, alias, timer)

Execute these steps **in order**.

### Step 1: Build Rename Map
Spawn a **Sonnet subagent** (Task tool, `model: sonnet`) with this prompt:

> You are a precise code generator for a C++26/Qt 6.9.3 project.
>
> Read the struct header at `src/automation/<target>.h`. For every field that uses Hungarian notation (prefixes: b, str, i, n, t, f, q), produce a rename map as a markdown table:
>
> | Old Name | New Name |
> |---|---|
> | bEnabled | enabled |
> | ... | ... |
>
> Rules:
> - Drop the prefix, split on case boundaries, join with underscores, lowercase.
> - Skip fields that are already clean (no Hungarian prefix): `trigger`, `name`, `contents`, `colour`, `wildcards`, `regexp`, `dispid`, `scriptLanguage`, `namedWildcards`, `owningPlugin`, `sound_to_play`, `ignore_case`, `omit_from_log`.
> - Do NOT rename method names or the class name.
> - Output ONLY the markdown table.

### Step 2: Display Map
Print the rename map returned by Sonnet so the user can review it before proceeding.

### Step 3: Apply Renames
Spawn a **Sonnet subagent** (Task tool, `model: sonnet`) with the rename map and this prompt:

> You are a precise code generator for a C++26/Qt 6.9.3 project.
>
> Apply the following renames across the entire codebase using `Edit(replace_all: true)` for each field. Process one field at a time.
>
> **Rename map:**
> (paste the table from Step 1)
>
> **Strategy:**
> 1. For each rename, use `Grep` to find all files containing the old name.
> 2. For each file found, use `Edit(file_path, old_string=<old>, new_string=<new>, replace_all=true)`.
> 3. Be careful with substring collisions — if renaming `bEnabled`, make sure you don't also rename `bEnabledCommand` (check the grep results for exact matches vs substrings). Use word-boundary-aware patterns in Grep (e.g., `\bbEnabled\b`).
> 4. Process ALL fields. Do not stop early.

### Step 4: Build and Test
Run `/build` and `/test` to verify the renames compile and pass tests.

### Step 5: Report
Report the results: how many fields renamed, how many files touched, build/test status.

## Pipeline (pdoc target)

### Step 1: Apply Rename
Spawn a **Sonnet subagent** (Task tool, `model: sonnet`) with this prompt:

> You are a precise code generator for a C++26/Qt 6.9.3 project.
>
> Rename the local variable `pDoc` to `doc` across all files in `src/world/lua_api/*.cpp`.
>
> Strategy:
> 1. Use `Grep` with pattern `\bpDoc\b` in `src/world/lua_api/` to find all occurrences.
> 2. For each file, use `Edit(file_path, old_string="pDoc", new_string="doc", replace_all=true)`.
> 3. Verify no other variables named `doc` already exist in scope that would collide (read each file first).

### Step 2: Build and Test
Run `/build` and `/test`.

## Notes
- The Lua API uses independent string keys (e.g., `"enabled"`, `"send_to"`), NOT the C++ field names. Renaming struct fields does NOT affect the Lua API.
- If build or test fails, the Sonnet agent should fix the issues before reporting.
- The Overseer (Opus) coordinates between steps but does NOT apply renames itself.
