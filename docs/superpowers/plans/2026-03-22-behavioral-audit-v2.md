# Behavioral Audit v2 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a runtime behavioral comparison system (baseline capture plugin + diff script) and re-run the source-level audit with Opus agents and enhanced criteria.

**Architecture:** Three independent components: (1) a Lua plugin that captures all observable API behavior to JSON, (2) a Python diff script that compares two baselines and reports differences, (3) an updated `/auditloop` skill with Opus agents and new comparison criteria. Components 1 and 2 are built first since they're novel; component 3 is a skill file update.

**Tech Stack:** Lua 5.1 (plugin), Python 3 (diff script), Markdown (skill file)

---

## File Structure

| File | Purpose |
|:---|:---|
| `plugins/mushkin_audit.xml` | Baseline capture plugin (runs in both MUSHclient and Mushkin) |
| `scripts/audit_diff.py` | Diff script comparing two baseline JSON files |
| `scripts/audit_allowlist.json` | Known-acceptable differences to skip during diff |
| `.claude/skills/auditloop/SKILL.md` | Updated audit skill with Opus agents + new criteria |

---

### Task 1: Build the Baseline Capture Plugin — JSON Encoder

**Files:**
- Create: `plugins/mushkin_audit.xml`

The plugin needs a self-contained JSON encoder. Build the plugin shell with the encoder first, test it standalone.

- [ ] **Step 1: Create plugin XML skeleton with embedded JSON encoder**

Create `plugins/mushkin_audit.xml` with:
- Plugin metadata (name="Mushkin_Audit", language="Lua", save_state="n", sequence="9999")
- Alias `audit` triggering `run_audit()`
- Self-contained `json_encode(value)` function that handles: strings (with escaping), numbers, booleans, nil (as JSON null), tables (sorted keys at all nesting levels, detect array vs object)
- Test function `test_json()` that validates encoding of edge cases
- Stub `run_audit()` that calls `test_json()` and prints "Audit plugin loaded"

```lua
-- JSON encoder (~60 lines, self-contained)
local function json_encode(val, indent, current_indent)
  indent = indent or "  "
  current_indent = current_indent or ""
  local next_indent = current_indent .. indent
  local t = type(val)
  if val == nil then return "null"
  elseif t == "boolean" then return tostring(val)
  elseif t == "number" then
    if val ~= val then return "null" end -- NaN
    if val == math.huge or val == -math.huge then return "null" end
    return tostring(val)
  elseif t == "string" then
    return '"' .. val:gsub('[\\"\b\f\n\r\t]', {
      ['\\']='\\\\', ['"']='\\"', ['\b']='\\b', ['\f']='\\f',
      ['\n']='\\n', ['\r']='\\r', ['\t']='\\t'
    }):gsub('[%z\1-\31]', function(c) return string.format('\\u%04x', string.byte(c)) end) .. '"'
  elseif t == "table" then
    -- Check if array (consecutive integer keys starting at 1)
    local is_array = true
    local max_n = 0
    for k, _ in pairs(val) do
      if type(k) ~= "number" or k ~= math.floor(k) or k < 1 then
        is_array = false; break
      end
      if k > max_n then max_n = k end
    end
    if max_n == 0 then is_array = false end
    if is_array then
      -- Verify no holes
      for i = 1, max_n do
        if val[i] == nil then is_array = false; break end
      end
    end
    if is_array then
      local parts = {}
      for i = 1, max_n do
        parts[i] = next_indent .. json_encode(val[i], indent, next_indent)
      end
      return "[\n" .. table.concat(parts, ",\n") .. "\n" .. current_indent .. "]"
    else
      -- Object with sorted keys (preserve original key types to handle false values)
      local raw_keys = {}
      for k in pairs(val) do raw_keys[#raw_keys+1] = k end
      table.sort(raw_keys, function(a, b) return tostring(a) < tostring(b) end)
      local parts = {}
      for _, k in ipairs(raw_keys) do
        local v = val[k]  -- direct lookup, no `or` (handles false correctly)
        parts[#parts+1] = next_indent .. json_encode(tostring(k), indent, next_indent) .. ": " .. json_encode(v, indent, next_indent)
      end
      return "{\n" .. table.concat(parts, ",\n") .. "\n" .. current_indent .. "}"
    end
  else
    return '"[unsupported: ' .. t .. ']"'
  end
end
```

