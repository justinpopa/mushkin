/**
 * world_mapper.cpp - Mapper Lua API Functions
 *
 * Implements the MUSHclient mapper API: AddToMapper, AddMapperComment,
 * DeleteAllMapItems, DeleteLastMapItem, EnableMapping, GetMappingCount,
 * GetMappingItem, GetMappingString, Mapping, GetMapColour, MapColour,
 * MapColourList, RemoveMapReverses.
 *
 * Map list entries are stored as:
 *   - Short direction:   "n", "s", "ne", etc.
 *   - Special move:      "forward_part/reverse_part"  e.g. "climb tree/down"
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
 * One-char abbreviations are included so AddToMapper("n") can normalise and
 * detect reverses without a separate code-path.
 */
static const std::unordered_map<std::string_view, MapDirection> kDirectionMap = {
    // Long names
    {"north", {"n", "s"}},
    {"south", {"s", "n"}},
    {"east", {"e", "w"}},
    {"west", {"w", "e"}},
    {"up", {"u", "d"}},
    {"down", {"d", "u"}},
    {"northeast", {"ne", "sw"}},
    {"northwest", {"nw", "se"}},
    {"southeast", {"se", "nw"}},
    {"southwest", {"sw", "ne"}},
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
 * world.AddToMapper(direction [, reverse])
 *
 * Adds one step to the in-memory map list.
 *
 * With two arguments the entry is stored as "direction/reverse".
 * With one argument:
 *   - If the direction is a recognised compass direction (long or short form)
 *     the stored short_form is used.
 *   - Otherwise the text is stored as-is.
 *
 * If m_bRemoveMapReverses is true and the last entry in the list is the
 * reverse of this direction, that entry is popped instead of appending.
 *
 * Validation: neither argument may contain { } ( ) / \
 *
 * @return eOK (0) on success, eBadMapItem (30023) on invalid input.
 */
int L_AddToMapper(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    int nargs = lua_gettop(L);

    const char* dir_raw = luaL_checkstring(L, 1);
    std::string_view dir_sv(dir_raw);

    if (dir_sv.empty() || containsInvalidChar(dir_sv))
        return luaReturnError(L, eBadMapItem);

    if (nargs >= 2) {
        // Two-argument form: "direction/reverse"
        const char* rev_raw = luaL_checkstring(L, 2);
        std::string_view rev_sv(rev_raw);

        if (rev_sv.empty() || containsInvalidChar(rev_sv))
            return luaReturnError(L, eBadMapItem);

        QString entry = QString::fromUtf8(dir_raw) + QChar('/') + QString::fromUtf8(rev_raw);
        pDoc->m_mapList.append(entry);
        return luaReturnOK(L);
    }

    // One-argument form: look up in direction table
    auto it = kDirectionMap.find(dir_sv);
    QString entry;
    QString reverseShort;

    if (it != kDirectionMap.end()) {
        entry = QString::fromLatin1(it->second.short_form);
        reverseShort = QString::fromLatin1(it->second.reverse);
    } else {
        entry = QString::fromUtf8(dir_raw);
    }

    // Auto-cancel reverses: if the last item is the opposite direction, pop it.
    if (pDoc->m_bRemoveMapReverses && !reverseShort.isEmpty() && !pDoc->m_mapList.isEmpty() &&
        pDoc->m_mapList.last() == reverseShort) {
        pDoc->m_mapList.removeLast();
        return luaReturnOK(L);
    }

    pDoc->m_mapList.append(entry);
    return luaReturnOK(L);
}

/**
 * world.AddMapperComment(comment)
 *
 * Appends a freeform comment to the map list.  The comment is stored as
 * "{comment}" so GetMappingString can distinguish it from direction entries.
 *
 * @return eOK (0) on success, eBadMapItem (30023) on invalid input.
 */
int L_AddMapperComment(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    const char* text = luaL_checkstring(L, 1);
    std::string_view sv(text);

    if (sv.empty() || containsInvalidChar(sv))
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
 * world.EnableMapping(enabled)
 *
 * Enables or disables the mapper.
 *
 * @param enabled (boolean) true to enable, false to disable
 * @return eOK (0)
 */
int L_EnableMapping(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    pDoc->m_bMapping = lua_toboolean(L, 1) != 0;
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
 * Returns an empty string if the index is out of range.
 *
 * @param index (number) 0-based index into the map list
 * @return (string) The stored entry string, or "" if out of range.
 */
int L_GetMappingItem(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    int index = static_cast<int>(luaL_checknumber(L, 1));

    if (index < 0 || index >= pDoc->m_mapList.size()) {
        lua_pushliteral(L, "");
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
 *     prefix: "3n", "2e".
 *   - Multi-char directions (ne/nw/se/sw) with count > 1 are parenthesized:
 *     "2(ne)".
 *   - Single occurrences are emitted without count: "n", "ne".
 *   - Special moves (entries containing "/") show only the forward part,
 *     parenthesized: "(climb tree)".
 *   - Comments ("{text}") are included as-is.
 *   - Tokens are joined with a single space.
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

    QStringList tokens;
    int i = 0;
    int total = list.size();

    while (i < total) {
        const QString& item = list.at(i);

        // Comment entry: starts with '{'
        if (item.startsWith(QChar('{'))) {
            tokens.append(item);
            ++i;
            continue;
        }

        // Special move: contains '/'
        if (item.contains(QChar('/'))) {
            int slash = item.indexOf(QChar('/'));
            QString forward = item.left(slash);
            tokens.append(QChar('(') + forward + QChar(')'));
            ++i;
            continue;
        }

        // Plain direction: count consecutive identical entries
        int run = 1;
        while (i + run < total && list.at(i + run) == item && !item.contains(QChar('/')) &&
               !item.startsWith(QChar('{'))) {
            ++run;
        }

        if (run == 1) {
            tokens.append(item);
        } else if (item.size() == 1) {
            // Single-char direction: "3n"
            tokens.append(QString::number(run) + item);
        } else {
            // Multi-char direction (ne/nw/se/sw): "2(ne)"
            tokens.append(QString::number(run) + QChar('(') + item + QChar(')'));
        }

        i += run;
    }

    QByteArray result = tokens.join(QChar(' ')).toUtf8();
    lua_pushlstring(L, result.constData(), result.size());
    return 1;
}

/**
 * world.Mapping([enabled])
 *
 * Dual get/set for the mapping active flag.
 *
 * With no arguments: returns the current boolean value.
 * With one argument: sets the flag and returns eOK.
 *
 * @param enabled (boolean, optional) New value for the flag
 * @return (boolean|number) Current flag (get) or eOK (set)
 */
int L_Mapping(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    if (lua_gettop(L) >= 1) {
        pDoc->m_bMapping = lua_toboolean(L, 1) != 0;
        return luaReturnOK(L);
    }

    lua_pushboolean(L, pDoc->m_bMapping ? 1 : 0);
    return 1;
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
 * world.RemoveMapReverses([enabled])
 *
 * Dual get/set for the auto-cancel-reverses flag.
 *
 * With no arguments: returns the current boolean value.
 * With one argument: sets the flag and returns eOK.
 *
 * @param enabled (boolean, optional) New value for the flag
 * @return (boolean|number) Current flag (get) or eOK (set)
 */
int L_RemoveMapReverses(lua_State* L)
{
    WorldDocument* pDoc = doc(L);

    if (lua_gettop(L) >= 1) {
        pDoc->m_bRemoveMapReverses = lua_toboolean(L, 1) != 0;
        return luaReturnOK(L);
    }

    lua_pushboolean(L, pDoc->m_bRemoveMapReverses ? 1 : 0);
    return 1;
}
