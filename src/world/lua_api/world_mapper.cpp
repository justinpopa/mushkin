/**
 * world_mapper.cpp - Mapper Lua API Functions
 *
 * Implements the MUSHclient mapper API: AddToMapper, AddMapperComment,
 * DeleteAllMapItems, DeleteLastMapItem, EnableMapping, GetMappingCount,
 * GetMappingItem, GetMappingString, GetMapping, SetMapping, GetMapColour,
 * MapColour, MapColourList, GetRemoveMapReverses, SetRemoveMapReverses.
 *
 * Map list entries are stored as:
 *   - Short direction:   "n", "s", "ne", etc.
 *   - Special move:      "forward_part/reverse_part"  e.g. "climb tree/close door"
 *   - Comment:           "{text}"
 */

#include "lua_common.h"

#include <string_view>
#include <unordered_map>

// ========== Direction Table ==========

namespace {

/**
 * MapDirection - short form and its reverse for a single direction abbreviation.
 *
 * short_form is what gets stored in m_mapList.
 * reverse    is the short_form of the opposite direction.
 */
struct MapDirection {
    const char* short_form;
    const char* reverse;
};

/**
 * Lookup table keyed by any canonical spelling (long or short) of a direction.
 *
 * For each long-name entry the short_form field contains the abbreviation that
 * GetMappingString will emit (e.g. "n").  The reverse field is always the
 * abbreviation of the opposite direction (e.g. "s").
 *
 * One-char abbreviations are included so the auto-mapping path can normalise
 * directions without a separate code-path.
 *
 * The "f" entry is a self-reversing filler direction used by some MUSHclient
 * scripts.
 */
static const std::unordered_map<std::string_view, MapDirection> kDirectionMap = {
    // Long names
    {"north", {"n", "s"}},
    {"south", {"s", "n"}},
    {"east", {"e", "w"}},
    {"west", {"w", "e"}},
    {"up", {"u", "d"}},
    {"down", {"d", "u"}},
    // Short names (abbrevs)
    {"n", {"n", "s"}},
    {"s", {"s", "n"}},
    {"e", {"e", "w"}},
    {"w", {"w", "e"}},
    {"u", {"u", "d"}},
    {"d", {"d", "u"}},
    {"ne", {"ne", "sw"}},
    {"nw", {"nw", "se"}},
    {"se", {"se", "nw"}},
    {"sw", {"sw", "ne"}},
    // Self-reversing filler direction
    {"f", {"f", "f"}},
};

/**
 * containsInvalidChar - return true if s contains any of { } ( ) / \
 *
 * These characters are reserved as delimiters / special markers in the map
 * string format and must be rejected at input time.
 */
static bool containsInvalidChar(std::string_view s)
{
    for (char c : s) {
        if (c == '{' || c == '}' || c == '(' || c == ')' || c == '/' || c == '\\')
            return true;
    }
    return false;
}

} // namespace

// ========== Lua API Functions ==========

/**
 * world.AddToMapper(direction, reverse)
 *
 * Adds one step to the in-memory map list.
 *
 * Always requires exactly two arguments: direction and reverse.
 * Both arguments are stored verbatim as "direction/reverse" (no normalization).
 * Either argument alone may be an empty string, but not BOTH at the same time.
 *
 * Validation: neither argument may contain { } ( ) / \
 *
 * @return eOK (0) on success, eBadMapItem (30023) on invalid input.
 */
int L_AddToMapper(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* dir_raw = luaL_checkstring(L, 1);
    const char* rev_raw = luaL_checkstring(L, 2);

    std::string_view dir_sv(dir_raw);
    std::string_view rev_sv(rev_raw);

    // Reject only when BOTH are empty; either one alone being empty is acceptable
    if (dir_sv.empty() && rev_sv.empty())
        return luaReturnError(L, eBadMapItem);

    if (containsInvalidChar(dir_sv) || containsInvalidChar(rev_sv))
        return luaReturnError(L, eBadMapItem);

    QString entry = QString::fromUtf8(dir_raw) + QChar('/') + QString::fromUtf8(rev_raw);
    pDoc->m_mapList.append(entry);
    return luaReturnOK(L);
}

/**
 * world.AddMapperComment(comment)
 *
 * Appends a freeform comment to the map list.  The comment is stored as
 * "{comment}" so GetMappingString can distinguish it from direction entries.
 * An empty comment string is allowed (stored as "{}").
 *
 * @return eOK (0) on success, eBadMapItem (30023) on invalid input.
 */
