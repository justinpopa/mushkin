---
name: migrate
description: Orchestrate C++26 migration of a module through the full agent council.
---
# /migrate <module>

Orchestrates the full C++26 migration pipeline for a source module.

## Arguments
- `<module>` — one of: `storage`, `network`, `world`, `ui`, `automation`, `text`, `utils`

## Pipeline
Execute these steps **in order**. Each step depends on the previous.

### Step 1: Architect Analysis
Delegate to `/gemini`:
> "Analyze the `src/<module>/` directory. Identify all raw pointers that should be std::unique_ptr or std::span, all bool-return functions that should return std::expected, and all C-style arrays. Produce a ranked list of changes by risk and a proposed migration order for files within this module."

### Step 2: Implementation
Spawn a **Sonnet subagent** (Task tool, `model: sonnet`) with the code generation persona and the Architect's plan as context:
> "Based on this migration plan, generate and apply the refactored code for each file listed. Use std::expected<T, E> for fallible functions, std::span for buffer parameters, and std::unique_ptr for ownership. Preserve all existing behavior."

The Sonnet agent reads files and applies edits directly — no need for Opus to pre-read or manually apply.

For large modules, split into per-file Sonnet calls to avoid overloading a single subagent.

### Step 3: Review
Spawn a **Sonnet subagent** (Task tool, `model: sonnet`) with the review persona:
> "Review the refactored code in `src/<module>/` for memory safety. Check: (1) no raw owning pointers remain, (2) all std::expected error paths are handled, (3) no dangling spans, (4) Qt parent-child ownership is preserved where used. Flag any issues."

The reviewer reads the files directly — no need to paste code into the prompt.

### Step 4: Fix and Verify
- If the Reviewer flagged issues, fix them (delegate to Sonnet via Task tool)
- Run `/build` to verify compilation
- Run `/test` to verify no regressions

### Step 5: Update Tracking
- Update `docs/migration_status.md` with the new module status
- Update memory files

## Notes
- If any step fails, stop the pipeline and report to the user with the failure context.
- The Overseer (Opus) coordinates between steps but does NOT do the analysis, code generation, or review work itself.
- For large modules (world/, ui/), the Architect may recommend splitting into sub-phases. Follow that recommendation.
