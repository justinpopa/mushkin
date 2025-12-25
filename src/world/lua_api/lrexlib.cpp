/* lrexlib.cpp - PCRE regular expression library for Lua
 * Ported from original MUSHclient lrexlib.c by Reuben Thomas and Shmuel Zeigerman
 *
 * Implements:
 *   rex.new(pattern, [flags], [locale]) - Compile a PCRE pattern
 *   rex.flags() - Get table of available compilation flags
 *   rex.version() - Get PCRE version string
 *
 *   re:exec(text, [startoffset], [flags]) - Match pattern, returns byte offsets
 *   re:match(text, [startoffset], [flags]) - Match pattern, returns substrings
 *   re:gmatch(text, function, [maxmatches], [flags]) - Global match with callback
 */

#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <pcre.h>
}

static void L_lua_error(lua_State* L, const char* message)
{
    lua_pushstring(L, message);
    lua_error(L);
}

static void* Lmalloc(lua_State* L, size_t size)
{
    void* p = malloc(size);
    if (p == nullptr)
        L_lua_error(L, "malloc failed");
    return p;
}

static int get_startoffset(lua_State* L, int stackpos, size_t len)
{
    int startoffset = luaL_optint(L, stackpos, 1);
    if (startoffset > 0)
        startoffset--;
    else if (startoffset < 0) {
        startoffset += static_cast<int>(len);
        if (startoffset < 0)
            startoffset = 0;
    }
    return startoffset;
}

static int regex_tostring(lua_State* L, const char* type_handle, const char* type_name)
{
    char buf[256];
    void* udata = luaL_checkudata(L, 1, type_handle);
    if (udata) {
        snprintf(buf, sizeof(buf), "%s (%p)", type_name, udata);
        lua_pushstring(L, buf);
    } else {
        snprintf(buf, sizeof(buf), "must be userdata of type '%s'", type_name);
        luaL_argerror(L, 1, buf);
    }
    return 1;
}

struct flags_pair {
    const char* key;
    int val;
};

static int regex_get_flags(lua_State* L, const flags_pair* arr)
{
    lua_newtable(L);
    for (const flags_pair* p = arr; p->key != nullptr; p++) {
        lua_pushstring(L, p->key);
        lua_pushnumber(L, p->val);
        lua_rawset(L, -3);
    }
    return 1;
}

// PCRE implementation

static const char pcre_handle[] = "pcre_regex_handle";
static const char pcre_typename[] = "pcre_regex";

struct pcre2_ud {
    pcre* pr;
    pcre_extra* extra;
    int* match;
    int ncapt;
    const unsigned char* tables;
};

static const unsigned char* Lpcre_maketables(lua_State* L, int stackpos)
{
    char old_locale[256];
    const char* locale = luaL_checkstring(L, stackpos);

    strncpy(old_locale, setlocale(LC_CTYPE, nullptr), sizeof(old_locale) - 1);
    old_locale[sizeof(old_locale) - 1] = '\0';

    if (nullptr == setlocale(LC_CTYPE, locale))
        L_lua_error(L, "cannot set locale");

    const unsigned char* tables = pcre_maketables();
    setlocale(LC_CTYPE, old_locale);
    return tables;
}

static int Lpcre_comp(lua_State* L)
{
    char buf[256];
    const char* error;
    int erroffset;
    size_t clen;
    const char* pattern = luaL_checklstring(L, 1, &clen);
    int cflags = luaL_optint(L, 2, 0);
    const unsigned char* tables = nullptr;

    if (lua_gettop(L) > 2 && !lua_isnil(L, 3))
        tables = Lpcre_maketables(L, 3);

    pcre2_ud* ud = static_cast<pcre2_ud*>(lua_newuserdata(L, sizeof(pcre2_ud)));
    luaL_getmetatable(L, pcre_handle);
    lua_setmetatable(L, -2);
    ud->match = nullptr;
    ud->extra = nullptr;
    ud->tables = tables;
    ud->pr = pcre_compile(pattern, cflags, &error, &erroffset, tables);
    if (!ud->pr) {
        snprintf(buf, sizeof(buf), "%s (pattern offset: %d)", error, erroffset + 1);
        L_lua_error(L, buf);
    }

    ud->extra = pcre_study(ud->pr, 0, &error);
    if (error)
        L_lua_error(L, error);

    pcre_fullinfo(ud->pr, ud->extra, PCRE_INFO_CAPTURECOUNT, &ud->ncapt);
    ud->match = static_cast<int*>(Lmalloc(L, (ud->ncapt + 1) * 3 * sizeof(int)));

    return 1;
}