int L_AddMapperComment(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* text = luaL_checkstring(L, 1);
    std::string_view sv(text);

    if (containsInvalidChar(sv))
        return luaReturnError(L, eBadMapItem);

    pDoc->m_mapList.append(QChar('{') + QString::fromUtf8(text) + QChar('}'));
    return luaReturnOK(L);
}

/**
 * world.DeleteAllMapItems()
 *
 * Clears the entire map list.
 *
 * @return eOK (0) on success, eNoMapItems (30024) if the list was already empty.
 */
int L_DeleteAllMapItems(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    if (pDoc->m_mapList.isEmpty())
        return luaReturnError(L, eNoMapItems);

    pDoc->m_mapList.clear();
    return luaReturnOK(L);
}

/**
 * world.DeleteLastMapItem()
 *
 * Removes the most recently added map entry.
 *
 * @return eOK (0) on success, eNoMapItems (30024) if the list is empty.
 */
int L_DeleteLastMapItem(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    if (pDoc->m_mapList.isEmpty())
        return luaReturnError(L, eNoMapItems);

    pDoc->m_mapList.removeLast();
    return luaReturnOK(L);
}

/**
 * world.EnableMapping([enabled])
 *
 * Enables or disables the mapper.
 * Defaults to true when called with no arguments.
 *
 * @param enabled (boolean, optional) true to enable, false to disable (default: true)
 * @return eOK (0)
 */
int L_EnableMapping(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    pDoc->m_mapping.enabled = lua_gettop(L) < 1 || lua_toboolean(L, 1);
    return luaReturnOK(L);
}

/**
 * world.GetMappingCount()
 *
 * @return (number) Number of entries in the map list.
 */
int L_GetMappingCount(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushnumber(L, static_cast<lua_Number>(pDoc->m_mapList.size()));
    return 1;
}

/**
 * world.GetMappingItem(index)
 *
 * Returns the raw stored string at the given 0-based index.
 * Returns nil if the index is out of range (VT_NULL in original MUSHclient).
 *
 * @param index (number) 0-based index into the map list
 * @return (string) The stored entry string, or nil if out of range.
 */
int L_GetMappingItem(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    int index = static_cast<int>(luaL_checknumber(L, 1));

    if (index < 0 || index >= pDoc->m_mapList.size()) {
        lua_pushnil(L);
        return 1;
    }

    QByteArray bytes = pDoc->m_mapList.at(index).toUtf8();
    lua_pushlstring(L, bytes.constData(), bytes.size());
    return 1;
}

/**
 * world.GetMappingString()
 *
 * Builds a compact speedwalk string from the map list.
 *
 * Formatting rules:
 *   - Consecutive identical single-char directions are collapsed with a count
 *     prefix, capped at 99 per run: "3n ", "99n ".
 *   - Multi-char directions (ne/nw/se/sw) with count > 1 are parenthesized:
 *     "2(ne) ".
 *   - Single occurrences are emitted without count: "n ", "ne ".
 *   - Special moves (entries containing "/") are parenthesized in full:
 *     "(open door/close door) ".
 *   - Comments ("{text}" with length > 2) flush the current run, then are
 *     separated from surrounding content with \r\n before and after.
 *   - Tokens are built by appending each token with a trailing space.
 *
 * @return (string) The formatted speedwalk string.
 */
int L_GetMappingString(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const QStringList& list = pDoc->m_mapList;

    if (list.isEmpty()) {
        lua_pushliteral(L, "");
        return 1;
    }

    QString result;
    int i = 0;
    int total = list.size();

    // Helper lambda: flush a direction run into result
    auto flushRun = [&](const QString& item, int run) {
        if (run <= 0)
            return;
        if (run == 1) {
            result += item + QChar(' ');
        } else if (item.size() == 1) {
            // Single-char direction: "3n "
            result += QString::number(run) + item + QChar(' ');
        } else {
            // Multi-char direction (ne/nw/se/sw): "2(ne) "
            result += QString::number(run) + QChar('(') + item + QChar(')') + QChar(' ');
        }
    };

    while (i < total) {
        const QString& item = list.at(i);

        // Comment entry: starts with '{' and ends with '}' with length > 2
        if (item.startsWith(QChar('{')) && item.endsWith(QChar('}')) && item.size() > 2) {
            // Flush any accumulated run before the comment (already done inline below)
            // Prepend \r\n if there is already output
            if (!result.isEmpty())
                result += QStringLiteral("\r\n");
            result += item;
            result += QStringLiteral("\r\n");
            ++i;
            continue;
        }

        // Special move: contains '/' — treat entire stored string as one token
        if (item.contains(QChar('/'))) {
            result += QChar('(') + item + QChar(')') + QChar(' ');
            ++i;
            continue;
        }

        // Plain direction: count consecutive identical entries, capped at 99
        int run = 1;
        while (run < 99 && i + run < total && list.at(i + run) == item &&
               !item.contains(QChar('/')) && !item.startsWith(QChar('{'))) {
            ++run;
        }

        flushRun(item, run);
        i += run;

        // If we hit the 99 cap and the next item is the same direction, the
        // loop will naturally start a new run on the next iteration.
    }

    QByteArray bytes = result.toUtf8();
    lua_pushlstring(L, bytes.constData(), bytes.size());
    return 1;
}

