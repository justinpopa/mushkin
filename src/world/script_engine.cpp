#include "script_engine.h"
#include "../automation/plugin.h"
#include "../world/world_document.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QRegularExpression>
#include <QStringList>

#include "color_utils.h"

// Lua headers
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

// Lua C modules
int luaopen_lsqlite3(lua_State* L); // SQLite3 bindings for Lua
int luaopen_bit(lua_State* L);      // Bit manipulation library (LuaJIT)
int luaopen_progress(lua_State* L); // Progress dialog library
int luaopen_lpeg(lua_State* L);     // LPeg pattern matching library

// ========== FFI Shims for Cross-Platform Compatibility ==========

/**
 * CreateDirectoryA - Windows API compatibility shim for LuaJIT FFI
 *
 * Provides cross-platform directory creation for plugins using FFI to call
 * Windows CreateDirectoryA. This function is exported so LuaJIT's FFI can
 * find it via dlsym(RTLD_DEFAULT, "CreateDirectoryA").
 *
 * @param lpPathName - Path to directory to create (Windows or Unix format)
 * @param lpSecurityAttributes - Security attributes (ignored, Windows-only)
 * @return true on success, false on failure
 */
bool CreateDirectoryA(const char* lpPathName, void* lpSecurityAttributes)
{
    // Ignore security attributes (Windows-specific)
    (void)lpSecurityAttributes;

    if (!lpPathName || !lpPathName[0]) {
        return false; // Invalid path
    }

    // Convert path to QString and normalize (handles both \ and /)
    QString path = QString::fromUtf8(lpPathName);
    path = QDir::fromNativeSeparators(path);

    // Use QDir::mkpath which creates all parent directories if needed
    // (equivalent to Windows CreateDirectory behavior)
    QDir dir;
    bool success = dir.mkpath(path);

    if (!success) {
        qWarning() << "CreateDirectoryA: Failed to create directory:" << path;
    }

    return success;
}

/**
 * GetLastError - Windows API compatibility shim for LuaJIT FFI
 *
 * Returns the last Windows API error code. Since we're cross-platform and
 * handle errors directly in our shim functions (like CreateDirectoryA),
 * this simply returns 0 (ERROR_SUCCESS).
 *
 * Note: Only defined on non-Windows platforms. On Windows, the real
 * GetLastError from kernel32 is used.
 *
 * @return 0 (success)
 */
#ifndef _WIN32
unsigned long GetLastError(void)
{
    return 0; // ERROR_SUCCESS - no error
}
#endif

/**
 * CopyFileA - Windows API compatibility shim for LuaJIT FFI
 *
 * Provides cross-platform file copying for plugins using FFI to call
 * Windows CopyFileA.
 *
 * @param lpExistingFileName - Source file path
 * @param lpNewFileName - Destination file path
 * @param bFailIfExists - If true, fail if destination exists
 * @return true on success, false on failure
 */
bool CopyFileA(const char* lpExistingFileName, const char* lpNewFileName, bool bFailIfExists)
{
    if (!lpExistingFileName || !lpExistingFileName[0] || !lpNewFileName || !lpNewFileName[0]) {
        return false; // Invalid paths
    }

    // Convert paths to QString and normalize
    QString sourcePath = QString::fromUtf8(lpExistingFileName);
    QString destPath = QString::fromUtf8(lpNewFileName);
    sourcePath = QDir::fromNativeSeparators(sourcePath);
    destPath = QDir::fromNativeSeparators(destPath);

    // Check if source exists
    if (!QFile::exists(sourcePath)) {
        qWarning() << "CopyFileA: Source file does not exist:" << sourcePath;
        return false;
    }

    // Check if destination exists and bFailIfExists is true
    if (bFailIfExists && QFile::exists(destPath)) {
        qWarning() << "CopyFileA: Destination already exists:" << destPath;
        return false;
    }

    // Remove destination if it exists and we're allowed to overwrite
    if (!bFailIfExists && QFile::exists(destPath)) {
        if (!QFile::remove(destPath)) {
            qWarning() << "CopyFileA: Failed to remove existing destination:" << destPath;
            return false;
        }
    }

    // Copy the file
    bool success = QFile::copy(sourcePath, destPath);

    if (!success) {
        qWarning() << "CopyFileA: Failed to copy from" << sourcePath << "to" << destPath;
    }

    return success;
}

} // extern "C"

/**
 * ScriptEngine Constructor
 *
 * Creates a script engine for the given world document.
 * Does not create Lua state yet - call createScriptEngine() for that.
 */
ScriptEngine::ScriptEngine(WorldDocument* doc, const QString& language, QObject* parent)
    : QObject(parent), L(nullptr), m_doc(doc), m_plugin(nullptr), m_language(language)
{
}