static void Lpcre_getargs(lua_State* L, pcre2_ud** pud, const char** text, size_t* text_len)
{
    *pud = static_cast<pcre2_ud*>(luaL_checkudata(L, 1, pcre_handle));
    luaL_argcheck(L, *pud != nullptr, 1, "compiled regexp expected");
    *text = luaL_checklstring(L, 2, text_len);
}

typedef void (*Lpcre_push_matches)(lua_State* L, const char* text, pcre2_ud* ud);

static void Lpcre_push_substrings(lua_State* L, const char* text, pcre2_ud* ud)
{
    int namecount;
    unsigned char* name_table;
    int name_entry_size;
    const int* match = ud->match;

    lua_newtable(L);
    for (int i = 1; i <= ud->ncapt; i++) {
        int j = i * 2;
        if (match[j] >= 0)
            lua_pushlstring(L, text + match[j], match[j + 1] - match[j]);
        else
            lua_pushboolean(L, 0);
        lua_rawseti(L, -2, i);
    }

    // Named subpatterns
    pcre_fullinfo(ud->pr, ud->extra, PCRE_INFO_NAMECOUNT, &namecount);
    if (namecount <= 0)
        return;
    pcre_fullinfo(ud->pr, ud->extra, PCRE_INFO_NAMETABLE, &name_table);
    pcre_fullinfo(ud->pr, ud->extra, PCRE_INFO_NAMEENTRYSIZE, &name_entry_size);

    // First add every name as a non-match (for duplicates)
    unsigned char* tabptr = name_table;
    for (int i = 0; i < namecount; i++) {
        int n = (static_cast<int>(tabptr[0]) << 8) | tabptr[1];
        const char* name = reinterpret_cast<const char*>(tabptr + 2);
        if (n >= 0 && n <= ud->ncapt) {
            lua_pushstring(L, name);
            lua_pushboolean(L, 0);
            lua_settable(L, -3);
        }
        tabptr += name_entry_size;
    }

    // Now add the actual matches
    tabptr = name_table;
    for (int i = 0; i < namecount; i++) {
        int n = (static_cast<int>(tabptr[0]) << 8) | tabptr[1];
        const char* name = reinterpret_cast<const char*>(tabptr + 2);
        if (n >= 0 && n <= ud->ncapt) {
            int j = n * 2;
            if (match[j] >= 0) {
                lua_pushstring(L, name);
                lua_pushlstring(L, text + match[j], match[j + 1] - match[j]);
                lua_settable(L, -3);
            }
        }
        tabptr += name_entry_size;
    }
}

static void Lpcre_push_offsets(lua_State* L, const char* text, pcre2_ud* ud)
{
    (void)text; // unused
    lua_newtable(L);
    for (int i = 1, j = 1; i <= ud->ncapt; i++) {
        int k = i * 2;
        if (ud->match[k] >= 0) {
            lua_pushnumber(L, ud->match[k] + 1);
            lua_rawseti(L, -2, j++);
            lua_pushnumber(L, ud->match[k + 1]);
            lua_rawseti(L, -2, j++);
        } else {
            lua_pushboolean(L, 0);
            lua_rawseti(L, -2, j++);
            lua_pushboolean(L, 0);
            lua_rawseti(L, -2, j++);
        }
    }
}