/**
 * world.GetMapping()
 *
 * Returns the current state of the mapping active flag.
 *
 * @return (boolean) true if mapping is enabled, false otherwise.
 */
int L_GetMapping(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushboolean(L, pDoc->m_mapping.enabled ? 1 : 0);
    return 1;
}

/**
 * world.SetMapping([enabled])
 *
 * Sets the mapping active flag.
 * Defaults to true when called with no arguments.
 *
 * @param enabled (boolean, optional) New value for the flag (default: true)
 * @return eOK (0)
 */
int L_SetMapping(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    // Default to true if no argument (optboolean pattern)
    pDoc->m_mapping.enabled = lua_gettop(L) < 1 || lua_toboolean(L, 1);
    return luaReturnOK(L);
}

/**
 * world.GetMapColour(original)
 *
 * Looks up a colour substitution.  If original has a mapping, returns the
 * replacement; otherwise returns original unchanged.
 *
 * @param original (number) RGB colour value to look up
 * @return (number) Mapped RGB value, or original if no mapping exists
 */
int L_GetMapColour(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    QRgb original = static_cast<QRgb>(luaL_checknumber(L, 1));

    auto it = pDoc->m_colourTranslationMap.find(original);
    if (it != pDoc->m_colourTranslationMap.end()) {
        lua_pushnumber(L, static_cast<lua_Number>(it.value()));
    } else {
        lua_pushnumber(L, static_cast<lua_Number>(original));
    }
    return 1;
}

/**
 * world.MapColour(original, replacement)
 *
 * Adds or updates a colour substitution entry.
 *
 * @param original    (number) RGB colour value to substitute
 * @param replacement (number) RGB colour value to use instead
 * @return eOK (0)
 */
int L_MapColour(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    QRgb original = static_cast<QRgb>(luaL_checknumber(L, 1));
    QRgb replacement = static_cast<QRgb>(luaL_checknumber(L, 2));

    pDoc->m_colourTranslationMap.insert(original, replacement);
    return luaReturnOK(L);
}

/**
 * world.MapColourList()
 *
 * Returns a Lua table containing all colour substitution entries.
 * Keys are original RGB values (numbers); values are replacement RGB values.
 *
 * @return (table) { [original] = replacement, ... }
 */
int L_MapColourList(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    lua_newtable(L);

    for (auto it = pDoc->m_colourTranslationMap.cbegin(); it != pDoc->m_colourTranslationMap.cend();
         ++it) {
        lua_pushnumber(L, static_cast<lua_Number>(it.key()));
        lua_pushnumber(L, static_cast<lua_Number>(it.value()));
        lua_settable(L, -3);
    }

    return 1;
}

/**
 * world.GetRemoveMapReverses()
 *
 * Returns the current state of the auto-cancel-reverses flag.
 *
 * @return (boolean) true if auto-cancelling reverses is enabled, false otherwise.
 */
int L_GetRemoveMapReverses(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    lua_pushboolean(L, pDoc->m_mapping.remove_reverses ? 1 : 0);
    return 1;
}

/**
 * world.SetRemoveMapReverses(enabled)
 *
 * Sets the auto-cancel-reverses flag.
 * Requires an explicit boolean argument.
 *
 * @param enabled (boolean) New value for the flag
 * @return eOK (0)
 */
int L_SetRemoveMapReverses(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    pDoc->m_mapping.remove_reverses = lua_toboolean(L, 1) != 0;
    return luaReturnOK(L);
}
