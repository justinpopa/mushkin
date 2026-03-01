# Future Analysis Paths

Candidate analysis passes for the Mushkin codebase, ordered by expected value.

## 1. Security Audit [HIGH]
Network-facing app parsing untrusted telnet/MCCP/MXP/ANSI data. Specific concerns:
- Buffer handling in zlib (MCCP) decompression paths
- MXP tag injection / malformed element parsing
- Malicious ANSI escape sequences (terminal escape attacks)
- Lua sandbox escapes (os.execute, io.popen, loadfile, dofile)
- Remote access server (telnet listener) — authentication, command injection, DoS
- XML deserialization of world files / plugins (XXE, entity expansion)

## 2. Runtime Defect Detection [HIGH]
Run ASan/UBSan/LSan against the test suite and manual sessions:
- Use-after-free in signal/slot timing (disconnect/reconnect)
- Undefined behavior in arithmetic (signed overflow, shift amounts)
- Memory leaks in Qt parent-child edge cases
- Stack buffer overruns in protocol parsing

## 3. Dead Code Elimination [MEDIUM]
Identify and remove unreachable code:
- Methods declared in world_document.h but never called
- Enum values with no references
- Entire functions ported from MUSHclient that have no callers
- Ifdef'd code blocks that can never trigger on target platforms
- Good candidate for Gemini (2M context, can scan whole repo)

## 4. Test Coverage Analysis [MEDIUM]
Measure actual line/branch coverage with gcov/llvm-cov:
- Protocol parsers (telnet, MXP, ANSI)
- XML serialization round-trips with edge cases
- Error paths in std::expected-returning functions
- MiniWindow drawing operations
- Plugin load/unload lifecycle

## 5. Plugin Compatibility Testing [MEDIUM]
Load real-world MUSHclient plugin packs and verify behavior:
- Aardwolf plugin pack (most popular)
- Discworld plugins
- Generic mapper plugins
- Plugins that exercise: timers, triggers, aliases, miniwindows, sounds
- Requires manual UI verification for many features

## 6. Thread Safety Audit [MEDIUM]
Audit signal/slot connections crossing thread boundaries:
- WorldSocket ↔ WorldDocument data flow during disconnect/reconnect
- Timer evaluation loop vs incoming data processing
- Script engine callbacks during network events
- Remote access server concurrent connections

## 7. Remaining Lua API Gaps [LOW]
39 functions remaining (~9% of API):
- Triage by plugin usage frequency
- Some may be trivial stubs, others may block real plugins
- See docs/feature_gaps.md for full list
