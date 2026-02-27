---
name: runner
description: Fast triage and quick Q&A via Claude Haiku 4.5.
---
# /runner "[prompt]"

Routes to Claude Haiku 4.5 for fast, cheap responses — syntax checks, quick lookups, build error triage.

## Implementation
Use the **Task tool** with `model: "haiku"` and pass the user's prompt directly.

The Task tool call should use `subagent_type: "general-purpose"` and include the prompt.

## Example usage
- `/runner "What's the C++26 equivalent of boost::optional?"`
- `/runner "Triage this build error: <paste>"`
- `/runner "Is std::expected available in Clang 19?"`
