/**
 * world_sound.cpp - Sound System Implementation
 *
 * Implements sound playback using Qt Spatial Audio (QAudioEngine/QSpatialSound)
 * with full stereo panning support.
 *
 * BUFFER INDEXING (matches original MUSHclient)
 * =============================================
 * - Buffer 0: Special - auto-select free buffer (PlaySound) or stop all (StopSound)
 * - Buffers 1-10: Specific buffer numbers (converted to 0-9 internally)
 *
 * VOLUME/PAN SCALES (matches original MUSHclient Lua API)
 * =======================================================
 * - Volume: -100 (silent) to 0 (full volume)
 * - Pan: -100 (full left) to +100 (full right), 0 = center
 *
 * PANNING IMPLEMENTATION
 * ======================
 * Stereo panning is implemented by positioning QSpatialSound sources along
 * a horizontal arc in front of the listener:
 *   - Listener is at origin (0, 0, 0) facing +Z
 *   - Sounds are positioned at (X, 0, 100) where X = pan (already -100 to +100)
 *
 * SUPPORTED AUDIO FORMATS
 * =======================
 * Qt Spatial Audio uses the same FFmpeg backend as Qt Multimedia,
 * supporting WAV, MP3, OGG, FLAC, AAC, OPUS, and more.
 *
 * Original MUSHclient (DirectSound):
 *   - WAV files ONLY
 *   - 16-bit, 22.05 KHz, PCM, Mono/Stereo
 *
 * Mushkin (Qt Spatial Audio + FFmpeg):
 *   - WAV (all bit depths, all sample rates)
 *   - MP3 (MPEG-1/2 Layer 3)
 *   - OGG Vorbis
 *   - FLAC (lossless)
 *   - AAC (M4A, MP4)
 *   - OPUS
 *   - And many more formats supported by FFmpeg
 */

#include "../ui/views/output_view.h"
#include "lua_api/lua_common.h" // For error codes: eOK, eCannotPlaySound
#include "world_document.h"
#include <QAudioEngine>
#include <QAudioListener>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSpatialSound>
#include <QUrl>
#include <QVector3D>
#include <cmath> // for pow()

/**
 * InitializeSoundSystem - Create spatial audio engine and sound buffers
 *
 * Creates a single QAudioEngine with QAudioListener at origin,
 * and 10 QSpatialSound instances for simultaneous sound playback with panning.
 *
 * On headless systems (CI, servers) without audio hardware, this will
 * gracefully fail and leave sound disabled (m_audioEngine = nullptr).
 */
void WorldDocument::InitializeSoundSystem()
{
    qDebug() << "Initializing spatial audio system (" << MAX_SOUND_BUFFERS << "buffers)";

    // Initialize all buffer pointers to nullptr first (defensive)
    for (int i = 0; i < MAX_SOUND_BUFFERS; i++) {
        m_soundBuffers[i].spatialSound = nullptr;
        m_soundBuffers[i].isPlaying = false;
        m_soundBuffers[i].isLooping = false;
        m_soundBuffers[i].filename.clear();
    }

    // Try to create the audio engine - may fail on headless systems
    try {
        m_audioEngine = new QAudioEngine(this);
        if (!m_audioEngine) {
            qWarning() << "Failed to create QAudioEngine - sound disabled";
            return;
        }

        m_audioEngine->setOutputMode(QAudioEngine::Stereo);

        // Create listener at origin facing +Z
        m_audioListener = new QAudioListener(m_audioEngine);
        if (!m_audioListener) {
            qWarning() << "Failed to create QAudioListener - sound disabled";
            delete m_audioEngine;
            m_audioEngine = nullptr;
            return;
        }
        m_audioListener->setPosition(QVector3D(0, 0, 0));
        // Default rotation faces +Z which is correct for our coordinate system

        // Start the engine
        m_audioEngine->start();

        // Create sound buffers (spatial sounds)
        for (int i = 0; i < MAX_SOUND_BUFFERS; i++) {
            m_soundBuffers[i].spatialSound = new QSpatialSound(m_audioEngine);

            // Configure for direct sound (no room effects, minimal attenuation)
            m_soundBuffers[i].spatialSound->setDistanceModel(QSpatialSound::DistanceModel::Linear);
            m_soundBuffers[i].spatialSound->setSize(0.0f);              // Point source
            m_soundBuffers[i].spatialSound->setDistanceCutoff(1000.0f); // Very far cutoff
            m_soundBuffers[i].spatialSound->setManualAttenuation(1.0f); // No distance attenuation

            // Default position (centered, in front of listener)
            m_soundBuffers[i].spatialSound->setPosition(QVector3D(0, 0, 100));

            // Note: QSpatialSound doesn't have a playback completion signal.
            // For non-looping sounds (loops=1), the sound plays once and stops.
            // isPlaying is cleared when stop() is called or buffer is reused.
        }

        qDebug() << "Spatial audio system initialized";
    } catch (const std::exception& e) {
        qWarning() << "Exception during sound system initialization:" << e.what()
                   << "- sound disabled";
        CleanupSoundSystem();
    } catch (...) {
        qWarning() << "Unknown exception during sound system initialization - sound disabled";
        CleanupSoundSystem();
    }
}

