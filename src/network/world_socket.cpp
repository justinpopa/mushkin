#include "world_socket.h"
#include "../world/world_document.h"
#include "logging.h"
#include <QDebug>

WorldSocket::WorldSocket(WorldDocument* doc, QObject* parent)
    : QObject(parent), m_pDoc(doc), m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::readyRead, this, &WorldSocket::onReadyRead);
    connect(m_socket, &QTcpSocket::bytesWritten, this, &WorldSocket::onBytesWritten);
    connect(m_socket, &QTcpSocket::disconnected, this, &WorldSocket::onDisconnected);
    connect(m_socket, &QTcpSocket::connected, this, &WorldSocket::onConnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &WorldSocket::onError);
}

WorldSocket::~WorldSocket() = default;

void WorldSocket::connectToHost(const QString& host, quint16 port)
{
    m_socket->connectToHost(host, port);
}

void WorldSocket::disconnectFromHost()
{
    m_socket->disconnectFromHost();
}

std::expected<qint64, QString> WorldSocket::send(std::span<const char> data)
{
    qint64 written = m_socket->write(data.data(), static_cast<qint64>(data.size()));
    if (written < 0) {
        qCDebug(lcNetwork) << "WorldSocket::send() error:" << m_socket->errorString();
        return std::unexpected(m_socket->errorString());
    }
    return written;
}

std::expected<qint64, QString> WorldSocket::receive(std::span<char> buffer)
{
    qint64 nRead = m_socket->read(buffer.data(), static_cast<qint64>(buffer.size()));
    if (nRead < 0) {
        return std::unexpected(m_socket->errorString());
    }
    return nRead;
}

bool WorldSocket::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void WorldSocket::onReadyRead()
{
    m_pDoc->ReceiveMsg();
}

void WorldSocket::onBytesWritten(qint64 bytes)
{
    Q_UNUSED(bytes);

    if (m_outstanding_data.isEmpty()) {
        return;
    }

    QByteArray data = m_outstanding_data.toUtf8();
    qint64 count = m_socket->write(data);

    if (count > 0) {
        m_outstanding_data = m_outstanding_data.mid(count);
    } else if (count < 0) {
        qCDebug(lcNetwork) << "WorldSocket::onBytesWritten() error:" << m_socket->errorString();
        m_outstanding_data.clear();
    }
}

void WorldSocket::onDisconnected()
{
    qCDebug(lcNetwork) << "WorldSocket::onDisconnected()";
    m_pDoc->OnConnectionDisconnect();
}

void WorldSocket::onConnected()
{
    qCDebug(lcNetwork) << "WorldSocket::onConnected()";
    m_pDoc->OnConnect(0);
}

void WorldSocket::onError(QAbstractSocket::SocketError socketError)
{
    qCDebug(lcNetwork) << "WorldSocket::onError():" << m_socket->errorString();
    m_pDoc->OnConnect(static_cast<int>(socketError));
}