/**
 * ScriptEngine Destructor
 *
 * Automatically closes Lua state if still open.
 */
ScriptEngine::~ScriptEngine()
{
    disableScripting();
}

/**
 * createScriptEngine - Initialize scripting
 *
 * Creates and initializes the Lua state.
 *
 * @return true if successful, false on error
 */
bool ScriptEngine::createScriptEngine()
{
    openLua();
    return L != nullptr;
}

/**
 * disableScripting - Shutdown scripting
 *
 * Closes the Lua state and frees all resources.
 */
void ScriptEngine::disableScripting()
{
    closeLua();
}

/**
 * setPlugin - Set plugin pointer for this script engine
 *
 * Stores the plugin pointer in both the member variable and the Lua registry.
 * This allows API functions to retrieve the correct plugin context.
 */
void ScriptEngine::setPlugin(Plugin* plugin)
{
    m_plugin = plugin;

    // Store plugin pointer in Lua registry (if Lua state exists)
    if (L && plugin) {
        lua_pushlightuserdata(L, (void*)plugin);
        lua_setfield(L, LUA_REGISTRYINDEX, PLUGIN_STATE);
    }
}

/**
 * openLua - Create and initialize Lua state
 *
 * Initialize Lua State and Core Libraries
 *
 * Based on CScriptEngine::OpenLua() from lua_scripting.cpp
 *
 * Creates a new Lua state, loads all standard libraries, and stores
 * the document pointer in the Lua registry for access by API functions.
 *
 * Steps:
 * 1. Create new Lua state with luaL_newstate()
 * 2. Load standard libraries with luaL_openlibs()
 * 3. Store document pointer in registry under DOCUMENT_STATE key
 * 4. Clear stack
 *
 * In later stories (4.2, 4.3), we'll add:
 * - RegisterLuaRoutines() - API function registration
 * - Additional libraries (rex, bits, compress, etc.)
 * - Sandbox initialization
 */
