// remote_client.cpp
// Per-connection handler for Remote Access Server
//
// Part of Remote Access Server feature

#include "remote_client.h"
#include "../text/line.h"
#include "../world/world_document.h"
#include "ansi_formatter.h"
#include "i_remote_transport.h"
#include <QObject>

RemoteClient::RemoteClient(IRemoteTransport* transport, WorldDocument* doc, int scrollbackLines,
                           QObject* parent)
    : QObject(parent), m_transport(transport), m_pDoc(doc),
      m_formatter(std::make_unique<AnsiFormatter>(doc)), m_scrollbackLines(scrollbackLines),
      m_state(State::Connecting), m_connectedAt(QDateTime::currentDateTime())
{
    // Connect transport signals.
    connect(m_transport, &IRemoteTransport::ready, this, &RemoteClient::onTransportReady);
    connect(m_transport, &IRemoteTransport::dataAvailable, this, &RemoteClient::onReadyRead);
    connect(m_transport, &IRemoteTransport::closed, this, &RemoteClient::onDisconnected);
}

RemoteClient::~RemoteClient() = default;

bool RemoteClient::isAuthenticated() const
{
    return m_state == State::Authenticated;
}

QString RemoteClient::address() const
{
    return m_transport ? m_transport->peerAddress() : QString();
}

QDateTime RemoteClient::connectedAt() const
{
    return m_connectedAt;
}

void RemoteClient::sendLine(const Line* line)
{
    if (m_state != State::Authenticated || !line) {
        return;
    }

    QByteArray formatted = m_formatter->formatLine(line, true);
    sendBytes(formatted);
}

void RemoteClient::sendIncompleteLine(const Line* line)
{
    if (m_state != State::Authenticated || !line) {
        return;
    }

    QByteArray formatted = m_formatter->formatIncompleteLine(line);
    sendBytes(formatted);
}

void RemoteClient::sendRawText(const QString& text, bool includeNewline)
{
    QByteArray data = AnsiFormatter::formatRaw(text, includeNewline);
    sendBytes(data);
}

void RemoteClient::disconnect()
{
    m_state = State::Disconnecting;
    if (m_transport) {
        m_transport->close();
    }
}

void RemoteClient::sendBytes(const QByteArray& data)
{
    if (m_transport) {
        m_transport->writeRaw(m_transport->escapeOutgoing(data));
    }
}

void RemoteClient::onReadyRead()
{
    QByteArray clean = m_transport->processIncoming();

    if (!clean.isEmpty()) {
        processInput(clean);
    }
}

void RemoteClient::onDisconnected()
{
    m_state = State::Disconnecting;
    emit disconnected();
}

void RemoteClient::onTransportReady()
{
    // SSH transport has completed key exchange and authentication.
    // The connection is already authenticated — go straight to active state.
    m_state = State::Authenticated;
    m_formatter->reset(); // Reset ANSI state for clean output.
    sendWelcome();
    sendScrollback();
    emit authenticated();
}

void RemoteClient::processInput(const QByteArray& data)
{
    m_inputBuffer.append(QString::fromUtf8(data));

    // Security: cap input buffer to prevent OOM from clients sending
    // continuous data without newlines.
    constexpr int kMaxInputBuffer = 4096;
    if (m_inputBuffer.size() > kMaxInputBuffer) {
        sendRawText("\r\nInput too long. Disconnecting.\r\n");
        disconnect();
        return;
    }

    // Process complete lines (SSH clients typically send \r\n or just \n).
    int nlPos;
    while ((nlPos = m_inputBuffer.indexOf('\n')) != -1) {
        QString line = m_inputBuffer.left(nlPos);
        m_inputBuffer.remove(0, nlPos + 1);

        // Strip trailing \r if present (handles \r\n).
        if (line.endsWith('\r')) {
            line.chop(1);
        }

        switch (m_state) {
            case State::Authenticated:
                if (!line.isEmpty()) {
                    emit commandReceived(line);
                }
                break;

            default:
                // Ignore input in other states.
                break;
        }
    }
}

void RemoteClient::sendWelcome()
{
    QString worldName = m_pDoc ? m_pDoc->m_mush_name : "Unknown";
    QString banner = QString("\r\n"
                             "=== MUSHclient Qt Remote Access ===\r\n"
                             "World: %1\r\n"
                             "Streaming output...\r\n"
                             "\r\n")
                         .arg(worldName);
    sendBytes(banner.toUtf8());
}

void RemoteClient::sendScrollback()
{
    if (!m_pDoc || m_scrollbackLines <= 0) {
        return;
    }

    int totalLines = static_cast<int>(m_pDoc->m_lineList.size());
    int startLine = qMax(0, totalLines - m_scrollbackLines);

    if (startLine < totalLines) {
        sendRawText(QString("--- Last %1 lines ---\r\n").arg(totalLines - startLine));
    }

    for (int i = startLine; i < totalLines; ++i) {
        Line* line = m_pDoc->m_lineList.at(i).get();
        if (line) {
            sendLine(line);
        }
    }

    if (startLine < totalLines) {
        sendRawText("--- End scrollback ---\r\n\r\n");
    }
}
