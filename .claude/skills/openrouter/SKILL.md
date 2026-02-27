---
name: openrouter
description: Escape hatch to run any OpenRouter model as an agent with file access via opencode.
---
# /openrouter [model_id] "[prompt]"

Runs any OpenRouter model as a real agent with file access via opencode. Use this when a task genuinely benefits from a specialist model.

## Implementation
Parse the first argument as the OpenRouter model ID and everything after as the prompt.

Spawn a **Haiku wrapper agent** via the Task tool with `model: haiku`. The Haiku agent:

1. Runs the following command via Bash:
```bash
opencode run -m "openrouter/<MODEL_ID>" "<PROMPT>" 2>&1
```

2. Reads the output from opencode.
3. Returns the result to Opus.

**Important:**
- Do NOT pre-read source files — opencode gives the model its own file access.
- The Haiku agent should pass the prompt verbatim.
- If opencode returns an error or empty output, the Haiku agent should report the failure.

## Available models (subset)
Run `opencode models openrouter` for the full list. Common ones:
- `qwen/qwen3.5-397b-a17b` — Qwen 3.5 (large MoE)
- `qwen/qwen3-coder` — Qwen 3 Coder
- `deepseek/deepseek-r1:free` — DeepSeek R1 (free)
- `openai/o4-mini` — OpenAI o4-mini
- `openai/gpt-5-pro` — GPT-5 Pro

## Example usage
- `/openrouter qwen/qwen3-coder "Explain the plugin loading flow in src/automation/"`
- `/openrouter deepseek/deepseek-r1:free "Analyze this lock-free queue for ABA problems"`
- `/openrouter openai/o4-mini "Deep reasoning about static destruction order in this singleton"`