void ScriptEngine::openLua()
{
    // Don't create if already exists
    if (L) {
        qWarning() << "Lua state already exists";
        return;
    }

    // 1. Create new Lua state
    L = luaL_newstate();
    if (!L) {
        qCritical() << "Failed to create Lua state (out of memory?)";
        return;
    }

    // 2. Open all standard Lua libraries
    // This opens: base, package, string, table, math, io, os, debug, coroutine
    luaL_openlibs(L);

    // 3. Store world document pointer in registry
    // This allows Lua C functions to retrieve the document pointer
    // Key: DOCUMENT_STATE = "mushclient.document"
    // Value: lightuserdata (void* pointer to m_doc)
    lua_pushlightuserdata(L, (void*)m_doc);
    lua_setfield(L, LUA_REGISTRYINDEX, DOCUMENT_STATE);

    // 4. Register MUSHclient API functions
    RegisterLuaRoutines(L);

    // 5. Set up Lua package.path (issue #4)
    // Use only relative paths for portability - no system paths
    // MUSHclient also includes absolute exe_dir paths (the ! notation in luaconf.h),
    // but we intentionally omit those to avoid the portability issues they cause
    lua_getglobal(L, "package");

    QStringList luaPaths = {
        "./?.lua",
        "./lua/?.lua",
        "./lua/?/init.lua",
    };

    // For plugins, add the plugin directory paths
    // This allows require() to find Lua files in:
    // 1. Plugin's own directory and subdirectories
    // 2. Paths relative to current working directory
    Plugin* plugin = qobject_cast<Plugin*>(parent());
    if (plugin && !plugin->m_strDirectory.isEmpty()) {
        QString pluginDir = plugin->m_strDirectory;

        // Add plugin directory
        luaPaths << pluginDir + "/?.lua";
        luaPaths << pluginDir + "/?/init.lua";

        // Add plugin's lua subdirectory
        luaPaths << pluginDir + "/lua/?.lua";
        luaPaths << pluginDir + "/lua/?/init.lua";
    }

    QString packagePath = luaPaths.join(";");
    lua_pushstring(L, packagePath.toUtf8().constData());
    lua_setfield(L, -2, "path");
    lua_pop(L, 1); // pop package table

    // 5b. Set up Lua package.cpath for compiled C modules (issue #4)
    // We replace the default cpath entirely - no system paths
    // App bundle paths are needed for bundled modules like LuaSocket
    lua_getglobal(L, "package");

    QString appDir = QCoreApplication::applicationDirPath();

#ifdef Q_OS_WIN
    QString libExt = "dll";
#else
    QString libExt = "so";
#endif

    QStringList luaCPaths = {
        // App bundle paths (for bundled C modules like LuaSocket)
        appDir + "/lib/?." + libExt,
        appDir + "/lib/?/core." + libExt,
        appDir + "/lua/?." + libExt,
        appDir + "/lua/?/core." + libExt,
        // Relative paths (for user C modules)
        "./lib/?." + libExt,
        "./lib/?/core." + libExt,
        "./lua/?." + libExt,
        "./lua/?/core." + libExt,
        "./?." + libExt,
    };

    QString packageCPath = luaCPaths.join(";");
    lua_pushstring(L, packageCPath.toUtf8().constData());
    lua_setfield(L, -2, "cpath");
    lua_pop(L, 1); // pop package table

    // 6. Load additional Lua C modules
    // Load lsqlite3 (SQLite3 database bindings)
    // Call luaopen_lsqlite3() and register the returned table as global "sqlite3"
    lua_pushcfunction(L, luaopen_lsqlite3);
    lua_call(L, 0, 1);           // Call with 0 args, expect 1 result (the sqlite3 table)
    lua_setglobal(L, "sqlite3"); // Register as global "sqlite3"

    // Load bit library (bitwise operations - LuaJIT built-in)
    lua_pushcfunction(L, luaopen_bit);
    lua_call(L, 0, 1);       // Call with 0 args, expect 1 result (the bit table)
    lua_setglobal(L, "bit"); // Register as global "bit"

    // Add compatibility shims for original MUSHclient bit library names
    // LuaJIT uses different names than the original MUSHclient bit library
    const char* bit_compat_code = R"(
        -- Original MUSHclient bit library compatibility shims
        -- Map original function names to LuaJIT bit library equivalents
        bit.ashr = bit.arshift  -- arithmetic shift right
        bit.neg = bit.bnot      -- bitwise NOT
        bit.shl = bit.lshift    -- shift left
        bit.shr = bit.rshift    -- logical shift right
        bit.xor = bit.bxor      -- bitwise XOR

        -- Additional functions from original MUSHclient
        -- bit.test(value, mask1, ...) - test if bits are set
        bit.test = function(value, ...)
            local mask = 0
            for i = 1, select('#', ...) do
                mask = bit.bor(mask, select(i, ...))
            end
            return bit.band(value, mask) == mask
        end

        -- bit.clear(value, mask1, ...) - clear specified bits
        bit.clear = function(value, ...)
            for i = 1, select('#', ...) do
                value = bit.band(value, bit.bnot(select(i, ...)))
            end
            return value
        end

        -- bit.mod(a, b) - modulo operation
        bit.mod = function(a, b)
            return a % b
        end

        -- bit.tonumber(str, base) - convert string to number in any base (2-36)
        bit.tonumber = function(str, base)
            return tonumber(str, base or 10)
        end

        -- bit.tostring(num, base) - convert number to string in any base (2-36)
        bit.tostring = function(num, base)
            base = base or 10
            if base == 10 then
                return tostring(num)
            elseif base == 16 then
                return string.format("%X", num)
            elseif base == 8 then
                return string.format("%o", num)
            elseif base == 2 then
                local result = ""
                local n = math.floor(num)
                if n == 0 then return "0" end
                while n > 0 do
                    result = (n % 2) .. result
                    n = math.floor(n / 2)
                end
                return result
            else
                -- General base conversion for bases 2-36
                local digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                local result = ""
                local n = math.floor(num)
                if n == 0 then return "0" end
                local negative = n < 0
                n = math.abs(n)
                while n > 0 do
                    local digit = (n % base) + 1
                    result = digits:sub(digit, digit) .. result
                    n = math.floor(n / base)
                end
                return negative and ("-" .. result) or result
            end
        end
    )";

    if (luaL_dostring(L, bit_compat_code) != 0) {
        qWarning() << "Failed to load bit library compatibility shims:" << lua_tostring(L, -1);
        lua_pop(L, 1);
    }

    // Load progress library (progress dialogs for long-running operations)
    lua_pushcfunction(L, luaopen_progress);
    lua_call(L, 0, 1);            // Call with 0 args, expect 1 result (the progress table)
    lua_setglobal(L, "progress"); // Register as global "progress"

    // Load lpeg library (Parsing Expression Grammars for Lua)
    lua_pushcfunction(L, luaopen_lpeg);
    lua_call(L, 0, 1); // Call with 0 args, expect 1 result (the lpeg table)
    // Register as both global and in package.loaded (so require("lpeg") works)
    lua_pushvalue(L, -1);     // duplicate the lpeg table
    lua_setglobal(L, "lpeg"); // Register as global "lpeg"
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "loaded");
    lua_pushvalue(L, -3);        // push lpeg table again
    lua_setfield(L, -2, "lpeg"); // package.loaded["lpeg"] = lpeg
    lua_pop(L, 3);               // pop loaded, package, and lpeg

    // Load re.lua (regex-like interface for lpeg) - embedded as string literal
    // re.lua is part of LPeg and provides a regex-like interface on top of lpeg
    // Copyright 2007-2023, Lua.org & PUC-Rio - written by Roberto Ierusalimschy
    const char* reLuaCode = R"LUA(
