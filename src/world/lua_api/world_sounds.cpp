/**
 * world_sounds.cpp - Sound Playback API
 *
 * Lua bindings for sound system:
 * - world.PlaySound(buffer, filename, loop, volume, pan)
 * - world.StopSound(buffer)
 * - world.Sound(filename)
 */

#include "lua_common.h"

/**
 * world.PlaySound(buffer, filename, loop, volume, pan)
 *
 * Plays a sound file in a specific buffer with full control.
 *
 * @param buffer Buffer number (0 = auto-select, 1-10 = specific buffer)
 * @param filename Path to sound file (WAV, MP3, OGG, FLAC, etc.)
 * @param loop Loop playback? (boolean)
 * @param volume Volume (-100 = silent, 0 = full volume)
 * @param pan Panning (-100 = full left, 0 = center, +100 = full right)
 *
 * @return eOK (0) on success, error code on failure
 *
 * Example:
 *   world.PlaySound(1, "sounds/ding.wav", false, 0, 0)
 *   world.PlaySound(0, "sounds/alert.wav", false, -50, 0)  -- auto-select buffer, half volume
 *   world.PlaySound(2, "sounds/step.wav", false, 0, -100)  -- full left pan
 */
int L_PlaySound(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    if (!pDoc) {
        return luaL_error(L, "No world document");
    }

    // Get parameters
    qint16 buffer = luaL_checkinteger(L, 1);
    const char* filename = luaL_checkstring(L, 2);
    bool loop = lua_toboolean(L, 3);
    double volume = luaL_optnumber(L, 4, 0.0); // Default: full volume
    double pan = luaL_optnumber(L, 5, 0.0);    // Default: center

    // Call PlaySound
    qint32 result = pDoc->PlaySound(buffer, QString::fromUtf8(filename), loop, volume, pan);

    lua_pushinteger(L, result);
    return 1;
}

/**
 * world.StopSound(buffer)
 *
 * Stops sound playback in a specific buffer.
 *
 * @param buffer Buffer number (0 = stop all, 1-10 = specific buffer)
 *
 * @return eOK (0) on success, error code on failure
 *
 * Example:
 *   world.StopSound(1)  -- Stop buffer 1
 *   world.StopSound(0)  -- Stop all sounds
 */
int L_StopSound(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    if (!pDoc) {
        return luaL_error(L, "No world document");
    }

    // Get buffer parameter
    qint16 buffer = luaL_checkinteger(L, 1);

    // Call StopSound
    qint32 result = pDoc->StopSound(buffer);

    lua_pushinteger(L, result);
    return 1;
}

/**
 * world.Sound(filename)
 *
 * Simple sound playback - plays a sound file in the first available buffer.
 * Uses default settings: no loop, full volume, center panning.
 *
 * @param filename Path to sound file (WAV format)
 *
 * @return eOK (0) on success, eCannotPlaySound (30004) on failure
 *
 * Example:
 *   result = world.Sound("sounds/alert.wav")
 *   if result == 0 then print("Sound played successfully") end
 */
int L_Sound(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    if (!pDoc) {
        return luaL_error(L, "No world document");
    }

    // Get filename parameter
    const char* filename = luaL_checkstring(L, 1);

    // Call PlaySoundFile (finds first available buffer)
    bool success = pDoc->PlaySoundFile(QString::fromUtf8(filename));

    // Return error code (eOK or eCannotPlaySound) for API compatibility
    lua_pushinteger(L, success ? eOK : eCannotPlaySound);
    return 1;
}

/**
 * world.GetSoundStatus(buffer)
 *
 * Query the status of a sound buffer.
 *
 * @param buffer Buffer number (1-10, 1-based)
 *
 * @return Status code:
 *         -2: Buffer is free (no sound loaded)
 *         -1: Buffer out of range
 *          0: Sound is not playing
 *          1: Sound is playing but not looping
 *          2: Sound is playing and looping
 *
 * Example:
 *   status = world.GetSoundStatus(1)
 *   if status == 1 then print("Sound playing") end
 *   if status == 2 then print("Sound looping") end
 */
int L_GetSoundStatus(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    if (!pDoc) {
        return luaL_error(L, "No world document");
    }

    // Get buffer parameter (1-based)
    qint16 buffer = luaL_checkinteger(L, 1);

    // Call GetSoundStatus
    qint32 status = pDoc->GetSoundStatus(buffer);

    lua_pushinteger(L, status);
    return 1;
}
