/**
 * sound_manager.cpp - SoundManager implementation
 *
 * Implements sound playback using Qt Spatial Audio (QAudioEngine/QSpatialSound)
 * with full stereo panning support.
 *
 * BUFFER INDEXING (matches original MUSHclient)
 * =============================================
 * - Buffer 0: Special — auto-select free buffer (playSound) or stop all (stopSound)
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
 */

#include "sound_manager.h"
#include "lua_api/lua_common.h" // For error codes: eOK, eCannotPlaySound
#include "view_interfaces.h"
#include "world_context.h"
#include <QAudioEngine>
#include <QAudioListener>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSpatialSound>
#include <QUrl>
#include <QVector3D>
#include <algorithm> // for std::clamp
#include <cmath>     // for pow()
#include <memory>    // for std::weak_ptr

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

SoundManager::SoundManager(IWorldContext& ctx) : m_ctx(ctx)
{
    // m_soundBuffers is value-initialised by the aggregate initialiser in the header.
    // The audio engine is lazy-created on first playSound() call.
}

SoundManager::~SoundManager()
{
    cleanup();
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

/**
 * initialize - Create spatial audio engine and sound buffers
 *
 * Creates a single QAudioEngine with QAudioListener at origin,
 * and MAX_SOUND_BUFFERS QSpatialSound instances for simultaneous sound playback.
 *
 * On headless systems (CI, servers) without audio hardware this will
 * gracefully fail and leave sound disabled (m_audioEngine = nullptr).
 */
void SoundManager::initialize()
{
    qDebug() << "Initializing spatial audio system (" << MAX_SOUND_BUFFERS << "buffers)";

    // Defensive: reset all buffer state first
    for (int i = 0; i < MAX_SOUND_BUFFERS; i++) {
        m_soundBuffers[i].spatialSound = nullptr;
        m_soundBuffers[i].isPlaying = false;
        m_soundBuffers[i].isLooping = false;
        m_soundBuffers[i].filename.clear();
    }

    // Try to create the audio engine — may fail on headless systems
    try {
        // No Qt parent: unique_ptr is the sole owner.  Qt parent-child would
        // conflict with unique_ptr and cause a double-free on destruction.
        m_audioEngine = std::make_unique<QAudioEngine>();

        m_audioEngine->setOutputMode(QAudioEngine::Stereo);

        // Create listener at origin facing +Z (owned by engine via Qt parent-child)
        m_audioListener = new QAudioListener(m_audioEngine.get());
        m_audioListener->setPosition(QVector3D(0, 0, 0));
        // Default rotation faces +Z which is correct for our coordinate system

        // Start the engine
        m_audioEngine->start();

        // Create sound buffers (spatial sounds — owned by engine via parent-child)
        for (int i = 0; i < MAX_SOUND_BUFFERS; i++) {
            m_soundBuffers[i].spatialSound = new QSpatialSound(m_audioEngine.get());

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
        cleanup();
    } catch (...) {
        qWarning() << "Unknown exception during sound system initialization - sound disabled";
        cleanup();
    }
}

/**
 * cleanup - Stop and delete all sound buffers and the engine
 *
 * QAudioListener and QSpatialSound objects are children of the engine,
 * so deleting the engine destroys them.  We null the raw pointers afterward.
 */
void SoundManager::cleanup()
{
    qDebug() << "Cleaning up spatial audio system";

    // Stop all spatial sounds explicitly before engine teardown
    for (int i = 0; i < MAX_SOUND_BUFFERS; i++) {
        if (m_soundBuffers[i].spatialSound) {
            m_soundBuffers[i].spatialSound->stop();
            // Do NOT delete — owned by m_audioEngine via Qt parent-child
            m_soundBuffers[i].spatialSound = nullptr;
        }
        m_soundBuffers[i].isPlaying = false;
        m_soundBuffers[i].isLooping = false;
        m_soundBuffers[i].filename.clear();
    }

    // Signal UAF guard: any in-flight MSP lambda will see alive == false and bail out.
    *m_alive = false;

    // Stop and destroy engine via unique_ptr (also destroys listener and spatial sounds
    // via Qt parent-child).
    if (m_audioEngine) {
        m_audioEngine->stop();
        m_audioEngine.reset();
    }
    m_audioListener = nullptr; // Destroyed with engine above

    qDebug() << "Spatial audio system cleaned up";
}

// ---------------------------------------------------------------------------
// Buffer management
// ---------------------------------------------------------------------------

/**
 * isSoundBufferAvailable - Check if buffer is free (internal, 0-based index)
 *
 * @param buffer Buffer index (0-9, internal 0-based)
 * @return true if buffer is available, false otherwise
 */
bool SoundManager::isSoundBufferAvailable(qint16 buffer)
{
    if (buffer < 0 || buffer >= MAX_SOUND_BUFFERS)
        return false;

    return !m_soundBuffers[buffer].isPlaying;
}

/**
 * releaseInactiveSoundBuffers - Update buffer states
 *
 * Note: QSpatialSound doesn't provide a way to check if playback has finished.
 * For non-looping sounds, the buffer remains marked as "in use" until:
 * - A new sound is played in the same buffer
 * - stopSound() is called explicitly
 *
 * This is a minor limitation but acceptable for most use cases.
 */
void SoundManager::releaseInactiveSoundBuffers()
{
    // QSpatialSound has no mediaStatus() or playbackState() method.
    // Buffers are released when stop() is called or buffer is reused.
    // This function is kept for API compatibility but is effectively a no-op.
}

// ---------------------------------------------------------------------------
// Core playback
// ---------------------------------------------------------------------------

/**
 * playSound - Play sound with volume and panning
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
qint32 SoundManager::playSound(qint16 buffer, const QString& filename, bool loop, double volume,
                               double pan)
{
    // Lazy-load audio system on first use
    // This avoids creating audio objects in tests and headless environments
    if (!m_audioEngine) {
        initialize();
        // If still no audio engine, sound is disabled
        if (!m_audioEngine) {
            qWarning() << "playSound: Audio system not available";
            return eCannotPlaySound;
        }
    }

    // Release any inactive buffers first (free up stopped sounds)
    releaseInactiveSoundBuffers();

    // Clamp volume to valid range (-100 to 0)
    volume = std::clamp(volume, -100.0, 0.0);

    // Clamp pan to valid range (-100 to +100)
    pan = std::clamp(pan, -100.0, 100.0);

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
        qWarning() << "playSound: Invalid buffer" << (buffer + 1);
        return eBadParameter;
    }

    SoundBuffer& sb = m_soundBuffers[buffer];

    if (!sb.spatialSound) {
        qWarning() << "playSound: Buffer not initialized" << (buffer + 1);
        return eCannotPlaySound;
    }

    // Stop current sound in buffer
    sb.spatialSound->stop();

    // Resolve file path
    QString fullPath = resolveFilePath(filename);

    if (!QFile::exists(fullPath)) {
        qWarning() << "playSound: File not found:" << fullPath;
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
 * stopSound - Stop sound in buffer
 *
 * Buffer indexing matches original MUSHclient:
 * - Buffer 0: Stop ALL sounds and release buffers
 * - Buffers 1-10: Stop specific buffer (converted to 0-9 internally)
 *
 * @param buffer Buffer number (0 = stop all, 1-10 = specific buffer)
 * @return eOK on success, eBadParameter on invalid buffer
 */
qint32 SoundManager::stopSound(qint16 buffer)
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
        qWarning() << "stopSound: Invalid buffer" << (buffer + 1);
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
 * playSoundFile - Play sound in first available buffer
 *
 * Helper for triggers and simple playback.
 * Uses buffer 0 (auto-select) to let playSound find an available buffer.
 *
 * @param filename Path to sound file
 * @return true on success, false on error
 */
bool SoundManager::playSoundFile(const QString& filename)
{
    // Use buffer 0 for auto-select, with default settings (no loop, full volume, center pan)
    return playSound(0, filename, false, 0.0, 0.0) == eOK;
}

/**
 * getSoundStatus - Query the status of a sound buffer
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
qint32 SoundManager::getSoundStatus(qint16 buffer)
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

// ---------------------------------------------------------------------------
// MSP support
// ---------------------------------------------------------------------------

/**
 * playMSPSound - Play a sound for MSP, downloading if necessary
 *
 * @param filename Sound filename
 * @param url Base URL for downloading (empty = no download)
 * @param loop Loop the sound?
 * @param volume Volume (-100 to 0)
 * @param buffer Buffer number (0 = auto, 1-10 = specific)
 */
void SoundManager::playMSPSound(const QString& filename, const QString& url, bool loop,
                                double volume, qint16 buffer)
{
    // First, try to resolve the file locally
    QString fullPath = resolveFilePath(filename);

    if (QFile::exists(fullPath)) {
        // File exists locally, play it
        playSound(buffer, fullPath, loop, volume, 0.0);
        return;
    }

    // Check if we have it cached
    QString cacheDir = QDir::current().filePath("sounds/cached");
    QString cachedPath = QDir(cacheDir).filePath(filename);

    if (QFile::exists(cachedPath)) {
        // Found in cache, play it
        playSound(buffer, cachedPath, loop, volume, 0.0);
        return;
    }

    // File not found locally — try to download if URL provided
    if (url.isEmpty()) {
        qWarning() << "MSP: Sound file not found and no URL provided:" << filename;
        return;
    }

    // Build the download URL
    QString downloadUrl = url;
    if (!downloadUrl.endsWith('/'))
        downloadUrl += '/';
    downloadUrl += filename;

    qDebug() << "MSP: Downloading sound from:" << downloadUrl;

    // Create cache directory if needed
    QDir().mkpath(cacheDir);

    // Create network manager for this download (will be deleted when done).
    // No Qt parent: manager->deleteLater() is called in the reply finished handler
    // (both success and early-exit paths), so lifetime is bounded by the reply.
    QNetworkAccessManager* manager = new QNetworkAccessManager(nullptr);
    QUrl downloadQUrl(downloadUrl);
    QNetworkRequest netRequest(downloadQUrl);
    netRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                            QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = manager->get(netRequest);

    // Store parameters for when download completes via QObject properties
    reply->setProperty("msp_filename", filename);
    reply->setProperty("msp_cached_path", cachedPath);
    reply->setProperty("msp_buffer", buffer);
    reply->setProperty("msp_loop", loop);
    reply->setProperty("msp_volume", volume);

    // UAF guard: capture a weak_ptr so the lambda can detect if SoundManager has been
    // destroyed (cleanup() sets *m_alive = false, invalidating all weak_ptrs).
    // SoundManager is not a QObject so QPointer is unavailable; weak_ptr<bool> is the
    // simplest portable alternative.
    std::weak_ptr<bool> weakAlive{m_alive};

    // Capture `this` alongside weakAlive.  The alive guard ensures `this` is only
    // dereferenced when SoundManager is still live — cleanup() sets *m_alive = false
    // before destroying any audio state, so a locked non-false alive guarantees safety.
    QObject::connect(reply, &QNetworkReply::finished, reply, [this, weakAlive, reply, manager]() {
        // Bail out if SoundManager has been cleaned up — avoids use-after-free.
        auto alive = weakAlive.lock();
        if (!alive || !*alive) {
            reply->deleteLater();
            manager->deleteLater();
            return;
        }

        // Retrieve stored parameters
        QString storedCachedPath = reply->property("msp_cached_path").toString();
        qint16 storedBuffer = static_cast<qint16>(reply->property("msp_buffer").toInt());
        bool storedLoop = reply->property("msp_loop").toBool();
        double storedVolume = reply->property("msp_volume").toDouble();

        if (reply->error() == QNetworkReply::NoError) {
            // Save to cache
            QFile cacheFile(storedCachedPath);
            if (cacheFile.open(QIODevice::WriteOnly)) {
                cacheFile.write(reply->readAll());
                cacheFile.close();
                qDebug() << "MSP: Downloaded and cached:" << storedCachedPath;

                // SoundManager alive guard confirmed above — safe to call.
                playSound(storedBuffer, storedCachedPath, storedLoop, storedVolume, 0.0);
            } else {
                qWarning() << "MSP: Failed to cache file:" << storedCachedPath;
            }
        } else {
            qWarning() << "MSP: Download failed:" << reply->errorString();
        }

        // Clean up
        reply->deleteLater();
        manager->deleteLater();
    });
}

// ---------------------------------------------------------------------------
// Utility
// ---------------------------------------------------------------------------

/**
 * resolveFilePath - Resolve relative/absolute sound file paths
 *
 * @param filename File path to resolve
 * @return Resolved absolute path (or original filename if not found)
 */
QString SoundManager::resolveFilePath(const QString& filename)
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
 * isWindowActive - Check if the main window has focus
 *
 * Used for the sound_if_inactive trigger flag.
 *
 * @return true if window is active, false otherwise
 */
bool SoundManager::isWindowActive()
{
    IOutputView* view = m_ctx.activeOutputView();
    if (!view)
        return false;

    QWidget* window = view->parentWindow();
    if (!window)
        return false;

    return window->isActiveWindow();
}