local tonumber, type, print, error = tonumber, type, print, error
local setmetatable = setmetatable
local m = require"lpeg"
local mm = m
local mt = getmetatable(mm.P(0))
local version = _VERSION
_ENV = nil
local any = m.P(1)
local Predef = { nl = m.P"\n" }
local mem
local fmem
local gmem
local function updatelocale ()
  mm.locale(Predef)
  Predef.a = Predef.alpha
  Predef.c = Predef.cntrl
  Predef.d = Predef.digit
  Predef.g = Predef.graph
  Predef.l = Predef.lower
  Predef.p = Predef.punct
  Predef.s = Predef.space
  Predef.u = Predef.upper
  Predef.w = Predef.alnum
  Predef.x = Predef.xdigit
  Predef.A = any - Predef.a
  Predef.C = any - Predef.c
  Predef.D = any - Predef.d
  Predef.G = any - Predef.g
  Predef.L = any - Predef.l
  Predef.P = any - Predef.p
  Predef.S = any - Predef.s
  Predef.U = any - Predef.u
  Predef.W = any - Predef.w
  Predef.X = any - Predef.x
  mem = {}
  fmem = {}
  gmem = {}
  local mt = {__mode = "v"}
  setmetatable(mem, mt)
  setmetatable(fmem, mt)
  setmetatable(gmem, mt)
