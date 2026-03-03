// i_remote_transport.h
// Abstract transport interface for Remote Access Server
//
// Decouples RemoteClient from the concrete transport (Telnet, SSH, etc.).
// Part of Remote Access Server feature.

#ifndef I_REMOTE_TRANSPORT_H
#define I_REMOTE_TRANSPORT_H

#include <QByteArray>
#include <QObject>
#include <QString>

/**
 * Abstract transport layer for a single remote client connection.
 *
 * Responsibilities:
 *  - Owns (or wraps) the underlying I/O channel (socket, SSH channel, etc.)
 *  - Strips protocol framing from incoming data and buffers clean bytes
 *  - Escapes outgoing data for the protocol
 *  - Exposes terminal geometry reported by the client
 *
 * Concrete implementations: TelnetServerSession, SshServerSession.
 */
class IRemoteTransport : public QObject {
    Q_OBJECT

  public:
    explicit IRemoteTransport(QObject* parent = nullptr) : QObject(parent)
    {
    }
    ~IRemoteTransport() override = default;

    /**
     * Drain and return all buffered clean data received since the last call.
     * The transport strips protocol framing (IAC sequences, SSH channel
     * overhead, etc.) before buffering.  Returns an empty array when no
     * data is available.
     */
    virtual QByteArray processIncoming() = 0;

    /**
     * Escape raw application data so it is safe to send over the transport
     * (e.g. double IAC 0xFF bytes for Telnet; no-op for SSH).
     */
    virtual QByteArray escapeOutgoing(const QByteArray& data) = 0;

    /**
     * Write bytes directly to the underlying channel without further
     * escaping.  The caller is responsible for having called escapeOutgoing()
     * first when required.
     */
    virtual void writeRaw(const QByteArray& data) = 0;

    /** Initiate a graceful shutdown of the transport. Emits closed(). */
    virtual void close() = 0;

    /** Return "address:port" (or equivalent identifier) for the remote end. */
    virtual QString peerAddress() const = 0;

    /** Terminal width reported by the client, or a sensible default. */
    virtual int terminalWidth() const = 0;

    /** Terminal height reported by the client, or a sensible default. */
    virtual int terminalHeight() const = 0;

  signals:
    /**
     * Emitted once the transport is fully established and ready for
     * application-level data (e.g. after Telnet option negotiation).
     */
    void ready();

    /** Emitted when the transport has been closed (by either end). */
    void closed();

    /**
     * Emitted when clean application data is available.  The receiver
     * should call processIncoming() to retrieve it.
     */
    void dataAvailable();

    /** Emitted when the client reports a terminal resize. */
    void terminalSizeChanged(int width, int height);
};

#endif // I_REMOTE_TRANSPORT_H
