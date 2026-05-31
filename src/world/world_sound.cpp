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

#include "../automation/plugin.h" // ON_PLUGIN_PLAY_SOUND
#include "sound_manager.h"
#include "world_document.h"

qint32 WorldDocument::PlaySound(qint16 buffer, const QString& filename, bool loop, double volume,
                                double pan)
{
    return m_soundManager->playSound(buffer, filename, loop, volume, pan);
}

qint32 WorldDocument::StopSound(qint16 buffer)
{
    // Pure DirectSound buffer stop — no plugin callback.
    // Original: CMUSHclientDoc::StopSound (methods_sounds.cpp:361-402) fires no callback;
    // the OnPluginPlaySound notification belongs to CancelSound, not StopSound.
    return m_soundManager->stopSound(buffer);
}

void WorldDocument::CancelSound()
{
    // Fire OnPluginPlaySound with the empty string so plugins can suppress the cancel.
    // If a plugin handles it (returns true), skip the stop entirely — matching the
    // original CMUSHclientDoc::CancelSound (doc.cpp:7135-7158).
    if (!m_bInCancelSoundFilePlugin) {
        m_bInCancelSoundFilePlugin = true;
        const bool handled = SendToFirstPluginCallbacks(ON_PLUGIN_PLAY_SOUND, QString());
        m_bInCancelSoundFilePlugin = false;
        if (handled) {
            return; // handled by plugin? don't do our own sound cancel
        }
    }

    // default sound-cancel mechanism
    StopSound(0);
}

bool WorldDocument::PlaySoundFile(const QString& filename)
{
    // Let plugins intercept/suppress sound playback.
    // Original: doc.cpp:7115-7130 — SendToFirstPluginCallbacks with recursion guard.
    if (!m_bInPlaySoundFilePlugin) {
        m_bInPlaySoundFilePlugin = true;
        bool handled = SendToFirstPluginCallbacks(ON_PLUGIN_PLAY_SOUND, filename);
        m_bInPlaySoundFilePlugin = false;
        if (handled) {
            return true; // Plugin handled the sound
        }
    }

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