end
updatelocale()
local I = m.P(function (s,i) print(i, s:sub(1, i-1)); return i end)
local function patt_error (s, i)
  local msg = (#s < i + 20) and s:sub(i) or s:sub(i,i+20) .. "..."
  msg = ("pattern error near '%s'"):format(msg)
  error(msg, 2)
end
local function mult (p, n)
  local np = mm.P(true)
  while n >= 1 do
    if n%2 >= 1 then np = np * p end
    p = p * p
    n = n/2
  end
  return np
end
local function equalcap (s, i, c)
  if type(c) ~= "string" then return nil end
  local e = #c + i
  if s:sub(i, e - 1) == c then return e else return nil end
end
local S = (Predef.space + "--" * (any - Predef.nl)^0)^0
local name = m.R("AZ", "az", "__") * m.R("AZ", "az", "__", "09")^0
local arrow = S * "<-"
local seq_follow = m.P"/" + ")" + "}" + ":}" + "~}" + "|}" + (name * arrow) + -1
name = m.C(name)
local Def = name * m.Carg(1)
local function getdef (id, defs)
  local c = defs and defs[id]
  if not c then error("undefined name: " .. id) end
  return c
end
local function defwithfunc (f)
  return m.Cg(Def / getdef * m.Cc(f))
end
local num = m.C(m.R"09"^1) * S / tonumber
local String = "'" * m.C((any - "'")^0) * "'" + '"' * m.C((any - '"')^0) * '"'
local defined = "%" * Def / function (c,Defs)
  local cat =  Defs and Defs[c] or Predef[c]
  if not cat then error ("name '" .. c .. "' undefined") end
  return cat
end
local Range = m.Cs(any * (m.P"-"/"") * (any - "]")) / mm.R
local item = (defined + Range + m.C(any)) / m.P
local Class =
    "["
  * (m.C(m.P"^"^-1))
  * (item * ((item % mt.__add) - "]")^0) /
                          function (c, p) return c == "^" and any - p or p end
  * "]"
local function adddef (t, k, exp)
  if t[k] then
    error("'"..k.."' already defined as a rule")
  else
    t[k] = exp
  end
  return t
end
local function firstdef (n, r) return adddef({n}, n, r) end
local function NT (n, b)
  if not b then
    error("rule '"..n.."' used outside a grammar")
  else return mm.V(n)
  end
end
local exp = m.P{ "Exp",
  Exp = S * ( m.V"Grammar"
            + m.V"Seq" * ("/" * S * m.V"Seq" % mt.__add)^0 );
  Seq = (m.Cc(m.P"") * (m.V"Prefix" % mt.__mul)^0)
        * (#seq_follow + patt_error);
  Prefix = "&" * S * m.V"Prefix" / mt.__len
         + "!" * S * m.V"Prefix" / mt.__unm
         + m.V"Suffix";
  Suffix = m.V"Primary" * S *
          ( ( m.P"+" * m.Cc(1, mt.__pow)
            + m.P"*" * m.Cc(0, mt.__pow)
            + m.P"?" * m.Cc(-1, mt.__pow)
            + "^" * ( m.Cg(num * m.Cc(mult))
                    + m.Cg(m.C(m.S"+-" * m.R"09"^1) * m.Cc(mt.__pow))
                    )
            + "->" * S * ( m.Cg((String + num) * m.Cc(mt.__div))
                         + m.P"{}" * m.Cc(nil, m.Ct)
                         + defwithfunc(mt.__div)
                         )
            + "=>" * S * defwithfunc(mm.Cmt)
            + ">>" * S * defwithfunc(mt.__mod)
            + "~>" * S * defwithfunc(mm.Cf)
            ) % function (a,b,f) return f(a,b) end * S
          )^0;
  Primary = "(" * m.V"Exp" * ")"
            + String / mm.P
            + Class
            + defined
            + "{:" * (name * ":" + m.Cc(nil)) * m.V"Exp" * ":}" /
                     function (n, p) return mm.Cg(p, n) end
            + "=" * name / function (n) return mm.Cmt(mm.Cb(n), equalcap) end
            + m.P"{}" / mm.Cp
            + "{~" * m.V"Exp" * "~}" / mm.Cs
            + "{|" * m.V"Exp" * "|}" / mm.Ct
            + "{" * m.V"Exp" * "}" / mm.C
            + m.P"." * m.Cc(any)
            + (name * -arrow + "<" * name * ">") * m.Cb("G") / NT;
  Definition = name * arrow * m.V"Exp";
  Grammar = m.Cg(m.Cc(true), "G") *
            ((m.V"Definition" / firstdef) * (m.V"Definition" % adddef)^0) / mm.P
}
local pattern = S * m.Cg(m.Cc(false), "G") * exp / mm.P * (-any + patt_error)
local function compile (p, defs)
  if mm.type(p) == "pattern" then return p end
  local cp = pattern:match(p, 1, defs)
  if not cp then error("incorrect pattern", 3) end
  return cp
end
local function match (s, p, i)
  local cp = mem[p]
  if not cp then
    cp = compile(p)
    mem[p] = cp
  end
  return cp:match(s, i or 1)
end
local function find (s, p, i)
  local cp = fmem[p]
  if not cp then
    cp = compile(p) / 0
    cp = mm.P{ mm.Cp() * cp * mm.Cp() + 1 * mm.V(1) }
    fmem[p] = cp
  end
  local i, e = cp:match(s, i or 1)
  if i then return i, e - 1
  else return i
  end
end
local function gsub (s, p, rep)
  local g = gmem[p] or {}
  gmem[p] = g
  local cp = g[rep]
  if not cp then
    cp = compile(p)
    cp = mm.Cs((cp / rep + 1)^0)
    g[rep] = cp
  end
  return cp:match(s)
end
local re = {
  compile = compile,
  match = match,
  find = find,
  gsub = gsub,
  updatelocale = updatelocale,
}
if version == "Lua 5.1" then _G.re = re end
return re
)LUA";

    // Execute re.lua - it returns the 're' module table
    int reResult = luaL_dostring(L, reLuaCode);
    if (reResult == LUA_OK) {
        // Stack now has the 're' table on top
        // Register as both global and in package.loaded
        lua_pushvalue(L, -1);   // duplicate the re table
        lua_setglobal(L, "re"); // Register as global "re"
        lua_getglobal(L, "package");
        lua_getfield(L, -1, "loaded");
        lua_pushvalue(L, -3);      // push re table again
        lua_setfield(L, -2, "re"); // package.loaded["re"] = re
        lua_pop(L, 3);             // pop loaded, package, and re
    } else {
        qWarning() << "Failed to load re.lua:" << lua_tostring(L, -1);
        lua_pop(L, 1); // pop error message
    }

    // TODO: Register additional libraries
    //    luaopen_rex(L);    // PCRE regex
    //    etc.

    // 6b. Install path normalization wrappers for Windows plugin compatibility
    // Windows plugins use backslashes in paths, but POSIX systems need forward slashes
    // We wrap io.open, dofile, and loadfile to transparently convert separators
    // Also try application's lua/ directory for files not found at relative path
    // Note: Pass app_dir as a global to avoid string escaping issues
    QString normalizedAppDir = appDir;
    normalizedAppDir.replace('\\', '/');
    lua_pushstring(L, normalizedAppDir.toUtf8().constData());
    lua_setglobal(L, "_MUSHCLIENT_APP_DIR");

    const char* pathWrapperCode = R"(
        -- Store original functions
        local orig_io_open = io.open
        local orig_dofile = dofile
        local orig_loadfile = loadfile

        -- Application directory for fallback searches (set by C++)
        local app_dir = _MUSHCLIENT_APP_DIR or ""
        local lua_dir = app_dir .. "/lua/"

        -- Helper to normalize path separators (\ to /)
        local function normalize_path(path)
            if type(path) == "string" then
                return (path:gsub("\\", "/"))
            end
            return path
        end

        -- Helper to check if file exists
        local function file_exists(path)
            local f = orig_io_open(path, "r")
            if f then
                f:close()
                return true
            end
            return false
        end

        -- Helper to resolve path, trying app directory and lua directory if needed
        local function resolve_path(path)
            path = normalize_path(path)
            -- If absolute path or file exists at relative path, use as-is
            if path:sub(1,1) == "/" or file_exists(path) then
                return path
            end
            -- Try app directory first (for paths like "lua/aard_requirements.lua")
            local app_path = app_dir .. "/" .. path
            if file_exists(app_path) then
                return app_path
            end
            -- Try lua directory (for paths like "foo.lua" that should be in lua/)
            local lua_path = lua_dir .. path
            if file_exists(lua_path) then
                return lua_path
            end
            -- Return original (will fail with proper error message)
            return path
        end

        -- Wrap io.open to normalize paths and try app/lua directories
        io.open = function(filename, ...)
            return orig_io_open(resolve_path(filename), ...)
        end

        -- Wrap dofile to normalize paths and try lua directory
        dofile = function(filename)
            return orig_dofile(resolve_path(filename))
        end

        -- Wrap loadfile to normalize paths and try lua directory
        loadfile = function(filename, ...)
            return orig_loadfile(resolve_path(filename), ...)
        end

        -- Wrap require to normalize backslashes in package.path before searching
        -- This handles plugins that use Windows-style paths like "..\\subdir\\?.lua"
        local orig_require = require
        require = function(modname)
            -- Normalize backslashes in package.path and package.cpath
            package.path = (package.path or ""):gsub("\\", "/")
            package.cpath = (package.cpath or ""):gsub("\\", "/")
            return orig_require(modname)
        end

        -- Clean up the global we used to pass the path
        _MUSHCLIENT_APP_DIR = nil
    )";

    int result = luaL_dostring(L, pathWrapperCode);
    if (result != LUA_OK) {
        qWarning() << "Failed to install path normalization wrappers:" << lua_tostring(L, -1);
        lua_pop(L, 1); // pop error message
    }

    // 8. Create error_code table for Lua scripts
    // This allows scripts to check error codes by name: if result ~= error_code.eOK then ...
    lua_newtable(L);
    // clang-format off
    #define SET_ERROR_CODE(name, value) lua_pushinteger(L, value); lua_setfield(L, -2, #name)
    SET_ERROR_CODE(eOK, 0);
    SET_ERROR_CODE(eWorldOpen, 30001);
    SET_ERROR_CODE(eWorldClosed, 30002);
    SET_ERROR_CODE(eNoNameSpecified, 30003);
    SET_ERROR_CODE(eCannotPlaySound, 30004);
    SET_ERROR_CODE(eTriggerNotFound, 30005);
    SET_ERROR_CODE(eTriggerAlreadyExists, 30006);
    SET_ERROR_CODE(eTriggerCannotBeEmpty, 30007);
    SET_ERROR_CODE(eInvalidObjectLabel, 30008);
    SET_ERROR_CODE(eScriptNameNotLocated, 30009);
    SET_ERROR_CODE(eAliasNotFound, 30010);
    SET_ERROR_CODE(eAliasAlreadyExists, 30011);
    SET_ERROR_CODE(eAliasCannotBeEmpty, 30012);
    SET_ERROR_CODE(eCouldNotOpenFile, 30013);
    SET_ERROR_CODE(eLogFileNotOpen, 30014);
    SET_ERROR_CODE(eLogFileAlreadyOpen, 30015);
    SET_ERROR_CODE(eLogFileBadWrite, 30016);
    SET_ERROR_CODE(eTimerNotFound, 30017);
    SET_ERROR_CODE(eTimerAlreadyExists, 30018);
    SET_ERROR_CODE(eVariableNotFound, 30019);
    SET_ERROR_CODE(eCommandNotEmpty, 30020);
    SET_ERROR_CODE(eBadRegularExpression, 30021);
    SET_ERROR_CODE(eTimeInvalid, 30022);
    SET_ERROR_CODE(eBadMapItem, 30023);
    SET_ERROR_CODE(eNoMapItems, 30024);
    SET_ERROR_CODE(eUnknownOption, 30025);
    SET_ERROR_CODE(eOptionOutOfRange, 30026);
    SET_ERROR_CODE(eTriggerSequenceOutOfRange, 30027);
    SET_ERROR_CODE(eTriggerSendToInvalid, 30028);
    SET_ERROR_CODE(eTriggerLabelNotSpecified, 30029);
    SET_ERROR_CODE(ePluginFileNotFound, 30030);
    SET_ERROR_CODE(eProblemsLoadingPlugin, 30031);
    SET_ERROR_CODE(ePluginCannotSetOption, 30032);
    SET_ERROR_CODE(ePluginCannotGetOption, 30033);
    SET_ERROR_CODE(eNoSuchPlugin, 30034);
    SET_ERROR_CODE(eNotAPlugin, 30035);
    SET_ERROR_CODE(eNoSuchRoutine, 30036);
    SET_ERROR_CODE(ePluginDoesNotSaveState, 30037);
    SET_ERROR_CODE(ePluginCouldNotSaveState, 30038);
    SET_ERROR_CODE(ePluginDisabled, 30039);
    SET_ERROR_CODE(eErrorCallingPluginRoutine, 30040);
    SET_ERROR_CODE(eCommandsNestedTooDeeply, 30041);
    SET_ERROR_CODE(eBadParameter, 30046);
    SET_ERROR_CODE(eClipboardEmpty, 30050);
    SET_ERROR_CODE(eFileNotFound, 30051);
    SET_ERROR_CODE(eAlreadyTransferringFile, 30052);
    SET_ERROR_CODE(eNotTransferringFile, 30053);
    SET_ERROR_CODE(eNoSuchCommand, 30054);
    SET_ERROR_CODE(eArrayAlreadyExists, 30055);
    SET_ERROR_CODE(eArrayDoesNotExist, 30056);
    SET_ERROR_CODE(eArrayNotEvenNumberOfValues, 30057);
    SET_ERROR_CODE(eImportedWithDuplicates, 30058);
    SET_ERROR_CODE(eBadDelimiter, 30059);
    SET_ERROR_CODE(eSetReplacingExistingValue, 30060);
    SET_ERROR_CODE(eKeyDoesNotExist, 30061);
    SET_ERROR_CODE(eCannotImport, 30062);
    SET_ERROR_CODE(eItemInUse, 30063);
    SET_ERROR_CODE(eSpellCheckNotActive, 30064);
    SET_ERROR_CODE(eCannotAddFont, 30065);
    SET_ERROR_CODE(ePenStyleNotValid, 30066);
    SET_ERROR_CODE(eUnableToLoadImage, 30067);
    SET_ERROR_CODE(eImageNotInstalled, 30068);
    SET_ERROR_CODE(eInvalidNumberOfPoints, 30069);
    SET_ERROR_CODE(eInvalidPoint, 30070);
    SET_ERROR_CODE(eHotspotPluginChanged, 30071);
    SET_ERROR_CODE(eHotspotNotInstalled, 30072);
    SET_ERROR_CODE(eNoSuchWindow, 30073);
    SET_ERROR_CODE(eBrushStyleNotValid, 30074);
    SET_ERROR_CODE(eNoSuchNotepad, 30075);
    SET_ERROR_CODE(eFileNotOpened, 30076);
    SET_ERROR_CODE(eInvalidColourName, 30077);
    #undef SET_ERROR_CODE
    // clang-format on
    lua_setglobal(L, "error_code");

    // 9. Add check() utility function (from original lua_scripting.cpp)
    // This is so useful it's added as a built-in global function
    // See lua/check.lua for the standalone version
    const char* checkFunctionCode = R"(
        function check(result)
            if result ~= error_code.eOK then
                error(error_desc[result] or
                      string.format("Unknown error code: %i", result), 2)
            end
        end
    )";

    result = luaL_dostring(L, checkFunctionCode);
    if (result != LUA_OK) {
        qWarning() << "Failed to install check() function:" << lua_tostring(L, -1);
        lua_pop(L, 1);
    }

    // 10. Clear stack to ensure clean state
    lua_settop(L, 0);
}