- [ ] **Step 2: Verify the plugin loads in Mushkin**

Copy plugin to deployed Mushkin plugins folder, launch, confirm "Audit plugin loaded" appears and `test_json()` passes.

- [ ] **Step 3: Commit**

```bash
git add plugins/mushkin_audit.xml
git commit -m "feat: add baseline audit plugin skeleton with JSON encoder"
```

---

### Task 2: Build the Baseline Capture Plugin — Data Collection

**Files:**
- Modify: `plugins/mushkin_audit.xml`

Add all data collection functions. Each category is a function that returns a table.

- [ ] **Step 1: Add metadata collection**

```lua
local function collect_metadata()
  -- Compute world file hash for baseline identity verification
  local world_file_hash = ""
  local world_path = GetAlphaOption("name") -- world name for identification
  local app_dir = GetInfo(66) or ""
  local wf = io.open(GetInfo(70) or "", "rb") -- GetInfo(70) = world file pathname
  if wf then
    local content = wf:read("*a")
    wf:close()
    world_file_hash = Hash(content) -- MD5 hash via MUSHclient API
  end
  return {
    capture_date = os.date("%Y-%m-%d %H:%M:%S"),
    client_name = (string.find(Version() or "", "mushkin", 1, true)) and "Mushkin" or "MUSHclient",
    client_version = Version(),
    world_name = WorldName(),
    plugin_count = #GetPluginList(),
    world_file_hash = world_file_hash,
  }
end
```

- [ ] **Step 2: Add GetInfo collection (codes 1-310)**

```lua
local function collect_getinfo()
  local results = {}
  for i = 1, 310 do
    local ok, val = pcall(GetInfo, i)
    if ok then
      results[tostring(i)] = val
    else
      results[tostring(i)] = "__error__: " .. tostring(val)
    end
  end
  return results
end
```

- [ ] **Step 3: Add GetOption/GetAlphaOption collection**

Embed the complete option name lists from `config_options.cpp`. Call `GetOption(name)` for numeric options and `GetAlphaOption(name)` for string options.

```lua
local numeric_options = {
  "alternative_inverse", "alt_arrow_recalls_partial", "always_record_command_history",
  "append_to_log_file", "arrow_keys_wrap", "arrow_recalls_partial",
  -- ... (all 170 numeric options from config_options.cpp OptionsTable, lines 97-312)
  "wrap_column", "wrap_input", "write_world_name_to_log"
}

local alpha_options = {
  "auto_log_file_name", "auto_say_override_prefix", "auto_say_string",
  -- ... (all 69 string options from config_options.cpp AlphaOptionsTable, lines 320-396)
  "timestamp_input", "timestamp_notes", "timestamp_output"
}

local function collect_options()
  local results = { numeric = {}, alpha = {} }
  for _, name in ipairs(numeric_options) do
    local ok, val = pcall(GetOption, name)
    results.numeric[name] = ok and val or ("__error__: " .. tostring(val))
  end
  for _, name in ipairs(alpha_options) do
    local ok, val = pcall(GetAlphaOption, name)
    results.alpha[name] = ok and val or ("__error__: " .. tostring(val))
  end
  return results
end
```

Extract the complete option name lists by running:
```bash
grep -o '"[a-z_]*"' src/world/config_options.cpp | sort -u
```
Cross-reference with lines 97-312 (OptionsTable, 170 numeric) and 320-396 (AlphaOptionsTable, 69 alpha) in `config_options.cpp`. Embed ALL names in the plugin.

- [ ] **Step 4: Add plugin enumeration**

```lua
local function collect_plugins()
  local results = {}
  local plugins = GetPluginList()
  for _, id in ipairs(plugins) do
    local info = {}
    for code = 1, 28 do
      local ok, val = pcall(GetPluginInfo, id, code)
      info[tostring(code)] = ok and val or ("__error__: " .. tostring(val))
    end
    -- Also collect plugin variables
    -- GetPluginVariableList returns an array-like table of variable names
    local ok_vl, var_list = pcall(GetPluginVariableList, id)
    if ok_vl and var_list then
      local vars = {}
      for _, vname in pairs(var_list) do  -- use pairs (return type may vary)
        vars[vname] = GetPluginVariable(id, vname)
      end
      info.variables = vars
    end
    results[id] = info
  end
  return results
end
```

