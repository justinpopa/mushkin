#ifndef WORLD_SOCKET_H
#define WORLD_SOCKET_H

#include <QAbstractSocket>
#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <expected>
#include <span>

class WorldDocument;

/**
 * WorldSocket - Qt wrapper for MUD network communication
 *
 * Wraps QTcpSocket with signal/slot based async events. Delegates
 * connection handling to WorldDocument.
 */
class WorldSocket : public QObject {
    Q_OBJECT

  public:
    explicit WorldSocket(WorldDocument* doc, QObject* parent = nullptr);
    ~WorldSocket() override;

    void connectToHost(const QString& host, quint16 port);
    void disconnectFromHost();
    std::expected<qint64, QString> send(std::span<const char> data);
    std::expected<qint64, QString> receive(std::span<char> buffer);
    bool isConnected() const;
    [[nodiscard]] QString peerAddress() const;

    WorldDocument* m_pDoc;
    QString m_outstanding_data;

  private slots:
    void onReadyRead();
    void onBytesWritten(qint64 bytes);
    void onDisconnected();
    void onConnected();
    void onError(QAbstractSocket::SocketError socketError);

  private:
    QTcpSocket* m_socket;
};

#endif // WORLD_SOCKET_H