/**
 * closeLua - Destroy Lua state
 *
 * Lua cleanup
 *
 * Based on CScriptEngine::CloseLua() from lua_scripting.cpp
 *
 * Closes the Lua state and frees all Lua-allocated memory.
 * Sets L to nullptr to indicate Lua is no longer active.
 */
void ScriptEngine::closeLua()
{
    if (L) {
        lua_close(L);
        L = nullptr;
    }
}

// ========== Script Loading and Parsing ==========

/**
 * getTracebackFunction - Get debug.traceback function
 *
 * Pushes debug.traceback function onto stack, or nil if not available.
 *
 * Based on lua_scripting.cpp traceback handling
 *
 * @param L Lua state
 */
static void getTracebackFunction(lua_State* L)
{
    // Get debug table
    lua_getglobal(L, "debug");

    if (lua_istable(L, -1)) {
        // Get debug.traceback field
        lua_getfield(L, -1, "traceback");
        lua_remove(L, -2); // Remove debug table

        if (lua_isfunction(L, -1))
            return; // Got traceback function
    }

    // Couldn't get traceback, push nil
    lua_pop(L, 1);
    lua_pushnil(L);
}

/**
 * callLuaWithTraceBack - Call Lua function with debug traceback
 *
 * Calls lua_pcall with debug.traceback as the error handler,
 * so error messages include full stack traces.
 *
 * Based on CScriptEngine::CallLuaWithTraceBack() from lua_scripting.cpp
 *
 * @param L Lua state
 * @param nArgs Number of arguments on stack
 * @param nResults Number of expected results
 * @return Error code (0 = success, non-zero = error)
 */
