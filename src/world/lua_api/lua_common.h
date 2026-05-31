/**
 * lua_common.h - Common definitions for Lua API
 *
 * This header contains common includes, error codes, flag enums,
 * and helper functions used across the Lua API implementation.
 *
 * Extracted from lua_methods.cpp for modular organization.
 */

#ifndef LUA_COMMON_H
#define LUA_COMMON_H

#include "../../automation/alias.h"
#include "../../automation/plugin.h"
#include "../../automation/sendto.h"
#include "../../automation/timer.h"
#include "../../automation/trigger.h"
#include "../../world/config_options.h"
#include "../../world/world_document.h"
#include "../script_engine.h"
#include "miniwindow.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QCursor>
#include <QDateTime>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMenu>

// Lua 5.1 C headers (lua.hpp doesn't exist in Lua 5.1)
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include "../../utils/error_codes.h"

// ========== Flag Enums ==========

// Trigger flag constants (from original flags.h)
enum {
    eEnabled = 0x01,
    eOmitFromLog = 0x02,
    eOmitFromOutput = 0x04,
    eKeepEvaluating = 0x08,
    eIgnoreCase = 0x10,
    eTriggerRegularExpression = 0x20,
    eExpandVariables = 0x200,
    eReplace = 0x400,
    eLowercaseWildcard = 0x800,
    eTemporary = 0x4000,
    eTriggerOneShot = 0x8000,
};

// Alias flag constants
enum {
    // eEnabled = 0x01,  // same as trigger
    eUseClipboard = 0x02,
    // eOmitFromLogFile kept for backward compatibility below
    // eIgnoreCase conflicts, use eIgnoreAliasCase
    eIgnoreAliasCase = 0x20,
    eOmitFromLogFile = 0x40,
    eAliasRegularExpression = 0x80,
    eAliasOmitFromOutput = 0x100,
    // eExpandVariables = 0x200,  // same as trigger
    // eReplace = 0x400,  // same as trigger
    eAliasSpeedWalk = 0x800,
    eAliasQueue = 0x1000,
    eAliasMenu = 0x2000,
    // eTemporary = 0x4000,  // same as trigger
    eAliasOneShot = 0x8000,
};

// Timer flag constants
enum {
    eTimerEnabled = 1,
    eTimerAtTime = 2,
    eTimerOneShot = 4,
    eTimerTemporary = 8,
    eTimerActiveWhenClosed = 16,
    eTimerReplace = 32,
    eTimerSpeedWalk = 64,
    eTimerNote = 128,
};

// ========== Helper Functions ==========

/** Return an error code from a Lua function */
inline int luaReturnError(lua_State* L, int errorCode)
{
    lua_pushnumber(L, errorCode);
    return 1;
}

/** Return eOK (success) from a Lua function */
inline int luaReturnOK(lua_State* L)
{
    lua_pushnumber(L, eOK);
    return 1;
}

/** Return nil from a Lua function */
inline int luaReturnNil(lua_State* L)
{
    lua_pushnil(L);
    return 1;
}

/** Get WorldDocument from Lua registry */
inline WorldDocument* doc(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, DOCUMENT_STATE);
    WorldDocument* pDoc = static_cast<WorldDocument*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return pDoc;
}

/** Get Plugin from Lua registry (returns nullptr for world Lua state) */
inline Plugin* plugin(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, PLUGIN_STATE);
    Plugin* pPlugin = static_cast<Plugin*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return pPlugin;
}

/**
 * concatArgs - Concatenate all Lua function arguments into QString
 */
inline QString concatArgs(lua_State* L, const QString& delimiter = "", int first = 1)
{
    int n = lua_gettop(L);
    QStringList parts;

    for (int i = first; i <= n; i++) {
        size_t len;
        const char* s = lua_tolstring(L, i, &len);

        if (s) {
            parts.append(QString::fromUtf8(s, len));
        } else {
            const char* tname = lua_typename(L, lua_type(L, i));
            parts.append(QString("[%1]").arg(tname));
        }
    }

    return parts.join(delimiter);
}

/**
 * ColourNameToRGB - Convert color name to RGB value
 */
QRgb ColourNameToRGB(const QString& name);

/**
 * RGBColourToName - Convert RGB value to color name
 */
QString RGBColourToName(QRgb rgb);

/**
 * getColor - Get color from Lua argument (accepts string name or integer RGB)
 *
 * @param L Lua state
 * @param index Argument index
 * @param defaultColor Default color if nil
 * @return RGB color value
 */
inline QRgb getColor(lua_State* L, int index, QRgb defaultColor = qRgb(255, 255, 255))
{
    if (lua_isnil(L, index)) {
        return defaultColor;
    }

    if (lua_isnumber(L, index)) {
        // Direct RGB integer
        return (QRgb)lua_tointeger(L, index);
    }

    if (lua_isstring(L, index)) {
        // Color name
        const char* name = lua_tostring(L, index);
        // Empty string means "use default" (matches original MUSHclient behavior)
        if (name[0] == '\0') {
            return defaultColor;
        }
        return ColourNameToRGB(QString::fromUtf8(name));
    }

    return defaultColor;
}