- [ ] **Step 5: Add miniwindow and font metrics collection**

```lua
local function collect_miniwindows()
  local results = {}
  local windows = WindowList() or {}
  for _, win in ipairs(windows) do
    local info = {}
    for code = 1, 18 do
      local ok, val = pcall(WindowInfo, win, code)
      info[tostring(code)] = ok and val or ("__error__: " .. tostring(val))
    end
    -- Font metrics
    local fonts = WindowFontList(win) or {}
    local font_info = {}
    for _, fid in ipairs(fonts) do
      local fmetrics = {}
      for fcode = 1, 20 do
        local ok, val = pcall(WindowFontInfo, win, fid, fcode)
        fmetrics[tostring(fcode)] = ok and val or ("__error__: " .. tostring(val))
      end
      font_info[fid] = fmetrics
    end
    info.fonts = font_info
    results[win] = info
  end
  return results
end
```

- [ ] **Step 6: Add Lua API availability check**

```lua
local api_functions = {
  "Accelerator", "AcceleratorList", "AcceleratorTo", "Activate",
  -- ... (all 408 registered API function names)
  "WorldName", "WorldPort", "WriteLog"
}

local function collect_api_availability()
  local results = {}
  for _, name in ipairs(api_functions) do
    local val = _G[name]
    results[name] = val ~= nil and type(val) or "missing"
  end
  return results
end
```

Extract the function list from `lua_registration.cpp`:
- `worldlib` table (lines 529-1009): 408 functions registered as `world.FunctionName`
- Global aliases (lines 1042-1198): functions also registered as bare globals (e.g., `Note`, `print`, `Trim`, `Hash`, `SaveState`)
Check both `worldlib` entries AND separately-registered globals. Use:
```bash
grep -oP '{"(\w+)"' src/world/lua_api/lua_registration.cpp | sort -u
grep 'lua_setglobal.*"' src/world/lua_api/lua_registration.cpp | grep -oP '"(\w+)"' | sort -u
```

- [ ] **Step 7: Add Lua environment and error-path probes**

```lua
local function collect_lua_environment()
  return {
    lua_version = _VERSION,
    has_jit = (jit ~= nil),
    has_bit = (bit ~= nil),
    has_bit32 = (bit32 ~= nil),
    has_utf8 = (utf8 ~= nil),
    package_path = package.path,
    package_cpath = package.cpath,
    loaders_count = package.loaders and #package.loaders or 0,
    loaders_type = type(package.loaders),
    searchers_type = type(package.searchers),
  }
end

local function collect_error_probes()
  local results = {}
  -- Unknown option
  local ok, val = pcall(GetOption, "nonexistent_option_xyzzy")
  results["GetOption_unknown"] = ok and val or ("__error__: " .. tostring(val))
  -- Invalid GetInfo code
  ok, val = pcall(GetInfo, 99999)
  results["GetInfo_invalid"] = ok and val or ("__error__: " .. tostring(val))
  -- GetVariable for nonexistent
  ok, val = pcall(GetVariable, "nonexistent_variable_xyzzy")
  results["GetVariable_unknown"] = ok and val or ("__error__: " .. tostring(val))
  -- WindowInfo for nonexistent window
  ok, val = pcall(WindowInfo, "nonexistent_window_xyzzy", 1)
  results["WindowInfo_unknown"] = ok and val or ("__error__: " .. tostring(val))
  return results
end

local function collect_error_codes()
  local results = {}
  if error_code then
    for k, v in pairs(error_code) do
      results[k] = v
    end
  end
  return results
end
```

- [ ] **Step 8: Wire up `run_audit()` to collect all data and write JSON**

```lua
function run_audit()
  local t0 = os.clock()
  local data = {
    metadata = collect_metadata(),
    getinfo = collect_getinfo(),
    options = collect_options(),
    plugins = collect_plugins(),
    miniwindows = collect_miniwindows(),
    api_availability = collect_api_availability(),
    lua_environment = collect_lua_environment(),
    error_probes = collect_error_probes(),
    error_codes = collect_error_codes(),
  }
  local json = json_encode(data)
  local path = GetInfo(66) .. "audit_baseline.json"
  local f, err = io.open(path, "w")
  if not f then
    ColourNote("white", "red", "AUDIT ERROR: Cannot write to " .. path .. ": " .. tostring(err))
    return
  end
  f:write(json)
  f:close()
  local elapsed = os.clock() - t0
  -- Count items
  local count = 0
  for cat, tbl in pairs(data) do
    if type(tbl) == "table" then
      for _ in pairs(tbl) do count = count + 1 end
    end
  end
  ColourNote("lime", "black", string.format("AUDIT COMPLETE: %d items captured in %.2fs", count, elapsed))
  ColourNote("lime", "black", "Written to: " .. path)
end
```

