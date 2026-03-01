# Security Audit — Mushkin MUD Client

**Date:** 2026-02-28
**Auditors:** Gemini 3.1 Pro (broad scan), Claude Opus (verification + deep dive)
**Scope:** Network protocol parsing, remote access server, Lua scripting, XML deserialization

---

## Findings

### S1 — No Lua Sandbox [CRITICAL]

**Location:** `src/world/script_engine.cpp:240`
**Description:** `luaL_openlibs(L)` registers ALL standard Lua libraries including `os`, `io`, `debug`, and `package`. There is no restriction on dangerous functions.

**Available to any plugin/script:**
- `os.execute("rm -rf /")` — arbitrary command execution
- `io.open("/etc/passwd")` — read any file on the system
- `io.popen("curl ...")` — network access from Lua
- `package.loadlib()` — load arbitrary shared libraries
- `debug.getregistry()` — access Lua internals
- `require("lfs")` — LuaFileSystem gives full directory traversal

**Attack scenario:** A shared world file (`.mcl`) can reference a plugin that runs `os.execute()` on load. Since world files are commonly shared in MUD communities, this is a practical attack vector.

**Note:** This is **by design** for MUSHclient compatibility — the original also had no sandbox. However, it should be documented as a known risk, and a future "untrusted plugin" mode should be considered.

**Recommended fix:**
- [ ] Document the risk prominently (plugins run with full user privileges)
- [ ] Add a user confirmation prompt before executing scripts from newly loaded plugins
- [ ] Long-term: optional sandbox mode that disables `os`, `io`, `debug`, restricts `package.path/cpath`

---

### S2 — Unbounded Subnegotiation Buffer [HIGH]

**Location:** `src/world/telnet_parser.cpp:514`
**Description:** `Phase_SUBNEGOTIATION` appends bytes to `m_IAC_subnegotiation_data` with no size limit:
```cpp
m_IAC_subnegotiation_data.append(c);
```

**Attack scenario:** A malicious MUD server sends `IAC SB <type>` followed by gigabytes of data without ever sending `IAC SE`. The buffer grows until OOM.

**Recommended fix:**
- [x] Add a size limit (8KB) to `Phase_SUBNEGOTIATION`; abort and reset if exceeded

---

### S3 — Unbounded Line Buffer [HIGH]

**Location:** `src/world/world_document.cpp` — `AddToLine` / line accumulation
**Description:** Incoming text is accumulated into the current line buffer until a newline is received. A server sending continuous data without newlines causes unbounded memory growth.

**Attack scenario:** Malicious server sends megabytes of text without `\n`. The line buffer grows until OOM.

**Recommended fix:**
- [x] Force a line break after 1MB maximum

---

### S4 — Unbounded Remote Client Input Buffer [HIGH]

**Location:** `src/network/remote_client.cpp:142`
**Description:** `processInput` appends to `m_inputBuffer` without any size limit:
```cpp
m_inputBuffer.append(QString::fromUtf8(data));
```

An unauthenticated client can send data without newlines, growing the buffer indefinitely.

**Attack scenario:** Attacker connects to the remote access port and streams data without `\n`. No authentication required — the buffer grows during the `AwaitingPassword` state. OOM crash.

**Recommended fix:**
- [x] Limit `m_inputBuffer` to 4KB; disconnect client if exceeded

---

### S5 — MXP Entity Expansion Without Depth/Size Limits [MEDIUM]

**Location:** `src/world/mxp_engine.cpp:1510-1531`
**Description:** `MXP_DefineEntity` expands entity references within entity values via `MXP_GetEntity()`, which can recursively resolve custom entities. While a single level of expansion is implemented iteratively, an entity `&a;` can reference `&b;` which references `&c;`, etc. The expansion is not recursive in the code, but chained definitions can create exponential growth if the server defines them in the right order.

**Attack scenario:** Server defines: `ENTITY a "XXXX..."` (large value), then `ENTITY b "&a;&a;&a;&a;"`, then `ENTITY c "&b;&b;&b;&b;"`. Each level quadruples the size.

