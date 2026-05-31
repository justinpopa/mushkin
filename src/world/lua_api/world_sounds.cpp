/**
 * world_sounds.cpp - Sound Playback API
 *
 * Lua bindings for sound system:
 * - world.PlaySound(buffer, filename, loop, volume, pan)
 * - world.StopSound(buffer)
 * - world.Sound(filename)
 */

#include "../sound_manager.h"
#include "lua_common.h"
#include <QDir>
#include <QFile>
#include <QStandardPaths>

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
    bool loop = lua_toboolean(L, 3);
    double volume = luaL_optnumber(L, 4, 0.0); // Default: full volume
    double pan = luaL_optnumber(L, 5, 0.0);    // Default: center

    // Call PlaySound
    // filename is optional — nil/empty means "adjust existing playing sound" (original behavior)
    return luaReturn(L, pDoc->PlaySound(buffer, luaOptQString(L, 2), loop, volume, pan));
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
    return luaReturn(L, pDoc->StopSound(buffer));
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

    auto [filename] = luaArgs<QString>(L);

    // Original methods_sounds.cpp:21-22 — empty filename → eNoNameSpecified
    if (filename.isEmpty()) {
        return luaReturn(L, eNoNameSpecified);
    }

    // Call PlaySoundFile (finds first available buffer)
    return luaReturn(L, pDoc->PlaySoundFile(filename) ? eOK : eCannotPlaySound);
}

/**
 * world.PlaySoundMemory(buffer, data, loop, volume, pan)
 *
 * Play a sound from an in-memory buffer. In the original MUSHclient this
 * passed raw PCM/WAV data directly to the sound driver. Mushkin uses
 * Qt Multimedia which requires a file path or a QMediaContent source and
 * does not support raw memory buffers in a cross-platform way.
 *
 * This function is a stub that returns eCannotPlaySound to inform the
 * caller that memory-buffer playback is not supported. Use PlaySound()
 * with a file path instead.
 *
 * @param buffer (number) Buffer slot number (1-10)
 * @param data (string) Raw audio data (binary — ignored in this implementation)
 * @param loop (boolean) Whether to loop (optional)
 * @param volume (number) Volume -100..0 (optional)
 * @param pan (number) Stereo panning -100..100 (optional)
 *
 * @return (number) Error code:
 *   - eCannotPlaySound (30004): Memory-buffer playback not supported
 *
 * @example
 * -- This will always fail; use PlaySound with a file path instead
 * local result = PlaySoundMemory(1, audio_data)
 * if result ~= 0 then
 *     Note("Memory sound not supported, using file")
 *     PlaySound(1, "sounds/alert.wav", false, 0, 0)
 * end
 *
 * @see PlaySound, Sound, StopSound
 */
int L_PlaySoundMemory(lua_State* L)
{
    WorldDocument* pDoc = doc(L);
    if (!pDoc) {
        return luaL_error(L, "No world document");
    }

    // Original: lua_methods.cpp:4510-4527 — calls PlaySoundHelper with memory buffer
    // buffer = arg 1, data = arg 2 (string), loop = arg 3, volume = arg 4, pan = arg 5
    qint16 buffer = static_cast<qint16>(luaL_optinteger(L, 1, 0));
    size_t dataLen = 0;
    const char* data = luaL_checklstring(L, 2, &dataLen);
    bool loop = lua_toboolean(L, 3);
    double volume = luaL_optnumber(L, 4, 0.0); // 0 = full volume
    double pan = luaL_optnumber(L, 5, 0.0);    // 0 = center

    if (!data || dataLen == 0) {
        return luaReturn(L, eCannotPlaySound);
    }

    // Write memory buffer to a unique temp file (avoids race condition with concurrent calls)
    // Qt Multimedia doesn't support raw memory buffer playback directly
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString tempPath = tempDir + QString("/mushkin_sound_mem_%1.wav").arg(buffer);

    QFile tempFile(tempPath);
    if (!tempFile.open(QIODevice::WriteOnly)) {
        return luaReturn(L, eCannotPlaySound);
    }
    tempFile.write(data, static_cast<qint64>(dataLen));
    tempFile.close();

    // Play with all parameters (buffer, loop, volume, pan)
    qint32 result = pDoc->m_soundManager->playSound(buffer, tempPath, loop, volume, pan);
    return luaReturn(L, result);
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
    return luaReturn(L, pDoc->GetSoundStatus(buffer));
}