- [ ] **Step 9: Test in Mushkin — type `audit`, verify JSON output**

Launch Mushkin with Aardwolf world, type `audit`. Check:
- `audit_baseline.json` is created
- JSON is valid (run `python3 -m json.tool audit_baseline.json > /dev/null`)
- Contains all expected categories
- No `__error__` entries for known-working APIs

- [ ] **Step 10: Commit**

```bash
git add plugins/mushkin_audit.xml
git commit -m "feat: add comprehensive data collection to audit plugin"
```

---

### Task 3: Build the Diff Script

**Files:**
- Create: `scripts/audit_diff.py`
- Create: `scripts/audit_allowlist.json`

- [ ] **Step 1: Create the allowlist file**

```json
{
  "skip_keys": [
    "metadata.capture_date",
    "metadata.client_version",
    "metadata.client_name",
    "getinfo.1",
    "getinfo.35",
    "getinfo.36",
    "getinfo.56",
    "getinfo.57",
    "getinfo.58",
    "getinfo.59",
    "getinfo.66",
    "getinfo.67",
    "getinfo.68",
    "getinfo.69",
    "getinfo.70",
    "getinfo.71",
    "getinfo.227",
    "getinfo.228",
    "getinfo.229",
    "getinfo.230",
    "getinfo.231",
    "getinfo.232",
    "getinfo.233",
    "getinfo.234"
  ],
  "normalize_paths": [
    "getinfo.*",
    "options.alpha.*",
    "plugins.*.variables.*"
  ],
  "tolerance": {
    "miniwindows.*.1": 5,
    "miniwindows.*.2": 5,
    "miniwindows.*.10": 10,
    "miniwindows.*.11": 10,
    "miniwindows.*.12": 10,
    "miniwindows.*.13": 10
  },
  "skip_key_comments": {
    "getinfo.1": "Server address (may differ)",
    "getinfo.35-36": "Current time values",
    "getinfo.56-59": "Date/time formats",
    "getinfo.66-71": "App/world paths (platform-specific)",
    "getinfo.227-234": "Uptime/connection duration"
  }
}
```

The `skip_keys` list will need to be expanded during initial runs as platform-specific differences are identified. Start conservative.

- [ ] **Step 2: Create the diff script**

Create `scripts/audit_diff.py`:

```python
#!/usr/bin/env python3
"""Compare two MUSHclient/Mushkin audit baseline JSON files."""

import json
import sys
import re
import os
from pathlib import Path
from collections import defaultdict

def load_json(path):
    with open(path) as f:
        return json.load(f)

def load_allowlist(path):
    if not os.path.exists(path):
        return {"skip_keys": [], "normalize_paths": [], "tolerance": {}}
    with open(path) as f:
        return json.load(f)

def normalize_path_value(val):
    """Normalize path separators and strip drive letters."""
    if not isinstance(val, str):
        return val
    val = val.replace("\\", "/")
    # Strip Windows drive letters (C:/ -> /)
    val = re.sub(r'^[A-Za-z]:/', '/', val)
    return val

def should_skip(key, allowlist):
    return key in allowlist.get("skip_keys", [])

def should_normalize_path(key, allowlist):
    for pattern in allowlist.get("normalize_paths", []):
        if re.match(pattern.replace("*", ".*"), key):
            return True
    return False

def get_tolerance(key, allowlist):
    for pattern, tol in allowlist.get("tolerance", {}).items():
        if re.match(pattern.replace("*", ".*"), key):
            return tol
    return None

def flatten(d, prefix=""):
    """Flatten nested dict to dot-separated keys."""
    items = {}
    for k, v in d.items():
        key = f"{prefix}.{k}" if prefix else k
        if isinstance(v, dict):
            items.update(flatten(v, key))
        else:
            items[key] = v
    return items

def diff_baselines(original, mushkin, allowlist):
    flat_orig = flatten(original)
    flat_mush = flatten(mushkin)
    all_keys = set(flat_orig.keys()) | set(flat_mush.keys())

    findings = defaultdict(list)
    stats = {"match": 0, "mismatch": 0, "missing": 0, "extra": 0, "skipped": 0}

    for key in sorted(all_keys):
        if should_skip(key, allowlist):
            stats["skipped"] += 1
            continue

        in_orig = key in flat_orig
        in_mush = key in flat_mush

        if in_orig and not in_mush:
            category = key.split(".")[0]
            findings[category].append(("MISSING", key, flat_orig[key], None))
            stats["missing"] += 1
        elif in_mush and not in_orig:
            category = key.split(".")[0]
            findings[category].append(("EXTRA", key, None, flat_mush[key]))
            stats["extra"] += 1
        else:
            orig_val = flat_orig[key]
            mush_val = flat_mush[key]

            # Path normalization
            if should_normalize_path(key, allowlist):
                orig_val = normalize_path_value(orig_val)
                mush_val = normalize_path_value(mush_val)

            # Tolerance check
            tol = get_tolerance(key, allowlist)
            if tol and isinstance(orig_val, (int, float)) and isinstance(mush_val, (int, float)):
                if abs(orig_val - mush_val) <= tol:
                    stats["match"] += 1
                    continue

            if orig_val == mush_val:
                stats["match"] += 1
            else:
                category = key.split(".")[0]
                findings[category].append(("MISMATCH", key, orig_val, mush_val))
                stats["mismatch"] += 1

    return findings, stats

def print_report(findings, stats):
    print("=" * 70)
    print("BEHAVIORAL AUDIT v2 — BASELINE COMPARISON REPORT")
    print("=" * 70)
    print()
    print(f"  Matched:  {stats['match']}")
    print(f"  Mismatch: {stats['mismatch']}")
    print(f"  Missing:  {stats['missing']}")
    print(f"  Extra:    {stats['extra']}")
    print(f"  Skipped:  {stats['skipped']}")
    print()

    if not findings:
        print("No unexpected differences found.")
        return

    for category in sorted(findings.keys()):
        items = findings[category]
        print(f"--- {category} ({len(items)} differences) ---")
        for kind, key, orig, mush in items:
            if kind == "MISMATCH":
                print(f"  MISMATCH {key}")
                print(f"    original: {repr(orig)}")
                print(f"    mushkin:  {repr(mush)}")
            elif kind == "MISSING":
                print(f"  MISSING  {key} = {repr(orig)}")
            elif kind == "EXTRA":
                print(f"  EXTRA    {key} = {repr(mush)}")
        print()

def write_findings(findings, output_path):
    with open(output_path, "w") as f:
        f.write("# Audit v2 Runtime Findings\n\n")
        f.write(f"Generated: {__import__('datetime').datetime.now().isoformat()}\n\n")
        f.write("Triage these into `docs/behavioral_worklist.md` with appropriate severity.\n\n")
        for category in sorted(findings.keys()):
            f.write(f"## {category}\n\n")
            for kind, key, orig, mush in findings[category]:
                if kind == "MISMATCH":
                    f.write(f"- **MISMATCH** `{key}`: original=`{orig}` mushkin=`{mush}`\n")
                elif kind == "MISSING":
                    f.write(f"- **MISSING** `{key}`: original=`{orig}`\n")
                elif kind == "EXTRA":
                    f.write(f"- **EXTRA** `{key}`: mushkin=`{mush}`\n")
            f.write("\n")

def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <original.json> <mushkin.json> [--findings PATH]")
        sys.exit(1)

    orig_path = sys.argv[1]
    mush_path = sys.argv[2]
    findings_path = "docs/audit_v2_runtime_findings.md"
    for i, arg in enumerate(sys.argv):
        if arg == "--findings" and i + 1 < len(sys.argv):
            findings_path = sys.argv[i + 1]

    allowlist_path = os.path.join(os.path.dirname(__file__), "audit_allowlist.json")

    original = load_json(orig_path)
    mushkin = load_json(mush_path)
    allowlist = load_allowlist(allowlist_path)

    # Metadata validation
    orig_meta = original.get("metadata", {})
    mush_meta = mushkin.get("metadata", {})
    if orig_meta.get("world_name") != mush_meta.get("world_name"):
        print(f"WARNING: World names differ: {orig_meta.get('world_name')} vs {mush_meta.get('world_name')}")
    orig_count = orig_meta.get("plugin_count", 0)
    mush_count = mush_meta.get("plugin_count", 0)
    if abs(orig_count - mush_count) > 5:
        print(f"WARNING: Plugin counts differ significantly: {orig_count} vs {mush_count}")

    findings, stats = diff_baselines(original, mushkin, allowlist)
    print_report(findings, stats)

    if findings:
        write_findings(findings, findings_path)
        print(f"Findings written to: {findings_path}")

    sys.exit(0 if not findings else 1)

if __name__ == "__main__":
    main()
```

