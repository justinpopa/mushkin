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
├── codex CLI     [o3 reasoning — UB, concurrency, correctness]
└── Task(haiku) → opencode → any model  [/openrouter escape hatch]
```

## Routing Heuristics
When the user's request matches multiple approaches, prefer the cheapest capable one:
1. **Haiku** — quick lookups, syntax checks, triage. Nearly free.
2. **Sonnet** — code generation, memory safety review, UB reasoning, implementation diffs.
3. **Gemini** — 3+ modules, global context, 2M-token analysis. Free.
4. **Codex CLI** — direct OpenAI reasoning (o3) for UB, concurrency, correctness proofs.
5. **Escape hatch (`/openrouter`)** — when a task needs a model not available via Codex CLI.

### Fallback Behavior
When routing is ambiguous, prefer the cheaper model. Haiku should be attempted first for triage/lookup tasks before escalating to Sonnet.

### Error Handling
- **Runner** reports triage findings back to the Overseer with suggested fixes
- **Sonnet** escalates to Codex CLI (o3) or `/openrouter` when initial analysis is uncertain or contradictory
