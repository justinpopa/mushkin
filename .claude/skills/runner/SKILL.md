---
name: runner
description: Fast triage and quick Q&A via Claude Haiku 4.5.
---
# /runner "[prompt]"

Uses Claude Haiku 4.5 for fast, cheap responses — syntax checks, quick lookups, build error triage.

## Implementation
Do NOT call an external API. Instead, use the **Task tool** with `model: "haiku"` and pass the user's prompt directly. This routes to Claude Haiku 4.5 via the built-in subagent system — no API key or curl needed.

The Task tool call should use `subagent_type: "general-purpose"` and include the prompt.

## Example usage
- `/runner "What's the C++26 equivalent of boost::optional?"`
- `/runner "Triage this build error: <paste>"`
- `/runner "Is std::expected available in Clang 19?"`