- [ ] **Step 3: Test with synthetic data**

Create two small test JSON files, run the diff script, verify output format and exit codes.

- [ ] **Step 4: Commit**

```bash
git add scripts/audit_diff.py scripts/audit_allowlist.json
git commit -m "feat: add audit baseline diff script and allowlist"
```

---

### Task 4: Update `/auditloop` Skill

**Files:**
- Modify: `.claude/skills/auditloop/SKILL.md`

- [ ] **Step 1: Reset all subsystem `[audited]` markers to `[unaudited]`**

- [ ] **Step 2: Change agent model from Sonnet to Opus**

In the Pipeline section, replace "Spawn a **Sonnet subagent**" with "Spawn an **Opus subagent**" and add `model: "opus"` to the Agent tool invocation.

- [ ] **Step 3: Add new comparison criteria to agent prompt**

Append to the agent prompt template:

```markdown
## Additional v2 Criteria (check ALL of these in addition to behavioral comparison)

### Init Ordering
- Verify operations happen in the correct sequence
- LoadState must run before parseScript (plugin state available to script body)
- Default fonts/settings must only apply to new worlds, not loaded ones
- Package loaders must be saved before language modules modify them

### Path Handling
- All path strings must go through backslash normalization before use
- Check: preferences database paths, world file paths, plugin XML paths, Lua API path returns
- resolveDirectory() must normalize backslashes

### DPI Assumptions
- Font sizes must use 96 DPI conversion (pixelSize = pointSize * 96/72)
- No direct calls to setPointSize() or setPointSizeF() for MUSHclient-compatible fonts
- Check both main output fonts and miniwindow fonts

### State Round-Trips
- SaveState and LoadState must use the same resolved path
- GetVariable must return what SetVariable stored
- OnPluginSaveState must fire before variables are written to XML
```

- [ ] **Step 4: Commit**

```bash
git add .claude/skills/auditloop/SKILL.md
git commit -m "feat: update auditloop skill for v2 (Opus agents, new criteria, reset queue)"
```

---

### Task 5: End-to-End Validation

**Files:**
- No new files

- [ ] **Step 1: Install audit plugin in Wine MUSHclient**

Copy `plugins/mushkin_audit.xml` to `~/.wine/drive_c/Program Files/MUSHclient/worlds/plugins/`

- [ ] **Step 2: Capture Wine baseline**

Launch Wine MUSHclient, load Aardwolf world, wait for all plugins to load, type `audit`. Copy the resulting `audit_baseline.json` to `audit_baseline_original.json` in the repo root.

- [ ] **Step 3: Capture Mushkin baseline**

Deploy latest Mushkin build, copy world files from Wine, launch, wait for all plugins to load, type `audit`. Copy result to `audit_baseline_mushkin.json`.

- [ ] **Step 4: Run diff**

```bash
python3 scripts/audit_diff.py audit_baseline_original.json audit_baseline_mushkin.json
```

Review the report. Expand `scripts/audit_allowlist.json` for any legitimate platform differences. Re-run until false positives are suppressed.

- [ ] **Step 5: Triage findings**

Review `docs/audit_v2_runtime_findings.md`. For each finding, assign severity (HIGH/MEDIUM/LOW) and add to `docs/behavioral_worklist.md`.

- [ ] **Step 6: Run source re-audit**

```
/auditloop
```

Repeat until all 16 subsystems are audited. New findings are appended to the worklist automatically.

- [ ] **Step 7: Begin fix cycle**

```
/loop 5m /fixloop
```

Process worklist items. Periodically re-run the Mushkin baseline capture and diff to verify convergence.
