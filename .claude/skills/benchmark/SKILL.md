---
name: benchmark
description: Run performance benchmarks and compare before/after results.
---
# /benchmark [command or test name]

Runs a performance benchmark and optionally compares against a previous baseline.

## Implementation

### If a specific command is provided:
1. Run the command via Bash with `time` wrapper:
```bash
hyperfine --warmup 3 --runs 10 '<COMMAND>' 2>&1 || \
  { time for i in $(seq 10); do <COMMAND> > /dev/null 2>&1; done; } 2>&1
```
   - Prefer `hyperfine` if available (more accurate). Fall back to shell `time` loop.

2. Report: mean time, min, max, std deviation.

### If "build" is specified:
Benchmark a full rebuild:
```bash
cmake --build build --clean-first -j$(sysctl -n hw.ncpu)
```

### If "test" is specified:
Benchmark the full test suite:
```bash
export QT_QPA_PLATFORM=offscreen && ctest --test-dir build
```

### Comparing before/after:
When the user says `/benchmark` after making changes, check if a previous benchmark result exists in `/tmp/mushkin_benchmark_baseline.txt`. If so, compare and report the delta. If not, save the current result as the baseline.

```bash
# Save baseline
echo "<RESULT>" > /tmp/mushkin_benchmark_baseline.txt

# Compare
cat /tmp/mushkin_benchmark_baseline.txt
```

## Example usage
- `/benchmark build` — benchmark a clean rebuild
- `/benchmark test` — benchmark the full test suite
- `/benchmark "./build/tests/test-pipeline-gtest"` — benchmark a specific test
