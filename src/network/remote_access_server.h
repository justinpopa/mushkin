// remote_access_server.h
// Remote Access Server for MUSHclient Qt
//
// Allows remote SSH clients to connect and control the MUD client.
// Each world has its own server instance.

#ifndef REMOTE_ACCESS_SERVER_H
#define REMOTE_ACCESS_SERVER_H

#include <QHostAddress>
#include <QList>
#include <QObject>
#include <expected>

enum class StartError {
    NoPassword,
    BindFailed,
    HostKeyFailed,
};

// Forward-declare libssh bind type to avoid polluting headers.
struct ssh_bind_struct;
typedef ssh_bind_struct* ssh_bind;

class QSocketNotifier;
class WorldDocument;
class RemoteClient;
class Line;

/** SSH server allowing remote clients to view output and send commands. */
class RemoteAccessServer : public QObject {
    Q_OBJECT

  public:
    explicit RemoteAccessServer(WorldDocument* doc, QObject* parent = nullptr);
    ~RemoteAccessServer() override;

    // Start the server. Binds to any address by default (SSH is encrypted).
    // Pass QHostAddress::LocalHost to restrict to loopback.
    std::expected<void, StartError> start(quint16 port,
                                          const QHostAddress& bindAddress = QHostAddress::Any);
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
    void setAuthorizedKeysFile(const QString& path);

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
    void broadcastLine(const Line* line);
    void broadcastIncompleteLine(const Line* line);

    WorldDocument* m_pDoc;
    // Owned via libssh (ssh_bind_free). nullptr when stopped.
    ssh_bind m_sshBind = nullptr;
    // Qt parent-child owned (parent = this). nullptr when stopped.
    QSocketNotifier* m_bindNotifier = nullptr;
    // Each RemoteClient is Qt-parented to this. Removed from list before deleteLater().
    QList<RemoteClient*> m_clients;
    QString m_password;
    QString m_authorizedKeysFile;
    quint16 m_port = 0;
    int m_scrollbackLines;
    int m_maxClients;
    int m_lastLineIndex;
};

#endif // REMOTE_ACCESS_SERVER_H
