/**
 * world_sound.cpp - WorldDocument sound forwarding wrappers
 *
 * All sound logic has been moved to SoundManager (sound_manager.cpp).
 * The methods below are thin forwarding wrappers kept for source compatibility
 * with callers in world_trigger_execution.cpp, telnet_parser.cpp, and the
 * Lua API bindings in lua_api/world_sounds.cpp.
 *
 * See sound_manager.cpp for implementation details and documentation.
 */

#include "sound_manager.h"
#include "world_document.h"

qint32 WorldDocument::PlaySound(qint16 buffer, const QString& filename, bool loop, double volume,
                                double pan)
{
    return m_soundManager->playSound(buffer, filename, loop, volume, pan);
}

qint32 WorldDocument::StopSound(qint16 buffer)
{
    return m_soundManager->stopSound(buffer);
}

bool WorldDocument::PlaySoundFile(const QString& filename)
{
    return m_soundManager->playSoundFile(filename);
}

qint32 WorldDocument::GetSoundStatus(qint16 buffer)
{
    return m_soundManager->getSoundStatus(buffer);
}

bool WorldDocument::IsWindowActive()
{
    return m_soundManager->isWindowActive();
}