**Mitigating factor:** `MXP_GetEntity` does a single lookup (no recursive expansion at use time), so the bomb only works at definition time via `MXP_DefineEntity`. The attack requires multiple `!ENTITY` definitions sent in sequence.

**Recommended fix:**
- [ ] Cap expanded entity value length (e.g., 64KB) in `MXP_DefineEntity`

---

### S6 — Plugin XML Entity Expansion [MEDIUM]

**Location:** `src/world/world_document_plugins.cpp:148-170`
**Description:** `preprocessPluginXml` extracts `<!ENTITY>` definitions and expands them with `QString::replace`. The expansion is single-pass (entities defined in terms of other entities won't chain), but a malicious plugin could define a very large entity value and reference it many times:
```xml
<!ENTITY big "AAAA...64KB...AAAA">
<!-- then reference &big; 1000 times in attribute values -->
```

**Mitigating factor:** Plugins are local files the user chose to install. This is not remotely exploitable.

**Recommended fix:**
- [ ] Cap total preprocessed XML size (e.g., 50MB) as a safety valve

---

### S7 — Non-Constant-Time Password Comparison [LOW]

**Location:** `src/network/remote_client.cpp:196`
**Description:** `checkPassword` uses `attempt == m_password` (standard `QString::operator==`), which returns early on first mismatch.

**Attack scenario:** Timing side-channel to determine password character-by-character. Requires high-precision timing and many attempts.

**Mitigating factor:** 3-attempt limit (`kMaxPasswordAttempts`) makes timing attacks impractical. Network jitter dwarfs comparison time differences.

**Recommended fix:**
- [ ] Use constant-time comparison (nice-to-have, low priority given attempt limit)

---

### S8 — Remote Access Commands Execute Without Restriction [MEDIUM]

**Location:** `src/network/remote_access_server.cpp:185`
**Description:** Once authenticated, `onClientCommand` passes the command directly to `m_pDoc->Execute(command)` with no filtering. This means a remote client can:
- Execute any Lua code via `DoAfterSpecial` or trigger-based injection
- Load/unload plugins
- Modify world configuration
- Access all MUSHclient API functions

This is **by design** (mirrors MUSHclient original), but the remote client has identical privileges to the local user.

**Recommended fix:**
- [ ] Document that remote access = full control
- [ ] Consider optional command whitelist for restricted remote sessions

---

### S9 — Password Stored in Plaintext in Memory [LOW]

**Location:** `src/network/remote_client.h:66` — `QString m_password`
**Description:** The remote access password is stored as a plaintext `QString` in memory. It's also passed by value to each `RemoteClient` constructor.

**Mitigating factor:** This is a local application, not a server. The password is also stored in the world file (XML) in plaintext, matching MUSHclient original behavior.

**Recommended fix:**
- [ ] Low priority — document as known limitation

---

## Summary

| ID | Severity | Category | Status |
|:---|:---|:---|:---|
| S1 | CRITICAL | Lua sandbox | Accepted risk (MUSHclient compat) |
| S2 | HIGH | Protocol DoS | **Fixed** — 8KB subnegotiation buffer cap |
| S3 | HIGH | Protocol DoS | **Fixed** — 1MB line length cap with forced break |
| S4 | HIGH | Remote access DoS | **Fixed** — 4KB input buffer cap, disconnect on exceed |
| S5 | MEDIUM | MXP DoS | Needs fix |
| S6 | MEDIUM | Plugin XML DoS | Low priority (local files) |
| S7 | LOW | Timing attack | Low priority (attempt limit) |
| S8 | MEDIUM | Remote access | By design — document |
| S9 | LOW | Credential storage | By design — document |

## Recommended Priority

1. ~~**S2 + S3 + S4** — Buffer limits (all DoS vectors, straightforward fixes)~~ **Done**
2. **S5** — MXP entity size cap
3. **S1** — Document Lua risk; add plugin trust prompt
4. **S8** — Document remote access = full control
