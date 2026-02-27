#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <cstdint>

class WorldDocument;
class WorldSocket;

// Connection phase constants
// Keep the legacy anonymous-enum names available as inline constexpr so that
// existing code compiled against world_document.h still compiles unchanged.
inline constexpr qint32 CONNECT_NOT_CONNECTED = 0;
inline constexpr qint32 CONNECT_MUD_NAME_LOOKUP = 1;
inline constexpr qint32 CONNECT_CONNECTING_TO_MUD = 3;
inline constexpr qint32 CONNECT_CONNECTED_TO_MUD = 8;
inline constexpr qint32 CONNECT_DISCONNECTING = 9;

/**
 * ConnectionManager — companion object for WorldDocument
 *
 * Owns connection state, network statistics, timing, and the command queue.
 * Extracted from WorldDocument following the TelnetParser / MXPEngine pattern.
 *
 * WorldDocument holds a std::unique_ptr<ConnectionManager> m_connectionManager.
 * All moved fields are public (direct-access companion-object pattern).
 * The WorldSocket pointer is non-owning; Qt parent-child keeps it alive.
 */
class ConnectionManager {
  public:
    explicit ConnectionManager(WorldDocument& doc);
    ~ConnectionManager();

    ConnectionManager(const ConnectionManager&) = delete;
    ConnectionManager& operator=(const ConnectionManager&) = delete;

    // ========== Connection lifecycle ==========
    void connectToMud();
    void disconnectFromMud();
    void onConnect(int errorCode);
    void onConnectionDisconnect();

    // ========== State queries ==========
    [[nodiscard]] bool isConnected() const
    {
        return m_iConnectPhase == CONNECT_CONNECTED_TO_MUD;
    }
    [[nodiscard]] qint64 connectedTime() const;
    void resetConnectedTime();

    // ========== Command queue ==========
    [[nodiscard]] QStringList getCommandQueue() const;
    qint32 queue(const QString& message, bool echo);
    qint32 discardQueue();

    // ========== Public state (companion-object pattern: callers access directly) ==========

    // Socket — non-owning; created in WorldDocument ctor with WorldDocument as QObject parent.
    WorldSocket* m_pSocket = nullptr;

    // Connection phase
    qint32 m_iConnectPhase = CONNECT_NOT_CONNECTED;

    // Network statistics
    qint64 m_nBytesIn = 0;
    qint64 m_nBytesOut = 0;
    qint64 m_iInputPacketCount = 0;
    qint64 m_iOutputPacketCount = 0;
    qint32 m_nTotalLinesSent = 0;
    qint32 m_nTotalLinesReceived = 0;

    // Timing
    QDateTime m_tConnectTime;
    qint64 m_tsConnectDuration = 0;
    QDateTime m_whenWorldStarted;
    qint64 m_whenWorldStartedHighPrecision = 0;

    // Command queue (speedwalk / delayed send)
    QStringList m_CommandQueue;

  private:
    WorldDocument& m_doc;

    // Lua callbacks — execute OnConnect / OnDisconnect script functions
    void onWorldConnect();
    void onWorldDisconnect();
};
