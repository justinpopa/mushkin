/**
 * lua_bind.h - High-level binding utilities for the Lua API
 *
 * This header provides 7 components that reduce boilerplate across all Lua
 * API implementation files. It is included at the bottom of lua_common.h and
 * therefore has access to all helpers declared there (luaPushQString,
 * luaCheckQString, doc(), plugin(), etc.).
 *
 * Components:
 *   1. luaPush<T> / luaGet<T>              - typed stack operations
 *   2. luaArgs<Ts...>                      - variadic argument extraction
 *   3. luaReturn<Ts...>                    - variadic return
 *   4. LUA_UNWRAP / LUA_UNWRAP_VOID        - std::expected unwrap macros
 *   5. LUA_DOC_GETTER / SETTER / METHOD_OK - simple accessor macros
 *   6. InfoField<T> / luaInfoLookup        - info-table dispatch
 *   7. pluginOrDocLookup / findTrigger / findAlias / findTimer
 */

#pragma once

#include <concepts>
#include <cstddef>
#include <tuple>
#include <type_traits>

// ============================================================
// 1. luaPush<T> and luaGet<T>
//
// We use constrained function overloads rather than explicit template
// specialisations to avoid duplicate-definition errors when Qt type aliases
// (qint32, QRgb, quint32, …) resolve to the same underlying integer type as
// another overload on a given platform.
//
// Priority (highest specificity first so the compiler picks the right one):
//   QString  → luaPushQString
//   QStringList → luaPushQStringList
//   bool     → lua_pushboolean  (must come before integral to avoid bool→int)
//   enum     → cast to int, lua_pushnumber
//   integral → lua_pushnumber
//   float / double → lua_pushnumber
//   const char* → lua_pushstring
// ============================================================

/** QString — push as UTF-8 binary-safe string */
inline void luaPush(lua_State* L, const QString& val)
{
    luaPushQString(L, val);
}

/** QStringList — push as 1-indexed Lua table */
inline void luaPush(lua_State* L, const QStringList& val)
{
    luaPushQStringList(L, val);
}

/** bool — must come before the integral overload to prevent bool→int promotion */
inline void luaPush(lua_State* L, bool val)
{
    lua_pushboolean(L, val ? 1 : 0);
}

/** Enum types — cast to underlying integer, then push as number */
template <typename T>
    requires std::is_enum_v<T>
void luaPush(lua_State* L, T val)
{
    lua_pushnumber(L, static_cast<lua_Number>(static_cast<std::underlying_type_t<T>>(val)));
}

/** All integral types (int, qint32, qint64, quint16, quint32, QRgb, …) */
template <typename T>
    requires(std::is_integral_v<T> && !std::is_same_v<T, bool> && !std::is_enum_v<T>)
void luaPush(lua_State* L, T val)
{
    lua_pushnumber(L, static_cast<lua_Number>(val));
}

/** Floating-point types (float, double) */
template <typename T>
    requires std::is_floating_point_v<T>
void luaPush(lua_State* L, T val)
{
    lua_pushnumber(L, static_cast<lua_Number>(val));
}

/** Raw C string */
inline void luaPush(lua_State* L, const char* val)
{
    lua_pushstring(L, val);
}

// ---------- luaGet<T> ----------
//
// Same strategy: overloads instead of explicit specialisations.
// The primary template is left undefined; only the overloads below compile.

/** Primary template — undefined; triggers a clean linker error if called with an unsupported type.
 */
template <typename T> T luaGet(lua_State* L, int idx);

template <> inline QString luaGet<QString>(lua_State* L, int idx)
{
    return luaCheckQString(L, idx);
}

template <> inline int luaGet<int>(lua_State* L, int idx)
{
    return static_cast<int>(luaL_checknumber(L, idx));
}

template <> inline double luaGet<double>(lua_State* L, int idx)
{
    return static_cast<double>(luaL_checknumber(L, idx));
}

template <> inline bool luaGet<bool>(lua_State* L, int idx)
{
    // lua_toboolean accepts any Lua type (0/nil → false, everything else → true)
    return lua_toboolean(L, idx) != 0;
}

// Provide qint64 only when it differs from int (e.g., 64-bit platforms where
// long long ≠ int).
#if defined(__LP64__) || defined(_WIN64)
template <> inline qint64 luaGet<qint64>(lua_State* L, int idx)
{
    return static_cast<qint64>(luaL_checknumber(L, idx));
}
#endif

