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
#include "../../ui/views/output_view.h"
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

// ========== Error Codes ==========

// MUSHclient error codes (from errors.h)
// These match the original MUSHclient error codes for plugin compatibility
enum {
    eOK = 0,                            // no error
    eWorldOpen = 30001,                 // the world is already open
    eWorldClosed = 30002,               // the world is closed, this action cannot be performed
    eNoNameSpecified = 30003,           // no name has been specified where one is required
    eCannotPlaySound = 30004,           // the sound file could not be played
    eTriggerNotFound = 30005,           // the specified trigger name does not exist
    eTriggerAlreadyExists = 30006,      // attempt to add a trigger that already exists
    eTriggerCannotBeEmpty = 30007,      // the trigger "match" string cannot be empty
    eInvalidObjectLabel = 30008,        // the name of this object is invalid
    eScriptNameNotLocated = 30009,      // script name is not in the script file
    eAliasNotFound = 30010,             // the specified alias name does not exist
    eAliasAlreadyExists = 30011,        // attempt to add a alias that already exists
    eAliasCannotBeEmpty = 30012,        // the alias "match" string cannot be empty
    eCouldNotOpenFile = 30013,          // unable to open requested file
    eLogFileNotOpen = 30014,            // log file was not open
    eLogFileAlreadyOpen = 30015,        // log file was already open
    eLogFileBadWrite = 30016,           // bad write to log file
    eTimerNotFound = 30017,             // the specified timer name does not exist
    eTimerAlreadyExists = 30018,        // attempt to add a timer that already exists
    eVariableNotFound = 30019,          // attempt to delete a variable that does not exist
    eCommandNotEmpty = 30020,           // attempt to use SetCommand with a non-empty command window
    eBadRegularExpression = 30021,      // bad regular expression syntax
    eTimeInvalid = 30022,               // time given to AddTimer is invalid
    eBadMapItem = 30023,                // map item contains invalid characters, or is empty
    eNoMapItems = 30024,                // no items in mapper
    eUnknownOption = 30025,             // option name not found
    eOptionOutOfRange = 30026,          // new value for option is out of range
    eTriggerSequenceOutOfRange = 30027, // trigger sequence value invalid
    eTriggerSendToInvalid = 30028,      // where to send trigger text to is invalid
    eTriggerLabelNotSpecified = 30029, // trigger label not specified/invalid for 'send to variable'
    ePluginFileNotFound = 30030,       // file name specified for plugin not found
    eProblemsLoadingPlugin = 30031,    // there was a parsing or other problem loading the plugin
    ePluginCannotSetOption = 30032,    // plugin is not allowed to set this option
    ePluginCannotGetOption = 30033,    // plugin is not allowed to get this option
    eNoSuchPlugin = 30034,             // plugin is not installed
    eNotAPlugin = 30035,               // only a plugin can do this
    eNoSuchRoutine = 30036,            // plugin does not support that routine
    ePluginDoesNotSaveState = 30037,   // plugin does not support saving state
    ePluginCouldNotSaveState = 30038,  // plugin could not save state (eg. no directory)
    ePluginDisabled = 30039,           // plugin is currently disabled
    eErrorCallingPluginRoutine = 30040,  // could not call plugin routine
    eCommandsNestedTooDeeply = 30041,    // calls to "Execute" nested too deeply
    eBadParameter = 30046,               // general problem with a parameter to a script call
    eClipboardEmpty = 30050,             // cannot get (text from the) clipboard
    eFileNotFound = 30051,               // cannot open the specified file
    eAlreadyTransferringFile = 30052,    // already transferring a file
    eNotTransferringFile = 30053,        // not transferring a file
    eNoSuchCommand = 30054,              // there is not a command of that name
    eArrayAlreadyExists = 30055,         // that array already exists
    eArrayDoesNotExist = 30056,          // that array does not exist
    eArrayNotEvenNumberOfValues = 30057, // values to be imported into array are not in pairs
    eImportedWithDuplicates = 30058,     // import succeeded, however some values were overwritten
    eBadDelimiter =
        30059, // import/export delimiter must be a single character, other than backslash
    eSetReplacingExistingValue = 30060, // array element set, existing value overwritten
    eKeyDoesNotExist = 30061,           // array key does not exist
    eCannotImport = 30062, // cannot import because cannot find unused temporary character
    eItemInUse = 30063,    // cannot delete trigger/alias/timer because it is executing a script
    eSpellCheckNotActive = 30064, // spell checker is not active
    // Miniwindow error codes
    eCannotAddFont = 30065,         // cannot create requested font
    ePenStyleNotValid = 30066,      // invalid settings for pen parameter
    eUnableToLoadImage = 30067,     // bitmap image could not be loaded
    eImageNotInstalled = 30068,     // image has not been loaded into window
    eInvalidNumberOfPoints = 30069, // number of points supplied is incorrect
    eInvalidPoint = 30070,          // point is not numeric
    eHotspotPluginChanged = 30071,  // hotspot processing must all be in same plugin
    eHotspotNotInstalled = 30072,   // hotspot has not been defined for this window
    eNoSuchWindow = 30073,          // requested miniwindow does not exist
    eBrushStyleNotValid = 30074,    // invalid settings for brush parameter
    // Notepad error codes
    eNoSuchNotepad = 30075,     // requested notepad does not exist
    eFileNotOpened = 30076,     // could not open file for writing
    eInvalidColourName = 30077, // invalid color name or format
};

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
 * @param name Name to validate (will be trimmed)
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

#endif // LUA_COMMON_H
