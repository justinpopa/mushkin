// remote_access_server.cpp
// Remote Access Server for MUSHclient Qt
//
// Allows remote telnet clients to connect and control the MUD client.

#include "remote_access_server.h"
#include "../text/line.h"
#include "../world/world_document.h"
#include "remote_client.h"
#include <QTcpServer>
#include <QTcpSocket>

RemoteAccessServer::RemoteAccessServer(WorldDocument* doc, QObject* parent)
    : QObject(parent), m_pDoc(doc), m_pServer(nullptr), m_password(), m_scrollbackLines(100),
      m_maxClients(5), m_lastLineIndex(-1)
{
}

RemoteAccessServer::~RemoteAccessServer()
{
    stop();
}

bool RemoteAccessServer::start(quint16 port, const QHostAddress& bindAddress)
{
    if (m_pServer) {
        return m_pServer->isListening();
    }

    if (m_password.isEmpty()) {
        emit error("Cannot start server: password not set");
        return false;
    }

    m_pServer = new QTcpServer(this);

    connect(m_pServer, &QTcpServer::newConnection, this, &RemoteAccessServer::onNewConnection);

    if (!m_pServer->listen(bindAddress, port)) {
        emit error(QString("Failed to start server: %1").arg(m_pServer->errorString()));
        delete m_pServer;
        m_pServer = nullptr;
        return false;
    }

    // Connect to WorldDocument signals for output streaming
    if (m_pDoc) {
        connect(m_pDoc, &WorldDocument::linesAdded, this, &RemoteAccessServer::onLinesAdded);
        connect(m_pDoc, &WorldDocument::incompleteLine, this,
                &RemoteAccessServer::onIncompleteLine);

        // Initialize line tracking
        m_lastLineIndex = m_pDoc->m_lineList.count() - 1;
    }

    emit serverStarted(m_pServer->serverPort());
    return true;
}

void RemoteAccessServer::stop()
{
    // Disconnect all clients
    disconnectAllClients();

    // Stop listening
    if (m_pServer) {
        m_pServer->close();
        delete m_pServer;
        m_pServer = nullptr;
    }

    // Disconnect from WorldDocument signals
    if (m_pDoc) {
        disconnect(m_pDoc, &WorldDocument::linesAdded, this, &RemoteAccessServer::onLinesAdded);
        disconnect(m_pDoc, &WorldDocument::incompleteLine, this,
                   &RemoteAccessServer::onIncompleteLine);
    }

    emit serverStopped();
}

bool RemoteAccessServer::isRunning() const
{
    return m_pServer && m_pServer->isListening();
}

quint16 RemoteAccessServer::port() const
{
    return m_pServer ? m_pServer->serverPort() : 0;
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
    // Copy list since disconnecting modifies it
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

void RemoteAccessServer::onNewConnection()
{
    while (m_pServer->hasPendingConnections()) {
        QTcpSocket* socket = m_pServer->nextPendingConnection();

        // Check max clients
        if (m_maxClients > 0 && m_clients.count() >= m_maxClients) {
            socket->write("Server full. Please try again later.\r\n");
            socket->close();
            socket->deleteLater();
            continue;
        }

        // Create client handler
        RemoteClient* client =
            new RemoteClient(socket, m_pDoc, m_password, m_scrollbackLines, this);

        // Connect client signals
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
    if (m_pDoc) {
        m_pDoc->Execute(command);
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

    // Send any new lines since last update
    int totalLines = m_pDoc->m_lineList.count();
    int startIndex = m_lastLineIndex + 1;

    for (int i = startIndex; i < totalLines; ++i) {
        Line* line = m_pDoc->m_lineList.at(i);
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

    broadcastIncompleteLine(m_pDoc->m_currentLine);
}

void RemoteAccessServer::broadcastLine(Line* line)
{
    for (RemoteClient* client : m_clients) {
        if (client->isAuthenticated()) {
            client->sendLine(line);
        }
    }
}

void RemoteAccessServer::broadcastIncompleteLine(Line* line)
{
    for (RemoteClient* client : m_clients) {
        if (client->isAuthenticated()) {
            client->sendIncompleteLine(line);
        }
    }
}
