// remote_client.cpp
// Per-connection handler for Remote Access Server
//
// Part of Remote Access Server feature

#include "remote_client.h"
#include "../text/line.h"
#include "../world/world_document.h"
#include "ansi_formatter.h"
#include "telnet_server_session.h"
#include <QTcpSocket>

namespace {
constexpr int kMaxPasswordAttempts = 3;
}

RemoteClient::RemoteClient(QTcpSocket* socket, WorldDocument* doc, const QString& password,
                           int scrollbackLines, QObject* parent)
    : QObject(parent), m_pSocket(socket), m_pDoc(doc),
      m_telnet(std::make_unique<TelnetServerSession>(socket, this)),
      m_formatter(std::make_unique<AnsiFormatter>(doc)), m_password(password),
      m_scrollbackLines(scrollbackLines), m_state(State::Negotiating), m_failedAttempts(0),
      m_maxFailedAttempts(kMaxPasswordAttempts), m_connectedAt(QDateTime::currentDateTime())
{
    // Take ownership of socket
    m_pSocket->setParent(this);

    // Connect socket signals
    connect(m_pSocket, &QTcpSocket::readyRead, this, &RemoteClient::onReadyRead);
    connect(m_pSocket, &QTcpSocket::disconnected, this, &RemoteClient::onDisconnected);
    connect(m_pSocket, &QTcpSocket::errorOccurred, this, &RemoteClient::onError);

    // Connect telnet session signals
    connect(m_telnet.get(), &TelnetServerSession::negotiationComplete, this,
            &RemoteClient::onNegotiationComplete);

    // Start telnet negotiation
    m_telnet->initiateNegotiation();
}

RemoteClient::~RemoteClient()
{
    if (m_pSocket && m_pSocket->isOpen()) {
        m_pSocket->close();
    }
}

bool RemoteClient::isAuthenticated() const
{
    return m_state == State::Authenticated;
}

QString RemoteClient::address() const
{
    if (m_pSocket) {
        return QString("%1:%2").arg(m_pSocket->peerAddress().toString()).arg(m_pSocket->peerPort());
    }
    return QString();
}

QDateTime RemoteClient::connectedAt() const
{
    return m_connectedAt;
}

void RemoteClient::sendLine(Line* line)
{
    if (m_state != State::Authenticated || !line) {
        return;
    }

    QByteArray formatted = m_formatter->formatLine(line, true);
    sendBytes(formatted);
}

void RemoteClient::sendIncompleteLine(Line* line)
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
    if (m_pSocket) {
        m_pSocket->close();
    }
}

void RemoteClient::sendBytes(const QByteArray& data)
{
    if (m_pSocket && m_pSocket->isOpen()) {
        QByteArray escaped = TelnetServerSession::escapeOutgoing(data);
        m_pSocket->write(escaped);
    }
}

void RemoteClient::onReadyRead()
{
    QByteArray raw = m_pSocket->readAll();
    QByteArray clean = m_telnet->processIncoming(raw);

    if (!clean.isEmpty()) {
        processInput(clean);
    }
}

void RemoteClient::onDisconnected()
{
    m_state = State::Disconnecting;
    emit disconnected();
}

void RemoteClient::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    if (m_pSocket) {
        emit error(m_pSocket->errorString());
    }
}

void RemoteClient::onNegotiationComplete()
{
    // Telnet negotiation done, send welcome and password prompt
    sendWelcome();
    sendPasswordPrompt();
    m_state = State::AwaitingPassword;
}

void RemoteClient::processInput(const QByteArray& data)
{
    m_inputBuffer.append(QString::fromUtf8(data));

    // Process complete lines (telnet sends \r\n, some clients send just \n)
    int nlPos;
    while ((nlPos = m_inputBuffer.indexOf('\n')) != -1) {
        QString line = m_inputBuffer.left(nlPos);
        m_inputBuffer.remove(0, nlPos + 1);

        // Strip trailing \r if present (handles \r\n)
        if (line.endsWith('\r')) {
            line.chop(1);
        }

        // Handle based on state
        switch (m_state) {
            case State::AwaitingPassword:
                if (checkPassword(line)) {
                    handleAuthSuccess();
                } else {
                    handleAuthFailure();
                }
                break;

            case State::Authenticated:
                if (!line.isEmpty()) {
                    emit commandReceived(line);
                }
                break;

            default:
                // Ignore input in other states
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
                             "\r\n")
                         .arg(worldName);
    sendBytes(banner.toUtf8());
}

void RemoteClient::sendPasswordPrompt()
{
    sendBytes(QByteArray("Password: "));
}

bool RemoteClient::checkPassword(const QString& attempt)
{
    return attempt == m_password;
}

void RemoteClient::handleAuthSuccess()
{
    m_state = State::Authenticated;
    m_formatter->reset(); // Reset ANSI state for clean output

    sendRawText("\r\nAuthenticated. Streaming output...\r\n");

    // Send scrollback
    sendScrollback();

    emit authenticated();
}

void RemoteClient::handleAuthFailure()
{
    m_failedAttempts++;

    if (m_failedAttempts >= m_maxFailedAttempts) {
        sendRawText("\r\nToo many failed attempts. Disconnecting.\r\n");
        disconnect();
    } else {
        int remaining = m_maxFailedAttempts - m_failedAttempts;
        sendRawText(QString("\r\nIncorrect password. %1 attempt(s) remaining.\r\n").arg(remaining));
        sendPasswordPrompt();
    }
}

void RemoteClient::sendScrollback()
{
    if (!m_pDoc || m_scrollbackLines <= 0) {
        return;
    }

    int totalLines = m_pDoc->m_lineList.count();
    int startLine = qMax(0, totalLines - m_scrollbackLines);

    if (startLine < totalLines) {
        sendRawText(QString("--- Last %1 lines ---\r\n").arg(totalLines - startLine));
    }

    for (int i = startLine; i < totalLines; ++i) {
        Line* line = m_pDoc->m_lineList.at(i);
        if (line) {
            sendLine(line);
        }
    }

    if (startLine < totalLines) {
        sendRawText("--- End scrollback ---\r\n\r\n");
    }
}
