// remote_access_server.h
// Remote Access Server for MUSHclient Qt
//
// Allows remote telnet clients to connect and control the MUD client.
// Each world has its own server instance.

#ifndef REMOTE_ACCESS_SERVER_H
#define REMOTE_ACCESS_SERVER_H

#include <QHostAddress>
#include <QList>
#include <QObject>

class QTcpServer;
class WorldDocument;
class RemoteClient;
class Line;

/** TCP server allowing remote telnet clients to view output and send commands. */
class RemoteAccessServer : public QObject {
    Q_OBJECT

  public:
    explicit RemoteAccessServer(WorldDocument* doc, QObject* parent = nullptr);
    ~RemoteAccessServer();

    // Start the server. By default binds to localhost only for security.
    // Pass QHostAddress::Any to allow connections from other machines.
    bool start(quint16 port, const QHostAddress& bindAddress = QHostAddress::LocalHost);
    void stop();
    bool isRunning() const;
    quint16 port() const;
    int clientCount() const;
    int authenticatedClientCount() const;
    void disconnectAllClients();
    void broadcastMessage(const QString& message);
    void setPassword(const QString& password);
    void setScrollbackLines(int lines);
    void setMaxClients(int max);

  signals:
    void serverStarted(quint16 port);
    void serverStopped();
    void clientConnected(const QString& address);
    void clientDisconnected(const QString& address);
    void clientAuthenticated(const QString& address);
    void error(const QString& message);

  private slots:
    void onNewConnection();
    void onClientAuthenticated();
    void onClientCommand(const QString& command);
    void onClientDisconnected();
    void onLinesAdded();
    void onIncompleteLine();

  private:
    void broadcastLine(Line* line);
    void broadcastIncompleteLine(Line* line);

    WorldDocument* m_pDoc;
    QTcpServer* m_pServer;
    QList<RemoteClient*> m_clients;
    QString m_password;
    int m_scrollbackLines;
    int m_maxClients;
    int m_lastLineIndex;
};

#endif // REMOTE_ACCESS_SERVER_H
