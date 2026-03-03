// remote_access_server.cpp
// Remote Access Server for MUSHclient Qt
//
// Allows remote SSH clients to connect and control the MUD client.

#include "remote_access_server.h"
#include "../text/line.h"
#include "../world/world_document.h"
#include "remote_client.h"
#include "ssh_host_key_manager.h"
#include "ssh_server_session.h"

#include <QHostAddress>
#include <QSocketNotifier>

#include <libssh/libssh.h>
#include <libssh/server.h>

RemoteAccessServer::RemoteAccessServer(WorldDocument* doc, QObject* parent)
    : QObject(parent), m_pDoc(doc), m_password(), m_authorizedKeysFile(), m_scrollbackLines(100),
      m_maxClients(5), m_lastLineIndex(-1)
{
}

RemoteAccessServer::~RemoteAccessServer()
{
    stop();
}

std::expected<void, StartError> RemoteAccessServer::start(quint16 port,
                                                          const QHostAddress& bindAddress)
{
    if (m_sshBind) {
        return {};
    }

    if (m_password.isEmpty() && m_authorizedKeysFile.isEmpty()) {
        emit error("Cannot start server: no password or authorized keys file set");
        return std::unexpected(StartError::NoPassword);
    }

    // Ensure a host key exists (generates one on first use).
    auto keyResult = SshHostKeyManager::ensureHostKey();
    if (!keyResult) {
        emit error("Cannot start server: failed to generate or load host key");
        return std::unexpected(StartError::HostKeyFailed);
    }
    const QString keyPath = *keyResult;

    m_sshBind = ssh_bind_new();
    if (!m_sshBind) {
        emit error("Cannot start server: ssh_bind_new() failed");
        return std::unexpected(StartError::BindFailed);
    }

    // Configure host key.
    {
        const QByteArray keyPathBytes = keyPath.toUtf8();
        const char* keyPathCStr = keyPathBytes.constData();
        ssh_bind_options_set(m_sshBind, SSH_BIND_OPTIONS_HOSTKEY, keyPathCStr);
    }

    // Configure bind address.
    {
        const QByteArray addrBytes = bindAddress.toString().toUtf8();
        const char* addrCStr = addrBytes.constData();
        ssh_bind_options_set(m_sshBind, SSH_BIND_OPTIONS_BINDADDR, addrCStr);
    }

    // Configure port.
    {
        unsigned int portVal = static_cast<unsigned int>(port);
        ssh_bind_options_set(m_sshBind, SSH_BIND_OPTIONS_BINDPORT, &portVal);
    }

    // Non-blocking so QSocketNotifier can drive accept.
    ssh_bind_set_blocking(m_sshBind, 0);

    if (ssh_bind_listen(m_sshBind) != SSH_OK) {
        emit error(QString("Failed to start SSH server on port %1").arg(port));
        ssh_bind_free(m_sshBind);
        m_sshBind = nullptr;
        return std::unexpected(StartError::BindFailed);
    }

    m_port = port;

    // Watch the bind fd for incoming connections.
    auto fd = static_cast<qintptr>(ssh_bind_get_fd(m_sshBind));
    m_bindNotifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(m_bindNotifier, &QSocketNotifier::activated, this,
            &RemoteAccessServer::onNewConnection);

    // Connect to WorldDocument signals for output streaming.
    if (m_pDoc) {
        connect(m_pDoc, &WorldDocument::linesAdded, this, &RemoteAccessServer::onLinesAdded);
        connect(m_pDoc, &WorldDocument::incompleteLine, this,
                &RemoteAccessServer::onIncompleteLine);

        // Initialize line tracking.
        m_lastLineIndex = static_cast<int>(m_pDoc->m_lineList.size()) - 1;
    }

    emit serverStarted(m_port);
    return {};
}

void RemoteAccessServer::stop()
{
    // Disconnect all clients.
    disconnectAllClients();

    // Stop listening.
    if (m_bindNotifier) {
        m_bindNotifier->setEnabled(false);
        delete m_bindNotifier;
        m_bindNotifier = nullptr;
    }
    if (m_sshBind) {
        ssh_bind_free(m_sshBind);
        m_sshBind = nullptr;
    }
    m_port = 0;

    // Disconnect from WorldDocument signals.
    if (m_pDoc) {
        disconnect(m_pDoc, &WorldDocument::linesAdded, this, &RemoteAccessServer::onLinesAdded);
        disconnect(m_pDoc, &WorldDocument::incompleteLine, this,
                   &RemoteAccessServer::onIncompleteLine);
    }

    emit serverStopped();
}

bool RemoteAccessServer::isRunning() const
{
    return m_sshBind != nullptr;
}

quint16 RemoteAccessServer::port() const
{
    return m_port;
}

