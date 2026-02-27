---
name: gemini
description: Access Gemini Pro for repo-wide C++26 architectural analysis via OpenRouter.
---
# /gemini "[prompt]"

## Model
Always use `google/gemini-3.1-pro-preview` via OpenRouter. No Flash. No native CLI. No substitutions.

## Execution

```bash
opencode run -m "openrouter/google/gemini-3.1-pro-preview" "<PROMPT>" 2>&1
```

- The native `gemini` CLI is **disabled** — Google is capacity-limiting their entire paid userbase.
- If OpenRouter fails, report the failure to the user. Do NOT fall back to Flash or any other model.

## Role context
Per AGENTS.md, this skill serves as the **Architect** agent (Gemini Pro) for global refactoring planning and large-context analysis.

## Important
- Do NOT pre-read files to paste into the prompt — let opencode read them itself.
- The `opencode` binary is a full agent (like Claude Code) with file access, code search, and tool use.

## Example usage
- `/gemini "Analyze main.cpp startup sequence for C++26 modernization"`
- `/gemini "Review all signal/slot connections in src/ui/ and propose a migration plan"`