// Callout function handlers
static int callout_function_x(pcre_callout_block* cb, int f_loc)
{
    int (*f)(pcre_callout_block*) = pcre_callout;
    lua_State* L = static_cast<lua_State*>(cb->callout_data);
    if (!L)
        return 0;

    lua_pushvalue(L, f_loc);
    lua_pushnumber(L, cb->callout_number);

    // Offset vectors
    lua_newtable(L);
    for (int i = 1, j = 1; i < cb->capture_top; i++) {
        int k = i * 2;
        if (cb->offset_vector[k] >= 0) {
            lua_pushnumber(L, cb->offset_vector[k] + 1);
            lua_rawseti(L, -2, j++);
            lua_pushnumber(L, cb->offset_vector[k + 1]);
            lua_rawseti(L, -2, j++);
        } else {
            lua_pushboolean(L, 0);
            lua_rawseti(L, -2, j++);
            lua_pushboolean(L, 0);
            lua_rawseti(L, -2, j++);
        }
    }

    lua_pushlstring(L, cb->subject, cb->subject_length);
    lua_pushnumber(L, cb->start_match + 1);
    lua_pushnumber(L, cb->current_position + 1);
    lua_pushnumber(L, cb->capture_top - 1);
    lua_pushnumber(L, cb->capture_last);

    lua_call(L, 7, 1);
    int result = static_cast<int>(lua_tonumber(L, -1));
    lua_pop(L, 1);

    pcre_callout = f;

    if (result < 0)
        result = PCRE_ERROR_NOMATCH;
    return result;
}

static int callout_function5(pcre_callout_block* cb)
{
    return callout_function_x(cb, 5);
}

static int callout_function6(pcre_callout_block* cb)
{
    return callout_function_x(cb, 6);
}

static void check_for_callout(lua_State* L, pcre2_ud* ud, int which, int (*f)(pcre_callout_block*))
{
    pcre_callout = nullptr;

    if (lua_isfunction(L, which)) {
        if (!ud->extra) {
            ud->extra = static_cast<pcre_extra*>(pcre_malloc(sizeof(pcre_extra)));
            if (ud->extra == nullptr)
                L_lua_error(L, "failed to get memory for PCRE callback");
            memset(ud->extra, 0, sizeof(pcre_extra));
        }
        ud->extra->callout_data = L;
        ud->extra->flags |= PCRE_EXTRA_CALLOUT_DATA;
        pcre_callout = f;
    }
}

static int Lpcre_match_generic(lua_State* L, Lpcre_push_matches push_matches)
{
    const char* text;
    pcre2_ud* ud;
    size_t elen;
    int eflags = luaL_optint(L, 4, 0);

    Lpcre_getargs(L, &ud, &text, &elen);
    int startoffset = get_startoffset(L, 3, elen);

    check_for_callout(L, ud, 5, callout_function5);

    int res = pcre_exec(ud->pr, ud->extra, text, static_cast<int>(elen), startoffset, eflags,
                        ud->match, (ud->ncapt + 1) * 3);
    if (res >= 0) {
        lua_pushnumber(L, ud->match[0] + 1);
        lua_pushnumber(L, ud->match[1]);
        (*push_matches)(L, text, ud);
        return 3;
    }
    return 0;
}

static int Lpcre_match(lua_State* L)
{
    return Lpcre_match_generic(L, Lpcre_push_substrings);
}

static int Lpcre_exec(lua_State* L)
{
    return Lpcre_match_generic(L, Lpcre_push_offsets);
}

static int Lpcre_gmatch(lua_State* L)
{
    size_t len;
    int nmatch = 0, limit = 0;
    const char* text;
    pcre2_ud* ud;
    int maxmatch = luaL_optint(L, 4, 0);
    int eflags = luaL_optint(L, 5, 0);
    int startoffset = 0;

    Lpcre_getargs(L, &ud, &text, &len);
    luaL_checktype(L, 3, LUA_TFUNCTION);

    if (maxmatch > 0)
        limit = 1;

    check_for_callout(L, ud, 6, callout_function6);

    while (!limit || nmatch < maxmatch) {
        int res = pcre_exec(ud->pr, ud->extra, text, static_cast<int>(len), startoffset, eflags,
                            ud->match, (ud->ncapt + 1) * 3);
        if (res >= 0) {
            int (*f)(pcre_callout_block*) = pcre_callout;

            nmatch++;
            lua_pushvalue(L, 3);
            lua_pushlstring(L, text + ud->match[0], ud->match[1] - ud->match[0]);
            Lpcre_push_substrings(L, text, ud);
            lua_call(L, 2, 1);
            if (lua_toboolean(L, -1))
                break;
            lua_pop(L, 1);
            startoffset = ud->match[1];
            pcre_callout = f;
        } else {
            break;
        }
    }
    lua_pushnumber(L, nmatch);
    return 1;
}

