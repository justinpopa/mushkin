#pragma once

/**
 * sound_manager.h - SoundManager companion object
 *
 * Owns all spatial audio state for a WorldDocument:
 * - QAudioEngine / QAudioListener (lazy-created on first PlaySound call)
 * - 10 QSpatialSound buffers
 *
 * OWNERSHIP MODEL
 * ===============
 * QAudioEngine owns QAudioListener and each QSpatialSound via Qt parent-child.
 * SoundManager stores raw pointers but does NOT own those objects; the engine
 * destructor handles their cleanup.  SoundManager DOES own the engine itself
 * (allocated with `new QAudioEngine` and deleted in cleanup()).
 *
 * BUFFER INDEXING (matches original MUSHclient)
 * =============================================
 * External API is 1-based (buffers 1–10).  Buffer 0 is special:
 *   - playSound(0, …) → auto-select first free buffer
 *   - stopSound(0)    → stop all buffers
 * Internally converted to 0-based (0–9) before use.
 */

#include <QObject>
#include <QString>
#include <array>
#include <cstdint>

class QAudioEngine;
class QAudioListener;
class QSpatialSound;
class WorldDocument;

inline constexpr int MAX_SOUND_BUFFERS = 10;

struct SoundBuffer {
    QSpatialSound* spatialSound = nullptr;
    bool isPlaying = false;
    bool isLooping = false;
    QString filename;
};

class SoundManager {
  public:
    explicit SoundManager(WorldDocument& doc);
    ~SoundManager();

    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    // Core playback
    qint32 playSound(qint16 buffer, const QString& filename, bool loop, double volume, double pan);
    qint32 stopSound(qint16 buffer);
    bool playSoundFile(const QString& filename);
    qint32 getSoundStatus(qint16 buffer);

    // MSP support (downloads and plays sounds for MUD Sound Protocol)
    void playMSPSound(const QString& filename, const QString& url, bool loop, double volume,
                      qint16 buffer);

    // Buffer management
    bool isSoundBufferAvailable(qint16 buffer);
    void releaseInactiveSoundBuffers();

    // Utility
    QString resolveFilePath(const QString& filename);
    bool isWindowActive();

    // Lifecycle (called by WorldDocument constructor / destructor)
    void initialize();
    void cleanup();

  private:
    WorldDocument& m_doc;

    // Qt Spatial Audio objects — owned by the engine via Qt parent-child
    // EXCEPT m_audioEngine itself, which is allocated/deleted in initialize()/cleanup().
    QAudioEngine* m_audioEngine = nullptr;
    QAudioListener* m_audioListener = nullptr; // Non-owning; owned by m_audioEngine
    std::array<SoundBuffer, MAX_SOUND_BUFFERS> m_soundBuffers{};
};