int RemoteAccessServer::clientCount() const
{
    return m_clients.count();
}

int RemoteAccessServer::authenticatedClientCount() const
{
    int count = 0;
    for (RemoteClient* client : m_clients) {
        if (client->isAuthenticated()) {
            count++;
        }
    }
    return count;
}

void RemoteAccessServer::disconnectAllClients()
{
    // Copy list since disconnecting modifies it.
    QList<RemoteClient*> clients = m_clients;
    for (RemoteClient* client : clients) {
        client->disconnect();
    }
    m_clients.clear();
}

void RemoteAccessServer::broadcastMessage(const QString& message)
{
    for (RemoteClient* client : m_clients) {
        if (client->isAuthenticated()) {
            client->sendRawText(message);
        }
    }
}

void RemoteAccessServer::setPassword(const QString& password)
{
    m_password = password;
}

void RemoteAccessServer::setScrollbackLines(int lines)
{
    m_scrollbackLines = lines;
}

void RemoteAccessServer::setMaxClients(int max)
{
    m_maxClients = max;
}

void RemoteAccessServer::setAuthorizedKeysFile(const QString& path)
{
    m_authorizedKeysFile = path;
}

void RemoteAccessServer::onNewConnection()
{
    // ssh_bind_accept() may queue multiple sessions per activation.
    for (;;) {
        ssh_session session = ssh_new();
        if (!session) {
            break;
        }

        int rc = ssh_bind_accept(m_sshBind, session);
        if (rc != SSH_OK) {
            ssh_free(session);
            break;
        }

        // Check max clients before allocating any objects.
        if (m_maxClients > 0 && m_clients.count() >= m_maxClients) {
            // Reject by immediately freeing — client will see a connection reset.
            ssh_disconnect(session);
            ssh_free(session);
            continue;
        }

        // Create the SSH transport. RemoteClient will be its Qt parent,
        // so the session is destroyed when the client is destroyed.
        auto* transport = new SshServerSession(session, m_password, m_authorizedKeysFile);

        // Create client handler (no password — auth is handled by the transport).
        auto* client = new RemoteClient(transport, m_pDoc, m_scrollbackLines, this);

        // Parent the transport to the client so lifetimes are tied together.
        transport->setParent(client);

        // Connect client signals.
        connect(client, &RemoteClient::authenticated, this,
                &RemoteAccessServer::onClientAuthenticated);
        connect(client, &RemoteClient::commandReceived, this, &RemoteAccessServer::onClientCommand);
        connect(client, &RemoteClient::disconnected, this,
                &RemoteAccessServer::onClientDisconnected);

        m_clients.append(client);
        emit clientConnected(client->address());
    }
}

void RemoteAccessServer::onClientAuthenticated()
{
    RemoteClient* client = qobject_cast<RemoteClient*>(sender());
    if (client) {
        emit clientAuthenticated(client->address());
    }
}

void RemoteAccessServer::onClientCommand(const QString& command)
{
    // Route command through WorldDocument's Execute() for full processing
    // (aliases, speedwalk, command stacking, etc.)
    // Security: disable script prefix so remote clients cannot execute
    // arbitrary Lua code. Aliases (including plugin-defined ones) still work.
    if (m_pDoc) {
        m_pDoc->Execute(command, /*allowScriptPrefix=*/false);
    }
}

void RemoteAccessServer::onClientDisconnected()
{
    RemoteClient* client = qobject_cast<RemoteClient*>(sender());
    if (client) {
        QString addr = client->address();
        m_clients.removeOne(client);
        client->deleteLater();
        emit clientDisconnected(addr);
    }
}

void RemoteAccessServer::onLinesAdded()
{
    if (!m_pDoc) {
        return;
    }

    // Send any new lines since the last update.
    int totalLines = static_cast<int>(m_pDoc->m_lineList.size());
    int startIndex = m_lastLineIndex + 1;

    for (int i = startIndex; i < totalLines; ++i) {
        Line* line = m_pDoc->m_lineList.at(i).get();
        if (line) {
            broadcastLine(line);
        }
    }

    m_lastLineIndex = totalLines - 1;
}

void RemoteAccessServer::onIncompleteLine()
{
    if (!m_pDoc || !m_pDoc->m_currentLine) {
        return;
    }

    broadcastIncompleteLine(m_pDoc->m_currentLine.get());
}

void RemoteAccessServer::broadcastLine(const Line* line)
{
    for (RemoteClient* client : m_clients) {
        if (client->isAuthenticated()) {
            client->sendLine(line);
        }
    }
}

void RemoteAccessServer::broadcastIncompleteLine(const Line* line)
{
    for (RemoteClient* client : m_clients) {
        if (client->isAuthenticated()) {
            client->sendIncompleteLine(line);
        }
    }
}
