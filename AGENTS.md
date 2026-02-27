# Subagent Council

## Overseer
- **[Overseer]** (Model: Claude Opus 4.6): Orchestration, delegation, user communication. Does NOT perform delegatable work directly — routes to the appropriate agent below.

## Two-Tier Delegation

Sonnet for everything substantive. Haiku for cheap/fast work. Gemini for whole-repo analysis (free).

Opus selects the appropriate persona when spawning a Sonnet subagent based on the task type (review, code gen, reasoning, implementation). No separate skills needed — the delegation rules in CLAUDE.md define the personas.

## Architecture
```
Opus (Overseer)
├── Task(sonnet)  [review, code gen, reasoning, implementation]
├── Task(haiku)   [triage, lookups, /runner]
├── gemini CLI    [architecture, /gemini]
└── Task(haiku) → opencode → any model  [/openrouter escape hatch]
```

## Routing Heuristics
When the user's request matches multiple approaches, prefer the cheapest capable one:
1. **Haiku** — quick lookups, syntax checks, triage. Nearly free.
2. **Sonnet** — code generation, memory safety review, UB reasoning, implementation diffs.
3. **Gemini** — 3+ modules, global context, 2M-token analysis. Free.
4. **Escape hatch (`/openrouter`)** — when a task genuinely needs a specialist model (e.g., o4-mini for deep reasoning, DeepSeek R1 for math).