// ============================================================
// 2. luaArgs<Ts...>
//    Extract a typed tuple of arguments from the Lua stack,
//    starting at index 1.
//
//    Usage:
//      auto [name, enabled] = luaArgs<QString, bool>(L);
// ============================================================

namespace detail {
template <typename Tuple, std::size_t... Is>
Tuple luaArgsImpl(lua_State* L, std::index_sequence<Is...>)
{
    return Tuple{luaGet<std::tuple_element_t<Is, Tuple>>(L, static_cast<int>(Is + 1))...};
}
} // namespace detail

template <typename... Ts> std::tuple<Ts...> luaArgs(lua_State* L)
{
    return detail::luaArgsImpl<std::tuple<Ts...>>(L, std::index_sequence_for<Ts...>{});
}

// ============================================================
// 3. luaReturn<Ts...>
//    Push multiple values to the Lua stack and return their count.
//
//    Usage:
//      return luaReturn(L, name, enabled, count);
// ============================================================

template <typename... Ts> int luaReturn(lua_State* L, const Ts&... vals)
{
    (luaPush(L, vals), ...);
    return static_cast<int>(sizeof...(Ts));
}

// ============================================================
// 4. LUA_UNWRAP / LUA_UNWRAP_VOID
//    Convenience macros for std::expected results.
//
//    LUA_UNWRAP(result)
//      - result must be a std::expected<T, int-convertible-error>
//      - if the result has no value, returns luaReturnError(L, result.error())
//      - variable 'L' must be in scope
//
//    LUA_UNWRAP_VOID(expr, err_code)
//      - expr must be a std::expected<void, ...>
//      - if the expression has no value, returns luaReturnError(L, err_code)
// ============================================================

/** Unwrap a std::expected; on failure push .error() as the Lua error code. */
#define LUA_UNWRAP(result)                                                                         \
    do {                                                                                           \
        if (!(result).has_value())                                                                 \
            return luaReturnError(L, static_cast<int>((result).error()));                          \
    } while (0)

/** Evaluate expr (a std::expected<void,...>); on failure push err_code. */
#define LUA_UNWRAP_VOID(expr, err_code)                                                            \
    do {                                                                                           \
        if (!(expr).has_value())                                                                   \
            return luaReturnError(L, (err_code));                                                  \
    } while (0)

// ============================================================
// 5. Simple getter/setter/method macros
//    These generate complete Lua-callable C functions.
//
//    LUA_DOC_GETTER(funcName, expr)
//      - Retrieves doc(), evaluates expr (which may use pDoc), pushes result.
//      - Return type is deduced automatically via the luaPush overload set.
//
//    LUA_DOC_SETTER_BOOL(funcName, field)
//      - Takes an optional boolean argument (defaults to true).
//      - Sets pDoc->field and returns nothing (0 values).
//
//    LUA_DOC_METHOD_OK(funcName, method_call)
//      - Calls pDoc->method_call with no return value check.
//      - Returns eOK to Lua.
// ============================================================

/**
 * Generate a zero-argument getter that deduces the return type via luaPush.
 *
 * Example:
 *   LUA_DOC_GETTER(LuaGetWorldName, pDoc->name())
 *   -> int LuaGetWorldName(lua_State* L) { ... }
 */
#define LUA_DOC_GETTER(funcName, expr)                                                             \
    int funcName(lua_State* L)                                                                     \
    {                                                                                              \
        WorldDocument* pDoc = doc(L);                                                              \
        luaPush(L, (expr));                                                                        \
        return 1;                                                                                  \
    }

/**
 * Generate a boolean setter with optional first argument (defaults to true).
 *
 * Example:
 *   LUA_DOC_SETTER_BOOL(LuaSetLogging, m_bLogging)
 */
#define LUA_DOC_SETTER_BOOL(funcName, field)                                                       \
    int funcName(lua_State* L)                                                                     \
    {                                                                                              \
        WorldDocument* pDoc = doc(L);                                                              \
        bool val = lua_isnone(L, 1) ? true : (lua_toboolean(L, 1) != 0);                           \
        pDoc->field = val;                                                                         \
        return 0;                                                                                  \
    }