/**
 * CleanupSoundSystem - Stop and delete all sound buffers and engine
 */
void WorldDocument::CleanupSoundSystem()
{
    qDebug() << "Cleaning up spatial audio system";

    // Stop and delete all spatial sounds
    for (int i = 0; i < MAX_SOUND_BUFFERS; i++) {
        if (m_soundBuffers[i].spatialSound) {
            m_soundBuffers[i].spatialSound->stop();
            delete m_soundBuffers[i].spatialSound;
            m_soundBuffers[i].spatialSound = nullptr;
        }
        m_soundBuffers[i].isPlaying = false;
        m_soundBuffers[i].isLooping = false;
        m_soundBuffers[i].filename.clear();
    }

    // Stop and delete engine (listener is owned by engine)
    if (m_audioEngine) {
        m_audioEngine->stop();
        delete m_audioEngine;
        m_audioEngine = nullptr;
    }
    m_audioListener = nullptr; // Deleted with engine

    qDebug() << "Spatial audio system cleaned up";
}

/**
 * IsSoundBufferAvailable - Check if buffer is free (internal, 0-based)
 *
 * @param buffer Buffer index (0-9, internal 0-based)
 * @return true if buffer is available, false otherwise
 */
bool WorldDocument::IsSoundBufferAvailable(qint16 buffer)
{
    if (buffer < 0 || buffer >= MAX_SOUND_BUFFERS)
        return false;

    return !m_soundBuffers[buffer].isPlaying;
}

/**
 * ReleaseInactiveSoundBuffers - Update buffer states
 *
 * Note: QSpatialSound doesn't provide a way to check if playback has finished.
 * For non-looping sounds, the buffer remains marked as "in use" until:
 * - A new sound is played in the same buffer
 * - StopSound() is called explicitly
 *
 * This is a minor limitation but acceptable for most use cases.
 */
void WorldDocument::ReleaseInactiveSoundBuffers()
{
    // QSpatialSound has no mediaStatus() or playbackState() method.
    // Buffers are released when stop() is called or buffer is reused.
    // This function is kept for API compatibility but is effectively a no-op.
}

/**
 * PlaySound - Play sound with volume and panning
 *
 * Buffer indexing matches original MUSHclient:
 * - Buffer 0: Auto-select first available buffer (searches first half, falls back to buffer 1)
 * - Buffers 1-10: Specific buffer (converted to 0-9 internally)
 *
 * @param buffer Buffer number (0 = auto, 1-10 = specific buffer)
 * @param filename Path to sound file
 * @param loop Loop playback?
 * @param volume Volume (-100 = silent, 0 = full volume)
 * @param pan Panning (-100 = full left, 0 = center, +100 = full right)
 * @return eOK on success, eCannotPlaySound on error
 */
