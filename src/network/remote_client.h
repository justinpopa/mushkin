// remote_client.h
// Per-connection handler for Remote Access Server
//
// Part of Remote Access Server feature

#ifndef REMOTE_CLIENT_H
#define REMOTE_CLIENT_H

#include <QAbstractSocket>
#include <QDateTime>
#include <QObject>
#include <QString>
#include <memory>

class QTcpSocket;
class Line;
class WorldDocument;
class TelnetServerSession;
class AnsiFormatter;

/** Handles a single remote client connection with authentication and streaming. */
class RemoteClient : public QObject {
    Q_OBJECT

  public:
    RemoteClient(QTcpSocket* socket, WorldDocument* doc, const QString& password,
                 int scrollbackLines, QObject* parent = nullptr);
    ~RemoteClient();

    bool isAuthenticated() const;
    QString address() const;
    QDateTime connectedAt() const;
    void sendLine(Line* line);
    void sendIncompleteLine(Line* line);
    void sendRawText(const QString& text, bool includeNewline = true);
    void disconnect();

  signals:
    void authenticated();
    void commandReceived(const QString& command);
    void disconnected();
    void error(const QString& message);

  private slots:
    void onReadyRead();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);
    void onNegotiationComplete();

  private:
    enum class State { Negotiating, AwaitingPassword, Authenticated, Disconnecting };

    void processInput(const QByteArray& data);
    void sendWelcome();
    void sendPasswordPrompt();
    bool checkPassword(const QString& attempt);
    void handleAuthSuccess();
    void handleAuthFailure();
    void sendScrollback();
    void sendBytes(const QByteArray& data);

    QTcpSocket* m_pSocket;
    WorldDocument* m_pDoc;
    std::unique_ptr<TelnetServerSession> m_telnet;
    std::unique_ptr<AnsiFormatter> m_formatter;
    QString m_password;
    int m_scrollbackLines;
    State m_state;
    int m_failedAttempts;
    int m_maxFailedAttempts;
    QDateTime m_connectedAt;
    QString m_inputBuffer;
};

#endif // REMOTE_CLIENT_H