static int Lpcre_gc(lua_State* L)
{
    pcre2_ud* ud = static_cast<pcre2_ud*>(luaL_checkudata(L, 1, pcre_handle));
    if (ud) {
        if (ud->pr)
            pcre_free(ud->pr);
        if (ud->extra)
            pcre_free(ud->extra);
        if (ud->tables)
            pcre_free(const_cast<unsigned char*>(ud->tables));
        if (ud->match)
            free(ud->match);
    }
    return 0;
}

static int Lpcre_tostring(lua_State* L)
{
    return regex_tostring(L, pcre_handle, pcre_typename);
}

static int Lpcre_vers(lua_State* L)
{
    lua_pushstring(L, pcre_version());
    return 1;
}

static flags_pair pcre_flags[] = {{"CASELESS", PCRE_CASELESS},
                                  {"MULTILINE", PCRE_MULTILINE},
                                  {"DOTALL", PCRE_DOTALL},
                                  {"EXTENDED", PCRE_EXTENDED},
                                  {"ANCHORED", PCRE_ANCHORED},
                                  {"DOLLAR_ENDONLY", PCRE_DOLLAR_ENDONLY},
                                  {"EXTRA", PCRE_EXTRA},
                                  {"NOTBOL", PCRE_NOTBOL},
                                  {"NOTEOL", PCRE_NOTEOL},
                                  {"UNGREEDY", PCRE_UNGREEDY},
                                  {"NOTEMPTY", PCRE_NOTEMPTY},
                                  {"UTF8", PCRE_UTF8},
                                  {"AUTO_CALLOUT", PCRE_AUTO_CALLOUT},
#if PCRE_MAJOR >= 4
                                  {"NO_AUTO_CAPTURE", PCRE_NO_AUTO_CAPTURE},
#endif
#if PCRE_MAJOR >= 7
                                  {"PARTIAL", PCRE_PARTIAL},
                                  {"FIRSTLINE", PCRE_FIRSTLINE},
                                  {"DUPNAMES", PCRE_DUPNAMES},
                                  {"NEWLINE_CR", PCRE_NEWLINE_CR},
                                  {"NEWLINE_LF", PCRE_NEWLINE_LF},
                                  {"NEWLINE_CRLF", PCRE_NEWLINE_CRLF},
                                  {"NEWLINE_ANY", PCRE_NEWLINE_ANY},
                                  {"NEWLINE_ANYCRLF", PCRE_NEWLINE_ANYCRLF},
#endif
                                  {nullptr, 0}};

static int Lpcre_get_flags(lua_State* L)
{
    return regex_get_flags(L, pcre_flags);
}

static const luaL_Reg pcremeta[] = {{"exec", Lpcre_exec},           {"match", Lpcre_match},
                                    {"gmatch", Lpcre_gmatch},       {"__gc", Lpcre_gc},
                                    {"__tostring", Lpcre_tostring}, {nullptr, nullptr}};

static const luaL_Reg rexlib[] = {
    {"new", Lpcre_comp}, {"flags", Lpcre_get_flags}, {"version", Lpcre_vers}, {nullptr, nullptr}};

static void createmeta(lua_State* L, const char* name)
{
    luaL_newmetatable(L, name);
    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -2);
    lua_rawset(L, -3);
}

extern "C" int luaopen_rex(lua_State* L)
{
    createmeta(L, pcre_handle);
    luaL_register(L, nullptr, pcremeta);
    lua_pop(L, 1);
    luaL_register(L, "rex", rexlib);
    return 1;
}
