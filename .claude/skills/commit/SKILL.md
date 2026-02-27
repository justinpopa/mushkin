---
name: commit
description: Stage changes and create a conventional commit for Release Please.
---
# /commit [optional message hint]

Creates a conventional commit from staged/unstaged changes. Enforces the conventional commit format required by Release Please.

## Implementation
1. Run `git status` and `git diff --stat` via Bash to see what changed.
2. Analyze the changes and determine the appropriate conventional commit type:
   - `feat:` — new feature or capability
   - `fix:` — bug fix
   - `refactor:` — code change that neither fixes a bug nor adds a feature
   - `perf:` — performance improvement
   - `test:` — adding or updating tests
   - `docs:` — documentation only
   - `chore:` — maintenance tasks (deps, CI, configs)
   - `build:` — build system changes
   - Append `!` for breaking changes (e.g., `feat!:`)

3. If the user provided a message hint, use it to guide the commit message.
4. Draft the commit message in this format:
   ```
   type(scope): short description

   Longer explanation if needed.
   ```
   - `scope` is optional but preferred — use the module name (e.g., `world`, `ui`, `network`, `storage`)
   - Short description: imperative mood, lowercase, no period, under 72 chars
   - Body: explain the "why", not the "what"

5. Stage relevant files with `git add` (specific files, not `-A`).
6. Create the commit.

## Important
- NEVER add co-author lines.
- NEVER use `git add -A` or `git add .` — stage specific files.
- If unsure about the commit type, ask the user.
- If there are unrelated changes, suggest splitting into multiple commits.

## Example usage
- `/commit` — auto-detect type and message from the diff
- `/commit "fixed the telnet timeout bug"` — use the hint to craft the message