int callLuaWithTraceBack(lua_State* L, int nArgs, int nResults)
{
    int base = lua_gettop(L) - nArgs; // Function index

    // Get debug.traceback function
    getTracebackFunction(L);

    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);                           // Pop nil
        return lua_pcall(L, nArgs, nResults, 0); // Call without traceback
    }

    lua_insert(L, base); // Put traceback under function and args
    int error = lua_pcall(L, nArgs, nResults, base);
    lua_remove(L, base); // Remove traceback function

    return error;
}

/**
 * luaError - Display Lua error message
 *
 * Extracts error message from Lua stack, parses line number,
 * and displays error to user.
 *
 * Based on luaError() from lua_scripting.cpp
 *
 * @param L Lua state (error message on top of stack)
 * @param event Event description ("Compile error", "Run-time error")
 * @param name Script name ("Script file", "Plugin")
 * @param doc WorldDocument (for displaying errors)
 */
static void luaError(lua_State* L, const QString& event, const QString& name, WorldDocument* doc)
{
    // Get error message from Lua stack
    QString errorMsg;
    if (lua_isstring(L, -1)) {
        errorMsg = QString::fromUtf8(lua_tostring(L, -1));
    } else {
        errorMsg = "<unknown error>";
    }
    lua_settop(L, 0); // Clear stack

    // Parse line number from error message
    // Format: "[string \"Script file\"]: error message"
    int lineNumber = 0;
    QRegularExpression lineRe(R"(\[string "[^"]+"\]:(\d+):)");
    QRegularExpressionMatch match = lineRe.match(errorMsg);
    if (match.hasMatch()) {
        lineNumber = match.captured(1).toInt();
    }

    // Display error
    qWarning() << "=== Lua Error ===" << event;
    qWarning() << "  Script:" << name;
    qWarning() << "  Message:" << errorMsg;
    if (lineNumber > 0) {
        qWarning() << "  Line:" << lineNumber;
    }

    // Display in output window
    if (doc) {
        // Orange error text on black background (BGR format for MUSHclient compatibility)
        doc->colourNote(BGR(255, 140, 0), BGR(0, 0, 0),
                        QString("=== %1: %2 ===").arg(event, name));
        doc->colourNote(BGR(255, 140, 0), BGR(0, 0, 0), errorMsg);

        // Show error lines (if we have a line number and script file)
        if (lineNumber > 0) {
            doc->showErrorLines(lineNumber);
        }
    }
}