/**
 * Generate a void method wrapper that returns eOK on success.
 *
 * Example:
 *   LUA_DOC_METHOD_OK(LuaClearOutput, ClearOutput())
 */
#define LUA_DOC_METHOD_OK(funcName, method_call)                                                   \
    int funcName(lua_State* L)                                                                     \
    {                                                                                              \
        WorldDocument* pDoc = doc(L);                                                              \
        pDoc->method_call;                                                                         \
        return luaReturnOK(L);                                                                     \
    }

// ============================================================
// 6. InfoField<T> / luaInfoLookup
//    Generic dispatch table for "GetXxxInfo(name, info_type)" functions.
//
//    Define a static table of InfoField entries, then call luaInfoLookup
//    to push the matching field (or nil if not found).
//
//    Usage:
//      static const InfoField<Trigger> kFields[] = {
//          {1, [](lua_State* L, const Trigger& t){ luaPush(L, t.label); }},
//          {2, [](lua_State* L, const Trigger& t){ luaPush(L, t.enabled); }},
//      };
//      return luaInfoLookup(L, *trigger, kFields, info_type);
// ============================================================

/** A single entry mapping an integer info_type to a field push function. */
template <typename T> struct InfoField {
    int id;
    void (*push)(lua_State* L, const T& obj);
};

/**
 * Linear scan over a fixed-size InfoField table.
 * Pushes the matching field and returns 1, or pushes nil and returns 1.
 *
 * @param L         Lua state
 * @param obj       Object whose field is to be pushed
 * @param table     C-array of InfoField<T>
 * @param info_type Integer selector provided by the Lua caller
 */
template <typename T, std::size_t N>
int luaInfoLookup(lua_State* L, const T& obj, const InfoField<T> (&table)[N], int info_type)
{
    for (std::size_t i = 0; i < N; ++i) {
        if (table[i].id == info_type) {
            table[i].push(L, obj);
            return 1;
        }
    }
    lua_pushnil(L);
    return 1;
}

// ============================================================
// 7. Plugin/World lookup helpers
//    When a plugin is active its own maps take priority; otherwise
//    the lookup falls through to the world document.
//
//    pluginOrDocLookup<T, Map>(L, name, plugin_map, doc_getter)
//      - If plugin(L) != nullptr: searches plugin->*plugin_map
//      - Otherwise: calls (doc(L)->*doc_getter)(name)
//
//    findTrigger / findAlias / findTimer
//      - Convenience wrappers with the correct map/getter arguments.
// ============================================================

/**
 * Look up an object by name from the active plugin's map, falling back to the
 * world document's getter method when no plugin is present.
 *
 * @tparam T          Object type (Trigger, Alias, Timer)
 * @tparam Map        std::map<QString, std::unique_ptr<T>> member type
 * @param L           Lua state
 * @param name        Object name to look up
 * @param plugin_map  Pointer-to-member for the plugin's collection
 * @param doc_getter  Pointer-to-member function on WorldDocument
 * @return Raw pointer (non-owning) or nullptr if not found
 */
template <typename T, typename Map>
T* pluginOrDocLookup(lua_State* L, const QString& name, Map Plugin::* plugin_map,
                     T* (WorldDocument::*doc_getter)(const QString&))
{
    Plugin* p = plugin(L);
    if (p != nullptr) {
        auto& m = p->*plugin_map;
        auto it = m.find(name);
        if (it != m.end()) {
            return it->second.get();
        }
        return nullptr;
    }
    return (doc(L)->*doc_getter)(name);
}

/** Find a Trigger by name, checking the active plugin first. */
inline Trigger* findTrigger(lua_State* L, const QString& name)
{
    return pluginOrDocLookup<Trigger>(L, name, &Plugin::m_TriggerMap, &WorldDocument::getTrigger);
}

/** Find an Alias by name, checking the active plugin first. */
inline Alias* findAlias(lua_State* L, const QString& name)
{
    return pluginOrDocLookup<Alias>(L, name, &Plugin::m_AliasMap, &WorldDocument::getAlias);
}

/** Find a Timer by name, checking the active plugin first. */
inline Timer* findTimer(lua_State* L, const QString& name)
{
    return pluginOrDocLookup<Timer>(L, name, &Plugin::m_TimerMap, &WorldDocument::getTimer);
}
