---
name: test
description: Run tests and triage failures.
---
# /test [optional filter]

Runs the test suite and triages any failures.

## Implementation
1. Run tests via Bash:
```bash
export QT_QPA_PLATFORM=offscreen && ctest --test-dir build --output-on-failure <FILTER>
```
   - If the user provides a filter argument, pass it as `--tests-regex <FILTER>` to ctest.
   - If no filter, run all tests.

2. If all tests **pass**, report the count and total time.
3. If any tests **fail**, delegate the failure output to the **Runner** (`/runner`) for triage. The Runner should identify: which test failed, the assertion that tripped, and suggest the likely cause.

## Example usage
- `/test` — run all tests
- `/test pipeline` — run only tests matching "pipeline"
- `/test miniwindow` — run only miniwindow tests