/**
 * validateObjectName - Validate object name format
 *
 * Based on CheckLabel() and CheckObjectName()
 * Rules:
 * - Cannot be empty
 * - Must start with a letter
 * - Can only contain letters, digits, and underscores
 *
 * @param name Name to validate (will be trimmed and lowercased on success)
 * @return eOK (0) if valid, eInvalidObjectLabel (30008) if invalid
 */
inline qint32 validateObjectName(QString& name)
{
    // Trim leading and trailing whitespace (matches CheckObjectName behavior)
    name = name.trimmed();

    // Check if empty
    if (name.isEmpty()) {
        return eInvalidObjectLabel;
    }

    // First character must be a letter
    if (!name.at(0).isLetter()) {
        return eInvalidObjectLabel;
    }

    // Subsequent characters must be alphanumeric or underscore
    for (int i = 1; i < name.length(); i++) {
        QChar ch = name.at(i);
        if (!ch.isLetterOrNumber() && ch != '_') {
            return eInvalidObjectLabel;
        }
    }

    // Object names are keyed in lower case (matches CheckObjectName::MakeLower)
    name = name.toLower();

    return eOK;
}

/**
 * validateScriptLabel - Validate a Lua script callback name
 *
 * Mirrors CheckLabel(label, bScript=true) from original doc.cpp:3863.
 * Same rules as validateObjectName but dots are also permitted, supporting
 * module.function style Lua identifiers (e.g., "mymod.OnClick").
 *
 * @param name Script callback name to validate (not modified)
 * @return eOK (0) if valid, eInvalidObjectLabel (30008) if invalid
 */
inline qint32 validateScriptLabel(const QString& name)
{
    if (name.isEmpty()) {
        return eInvalidObjectLabel;
    }

    // First character must be a letter
    if (!name.at(0).isLetter()) {
        return eInvalidObjectLabel;
    }

    for (int i = 1; i < name.length(); i++) {
        QChar ch = name.at(i);
        if (!ch.isLetterOrNumber() && ch != '_' && ch != '.') {
            return eInvalidObjectLabel;
        }
    }

    return eOK;
}

/**
 * PushJsonValue - Push QJsonValue onto Lua stack
 *
 * Recursively converts QJsonValue to Lua types:
 * - QJsonObject -> Lua table (string keys)
 * - QJsonArray -> Lua table (numeric keys, 1-indexed)
 * - QString -> Lua string
 * - double -> Lua number
 * - bool -> Lua boolean
 * - null -> Lua nil
 *
 * Used by GMCP functions to convert JSON messages to Lua tables.
 *
 * @param L Lua state
 * @param val QJsonValue to convert
 */
inline void PushJsonValue(lua_State* L, const QJsonValue& val)
{
    if (val.isObject()) {
        QJsonObject obj = val.toObject();
        lua_newtable(L);

        for (auto it = obj.begin(); it != obj.end(); ++it) {
            QString key = it.key();
            QByteArray keyBytes = key.toUtf8();
            lua_pushlstring(L, keyBytes.constData(), keyBytes.length());
            PushJsonValue(L, it.value());
            lua_settable(L, -3);
        }
    } else if (val.isArray()) {
        QJsonArray arr = val.toArray();
        lua_newtable(L);

        for (int i = 0; i < arr.size(); i++) {
            lua_pushnumber(L, i + 1); // Lua uses 1-based indexing
            PushJsonValue(L, arr[i]);
            lua_settable(L, -3);
        }
    } else if (val.isString()) {
        QString str = val.toString();
        QByteArray strBytes = str.toUtf8();
        lua_pushlstring(L, strBytes.constData(), strBytes.length());
    } else if (val.isDouble()) {
        lua_pushnumber(L, val.toDouble());
    } else if (val.isBool()) {
        lua_pushboolean(L, val.toBool());
    } else {
        lua_pushnil(L);
    }
}

// ========== Lua <-> QString Conversion Helpers ==========

/** Extract required string argument as QString */
inline QString luaCheckQString(lua_State* L, int idx)
{
    return QString::fromUtf8(luaL_checkstring(L, idx));
}

/** Extract optional string argument as QString */
inline QString luaOptQString(lua_State* L, int idx, const char* def = "")
{
    return QString::fromUtf8(luaL_optstring(L, idx, def));
}

/** Push QString to Lua stack (binary-safe) */
inline void luaPushQString(lua_State* L, const QString& str)
{
    QByteArray ba = str.toUtf8();
    lua_pushlstring(L, ba.constData(), ba.length());
}

/** Push QStringList to Lua as 1-indexed table */
inline void luaPushQStringList(lua_State* L, const QStringList& list)
{
    lua_newtable(L);
    for (int i = 0; i < list.size(); i++) {
        luaPushQString(L, list[i]);
        lua_rawseti(L, -2, i + 1);
    }
}

// ========== Validation Macros ==========

/** Validate condition or return error code */
#define LUA_VALIDATE(cond, err)                                                                    \
    do {                                                                                           \
        if (!(cond))                                                                               \
            return luaReturnError(L, err);                                                         \
    } while (0)

/** Validate object name or return error code */
#define LUA_VALIDATE_NAME(name)                                                                    \
    do {                                                                                           \
        qint32 _status = validateObjectName(name);                                                 \
        if (_status != eOK)                                                                        \
            return luaReturnError(L, _status);                                                     \
    } while (0)

#include "lua_bind.h"

#endif // LUA_COMMON_H
