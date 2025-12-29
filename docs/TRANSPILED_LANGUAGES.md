# Transpiled Language Support

This document tracks research and implementation status for languages that transpile to Lua.

## Important: LuaJIT Compatibility

Mushkin uses **LuaJIT** (Lua 5.1 with extensions), NOT standard Lua 5.3/5.4. This affects transpiler compatibility:

- LuaJIT is based on Lua 5.1 with some 5.2 backports
- No native integers (all numbers are doubles)
- No native bitwise operators syntax (`&`, `|`, `~`) - must use `bit` library
- No `goto` statement
- No `__pairs`/`__ipairs` metamethods

Transpilers must target **Lua 5.1** or have a LuaJIT-compatible mode.

---

## Implemented

### YueScript

**Status:** Implemented

**What:** A maintained fork of MoonScript - cleaner syntax that compiles to Lua.

**Integration:** Native Lua module (`yue.so`) provides `yue.to_lua()` for runtime transpilation.

**LuaJIT Compatible:** Yes - targets Lua 5.1+

**Repository:** https://github.com/pigpigyyy/Yuescript

---

## Researched - Not Yet Implemented

### TypeScriptToLua (TSTL)

**Status:** Researched, not implemented

**What:** Transpiles TypeScript to Lua, bringing static typing and TypeScript tooling.

**Challenge:** TSTL is an npm package requiring Node.js. It includes the full TypeScript compiler (~5MB).

**Integration Options:**
1. **Shell to Node.js** - Require `node`/`npx tstl` on user's system
2. **Embed QuickJS + TSTL** - Bundle JS engine + TSTL (large, ~5MB+)
3. **Pre-transpiled only** - Users run `tstl` externally, we load resulting Lua

**LuaJIT Compatible:** Configurable via `luaTarget` option:
```json
{
  "compilerOptions": {
    "luaTarget": "JIT"
  }
}
```

**Repository:** https://github.com/TypeScriptToLua/TypeScriptToLua

**Decision:** Deferred. Consider Option A (shell to Node.js) if there's user demand.

---

## Ready to Implement

### Teal

**Status:** Ready to implement

**What:** Typed Lua - adds static typing to Lua with minimal syntax changes. Think "TypeScript for Lua."

**Integration:** Pure Lua compiler (`tl`), installable via LuaRocks. Provides `tl.gen()` for runtime transpilation.

**LuaJIT Compatible:** Yes - explicitly supports Lua 5.1-5.4 and LuaJIT. Integer types work but with VM-dependent precision (doubles on LuaJIT).

**Size:** Small, pure Lua

**Repository:** https://github.com/teal-language/tl

**Website:** https://teal-language.org/

**Example:**
```teal
local function greet(name: string): string
   return "Hello, " .. name
end

local record Player
   name: string
   level: integer
end
```

---

### Fennel

**Status:** Ready to implement

**What:** Lisp syntax that compiles to Lua. Popular in game dev (TIC-80, LOVE2D). Zero runtime overhead.

**Integration:** Single-file Lua compiler (~3000 lines). Embeddable, provides `fennel.compileString()` for runtime transpilation. Compiles bitwise ops to use LuaJIT's `bit` library automatically.

**LuaJIT Compatible:** Yes - continuously tested against LuaJIT. Specifically handles LuaJIT's bitop library.

**Size:** Very small (~100KB single file)

**Repository:** https://github.com/bakpakin/Fennel (mirror, moving to SourceHut)

**Website:** https://fennel-lang.org/

**Example:**
```fennel
(fn greet [name]
  (.. "Hello, " name))

(local player {:name "Alice" :level 10})
```

---

### MoonScript

**What:** Original CoffeeScript-inspired Lua syntax (YueScript's predecessor).

**Status:** Less actively maintained than YueScript.

**Repository:** https://github.com/leafo/moonscript

**LuaJIT Compatible:** Yes

**Decision:** YueScript is preferred as the maintained successor.

---

### Haxe

**What:** Cross-platform language with Lua as a target.

**Challenge:** Requires Haxe compiler (separate toolchain).

**Repository:** https://github.com/HaxeFoundation/haxe

**LuaJIT Compatible:** Has Lua target, compatibility TBD

---

### Terra

**What:** Low-level language embedded in Lua, compiles to native code.

**Note:** Different use case - for performance-critical code, not scripting.

**Repository:** https://github.com/terralang/terra

---

## Selection Criteria

For a transpiled language to be a good fit for Mushkin:

1. **LuaJIT compatible** - Must target Lua 5.1 or have JIT mode
2. **Lua-based compiler** - Enables runtime transpilation without external deps
3. **Actively maintained** - Security updates, bug fixes
4. **Reasonable size** - Compiler should be embeddable
5. **Clear value add** - Typing, syntax, or tooling benefits

---

## Implementation Pattern

All transpiled languages follow the same pattern:

1. Add to `ScriptLanguage` enum in `src/automation/script_language.h`
2. Add transpile function to `ScriptEngine` (e.g., `transpileTeal()`)
3. Update `parseScript()` to handle new language
4. Load compiler module in `openLua()` if Lua-based
5. UI dropdowns automatically include new languages via enum helpers
