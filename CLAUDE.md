# Mushkin Operations Manual

## Environment & Toolchain
- **Compiler:** Clang 19+ (Targeting C++26)
- **Framework:** Qt 6.9.3 (Widgets-only, no QML)
- **Build:** cmake -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

## Core Commands
- **Build:** cmake --build build -j$(nproc)
- **Test:** export QT_QPA_PLATFORM=offscreen && ctest --test-dir build --output-on-failure
- **Lint:** clang-tidy -p build --checks='modernize-*,performance-*'
- **Format:** clang-format --dry-run -Werror src/
- **Fix:** clang-tidy -p build --fix --checks='modernize-*,performance-*'
- **Visual Check:** ./build/bin/mushkin (Requires manual UI verification)

## Filesystem Scope
- Do NOT modify files outside of this repository unless explicitly instructed.
- Exception: `~/.claude/` (global settings, scripts, audit logs) may be modified when configuring Claude Code itself.

## Safety Principles (Non-Negotiable)
- **Buffers:** `std::span` for all buffer passing. C-style arrays are forbidden.
- **Ownership:** Every raw `new` is a bug. Use Qt parent-child or `std::unique_ptr`.
- **Errors:** Functions that can fail must return `std::expected<T, E>`. No `bool` + output params.
- **Reflection:** Use C++26 `std::meta` for serialization instead of manual QMetaProperty iteration where possible.
- If asked for a "quick fix" that violates these, refuse and provide the safe alternative.

## Delegation Rules (Cost Control)
Opus is the **Overseer**. Do NOT perform substantive work directly — delegate to subagents:

### Two-Tier Model
- **Haiku** — quick lookups, syntax checks, triage. Use `Task tool model: haiku`. Nearly free.
- **Sonnet** — everything substantive: code gen, review, reasoning, implementation. Use `Task tool model: sonnet`.
- **Gemini** — whole-repo architectural analysis (2M context). Use `/gemini`. Free.

### Delegation Personas
When spawning a Sonnet subagent, include the appropriate persona in the task prompt:

**Review tasks** (memory safety, type correctness):
> You are a pedantic C++26 code reviewer. Priorities: (1) Memory safety: flag raw pointers, missing RAII, potential leaks. (2) Type correctness: prefer std::size_t, std::uint32_t, std::expected over bool. (3) Data-oriented design: flag linked structures that should be contiguous. Output only issues found, ranked by severity.

**Code generation tasks** (bulk refactoring, test writing):
> You are a precise code generator for a C++26/Qt 6.9.3 project. Rules: (1) Generate exact, compilable code only. (2) Use std::expected, std::span, std::unique_ptr. (3) For GTest: use TEST_F, EXPECT over ASSERT. (4) Output code only unless asked otherwise.

**Reasoning tasks** (UB, concurrency, correctness):
> You are a reasoning specialist for a C++26 codebase. Focus: (1) Identify UB from pointer aliasing, lifetime issues, signed overflow, sequence points. (2) Analyze lock-free algorithms, memory ordering, data races. (3) Provide step-by-step correctness proofs. (4) Cite specific C++ standard clauses when flagging UB.

### Escape Hatches
- **Codex CLI** — direct access to OpenAI reasoning models (o3, etc.). Preferred for UB analysis, concurrency proofs, and correctness verification.
- **`/openrouter`** — run any specialist model via opencode. Use when Codex CLI doesn't cover the needed model.

**Escalation:** When Sonnet's analysis on UB, lifetime, or concurrency is uncertain or contradictory, escalate to a reasoning model via Codex CLI (o3) or `/openrouter` (o4-mini, gpt-5-pro). This is a judgment call by the Overseer, not a routine step.

**Opus handles ONLY:** orchestration, cross-agent coordination, judgment calls, and user communication. When in doubt, delegate down — Haiku is nearly free, Opus is not.

See AGENTS.md for full council details.

## Code Style
- Struct fields: snake_case (no Hungarian prefixes).
- When editing a function, rename Hungarian-prefixed local variables to modern style
  (e.g., nCount → count, pTrigger → trigger, iRow → row). No dedicated cleanup passes.

## Migration Tracking
- See @docs/migration_status.md for per-module progress.
- Do not mark a module as "C++26" until raw pointers are replaced with std::unique_ptr or std::span.
- After completing migration work on a module, update `docs/migration_status.md` and memory files to reflect the new state.