/**
 * parseLua - Parse and execute Lua code
 *
 * Script Loading and Parsing
 *
 * Compiles Lua code with luaL_loadbuffer(), then executes it
 * with callLuaWithTraceBack(). Handles both compile and runtime
 * errors by calling luaError().
 *
 * Based on CScriptEngine::ParseLua() from lua_scripting.cpp
 *
 * @param code Lua code to execute
 * @param name Name for error messages (e.g., "Script file")
 * @return true on error, false on success
 */
bool ScriptEngine::parseLua(const QString& code, const QString& name)
{
    if (!L) {
        qWarning() << "parseLua: No Lua state";
        return true; // Error
    }

    // Time the operation
    QElapsedTimer timer;
    timer.start();

    // Convert QString to UTF-8 for Lua
    QByteArray codeBytes = code.toUtf8();
    QByteArray nameBytes = name.toUtf8();

    // Compile the Lua code
    if (luaL_loadbuffer(L, codeBytes.constData(), codeBytes.length(), nameBytes.constData())) {
        // Compile error
        luaError(L, "Compile error", name, m_doc);
        return true; // Error
    }

    // Execute the compiled code (this defines functions)
    int error = callLuaWithTraceBack(L, 0, 0);

    if (error) {
        // Runtime error
        luaError(L, "Run-time error", name, m_doc);
        return true; // Error
    }

    // Clear stack
    lua_settop(L, 0);

    // Update statistics (stored in document)
    if (m_doc) {
        qint64 elapsed = timer.nsecsElapsed();
        m_doc->m_iScriptTimeTaken += elapsed;
    }

    return false; // Success
}