qint32 WorldDocument::PlaySound(qint16 buffer, const QString& filename, bool loop, double volume,
                                double pan)
{
    // Lazy-load audio system on first use
    // This avoids creating audio objects in tests and headless environments
    if (!m_audioEngine) {
        InitializeSoundSystem();
        // If still no audio engine, sound is disabled
        if (!m_audioEngine) {
            qWarning() << "PlaySound: Audio system not available";
            return eCannotPlaySound;
        }
    }

    // Release any inactive buffers first (free up stopped sounds)
    ReleaseInactiveSoundBuffers();

    // Clamp volume to valid range (-100 to 0)
    if (volume > 0 || volume < -100.0)
        volume = 0.0;

    // Clamp pan to valid range (-100 to +100)
    if (pan > 100.0 || pan < -100.0)
        pan = 0.0;

    // Buffer 0 = auto-select free buffer (matches original MUSHclient behavior)
    if (buffer == 0) {
        // Search first half of buffers for a free one (like original)
        for (int i = 0; i < (MAX_SOUND_BUFFERS / 2); i++) {
            if (!m_soundBuffers[i].isPlaying) {
                buffer = i + 1; // Convert to 1-based
                break;
            }
        }
        // No free buffer found? Use buffer 1
        if (buffer == 0)
            buffer = 1;
    }

    // Convert 1-based to 0-based
    buffer--;

    // Validate buffer range
    if (buffer < 0 || buffer >= MAX_SOUND_BUFFERS) {
        qWarning() << "PlaySound: Invalid buffer" << (buffer + 1);
        return eBadParameter;
    }

    SoundBuffer& sb = m_soundBuffers[buffer];

    if (!sb.spatialSound) {
        qWarning() << "PlaySound: Buffer not initialized" << (buffer + 1);
        return eCannotPlaySound;
    }

    // Stop current sound in buffer
    sb.spatialSound->stop();

    // Resolve file path
    QString fullPath = ResolveFilePath(filename);

    if (!QFile::exists(fullPath)) {
        qWarning() << "PlaySound: File not found:" << fullPath;
        return eFileNotFound;
    }

    // Set source
    sb.spatialSound->setSource(QUrl::fromLocalFile(fullPath));
    sb.filename = fullPath;

    // Set looping
    sb.isLooping = loop;
    sb.spatialSound->setLoops(loop ? QSpatialSound::Infinite : 1);

    // Convert volume: percentage (-100 to 0) -> linear (0.0 to 1.0)
    // Original MUSHclient: -100 means -100 dB (silent), 0 means full volume
    // Each -3 dB halves the volume, so we use pow(10, volume/20) for proper dB conversion
    float linearVolume;
    if (volume <= -100.0) {
        linearVolume = 0.0f;
    } else if (volume >= 0) {
        linearVolume = 1.0f;
    } else {
        // volume is -100 to 0, treat as dB: 10^(dB/20)
        linearVolume = std::pow(10.0f, static_cast<float>(volume) / 20.0f);
    }
    sb.spatialSound->setVolume(linearVolume);

    // Convert pan to spatial position
    // pan: -100 (full left) to +100 (full right)
    // Position: X = pan, Y = 0, Z = 100 (fixed distance in front of listener)
    float panPosition = static_cast<float>(pan);
    sb.spatialSound->setPosition(QVector3D(panPosition, 0, 100));

    // Play
    sb.spatialSound->play();
    sb.isPlaying = true;

    qDebug() << "Playing sound:" << filename << "buffer:" << (buffer + 1) << "loop:" << loop
             << "volume:" << linearVolume << "pan:" << pan;

    return eOK;
}

/**
 * StopSound - Stop sound in buffer
 *
 * Buffer indexing matches original MUSHclient:
 * - Buffer 0: Stop ALL sounds and release buffers
 * - Buffers 1-10: Stop specific buffer (converted to 0-9 internally)
 *
 * @param buffer Buffer number (0 = stop all, 1-10 = specific buffer)
 * @return eOK on success, eBadParameter on invalid buffer
 */
