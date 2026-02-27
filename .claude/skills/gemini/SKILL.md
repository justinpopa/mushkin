---
name: gemini
description: Access Gemini 3 (Flash/Pro) for repo-wide C++26 architectural analysis.
---
# /gemini [--flash] "[prompt]"

When invoked, parse the arguments:
- If the first argument is `--flash`, use model `gemini-3-flash-preview` and treat the rest as the prompt.
- Otherwise (default), use model `gemini-3.1-pro-preview` and treat all arguments as the prompt.

Run via Bash with `--yolo` and `--approval-mode yolo` so Gemini can freely read files, search code, and use its tools without blocking on approval prompts. Use `-o text` for clean output:

```bash
gemini -m "<MODEL>" -p "<PROMPT>" --approval-mode yolo -o text
```

The `gemini` binary is a full agent (like Claude Code) with file access, code search, and tool use. The `--yolo` flags let it autonomously explore the repo and return results — no need to pre-read files or pipe content.

## Role context
Per AGENTS.md, this skill serves as the **Architect** agent (Gemini 3.1 Pro) for global refactoring planning and 2M-context analysis. Default to Pro unless the user explicitly requests Flash for speed/quota reasons.

## Important
- Gemini is **free** (school account). Prefer routing work here over paid models when the task fits.
- Gemini has its own project context via `GEMINI.md` and `.gemini/system.md` in the repo root.
- Do NOT pre-read files to paste into the prompt — let Gemini read them itself.

## Example usage
- `/gemini "Analyze main.cpp startup sequence for C++26 modernization"` (Pro, default)
- `/gemini --flash "Quick summary of includes in src/"` (Flash, explicit)
- `/gemini "Review all signal/slot connections in src/ui/ and propose a migration plan to C++26 generic lambdas"`