qint32 WorldDocument::StopSound(qint16 buffer)
{
    if (buffer == 0) {
        // Stop all sounds
        qDebug() << "Stopping all sounds";
        for (int i = 0; i < MAX_SOUND_BUFFERS; i++) {
            if (m_soundBuffers[i].spatialSound) {
                m_soundBuffers[i].spatialSound->stop();
                m_soundBuffers[i].isPlaying = false;
                m_soundBuffers[i].filename.clear();
            }
        }
        return eOK;
    }

    // Convert 1-based to 0-based
    buffer--;

    if (buffer < 0 || buffer >= MAX_SOUND_BUFFERS) {
        qWarning() << "StopSound: Invalid buffer" << (buffer + 1);
        return eBadParameter;
    }

    qDebug() << "Stopping sound in buffer" << (buffer + 1);
    if (m_soundBuffers[buffer].spatialSound) {
        m_soundBuffers[buffer].spatialSound->stop();
    }
    m_soundBuffers[buffer].isPlaying = false;
    m_soundBuffers[buffer].filename.clear();

    return eOK;
}

/**
 * PlaySoundFile - Play sound in first available buffer
 *
 * Helper for triggers and simple playback.
 * Uses buffer 0 (auto-select) to let PlaySound find an available buffer.
 *
 * @param filename Path to sound file
 * @return true on success, false on error
 */
bool WorldDocument::PlaySoundFile(const QString& filename)
{
    // Use buffer 0 for auto-select, with default settings (no loop, full volume, center pan)
    return PlaySound(0, filename, false, 0.0, 0.0) == eOK;
}

/**
 * ResolveFilePath - Resolve relative/absolute sound file paths
 *
 * @param filename File path to resolve
 * @return Resolved absolute path
 */
QString WorldDocument::ResolveFilePath(const QString& filename)
{
    // If absolute path, use as-is
    if (QFileInfo(filename).isAbsolute())
        return filename;

    // Try relative to current working directory
    QString fullPath = QDir::current().filePath(filename);

    if (QFile::exists(fullPath))
        return fullPath;

    // Try relative to sounds subdirectory
    QString soundsDir = QDir::current().filePath("sounds");
    fullPath = QDir(soundsDir).filePath(filename);

    if (QFile::exists(fullPath))
        return fullPath;

    // Return original (will fail later with file not found)
    return filename;
}

/**
 * GetSoundStatus - Query the status of a sound buffer
 *
 * Returns the current status of a specific sound buffer.
 *
 * @param buffer Buffer number (1-10, 1-based to match original MUSHclient)
 * @return Status code:
 *         -2: Buffer is free (no sound loaded)
 *         -1: Buffer out of range
 *          0: Sound is not playing
 *          1: Sound is playing but not looping
 *          2: Sound is playing and looping
 */
qint32 WorldDocument::GetSoundStatus(qint16 buffer)
{
    // Make buffer zero-relative (original MUSHclient uses 1-based indexing)
    buffer--;

    // Buffer must be in range
    if (buffer < 0 || buffer >= MAX_SOUND_BUFFERS) {
        return -1;
    }

    const SoundBuffer& sb = m_soundBuffers[buffer];

    // If no spatial sound or no sound loaded, buffer is free
    if (!sb.spatialSound || sb.filename.isEmpty()) {
        return -2;
    }

    // Check if playing
    if (sb.isPlaying) {
        return sb.isLooping ? 2 : 1;
    }

    // Not playing
    return 0;
}

/**
 * IsWindowActive - Check if main window has focus
 *
 * Used for bSoundIfInactive trigger flag
 *
 * @return true if window is active, false otherwise
 */
bool WorldDocument::IsWindowActive()
{
    if (!m_pActiveOutputView)
        return false;

    QWidget* window = m_pActiveOutputView->window();
    if (!window)
        return false;

    return window->isActiveWindow();
}
