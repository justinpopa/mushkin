// ssh_server_session.h
// SSH transport for Remote Access Server
//
// Part of Remote Access Server feature

#ifndef SSH_SERVER_SESSION_H
#define SSH_SERVER_SESSION_H

#include "i_remote_transport.h"

#include <QByteArray>
#include <QSocketNotifier>
#include <QString>
#include <memory>
#include <vector>

// Forward-declare libssh types to avoid polluting headers.
struct ssh_session_struct;
typedef ssh_session_struct* ssh_session;
struct ssh_channel_struct;
typedef ssh_channel_struct* ssh_channel;
struct ssh_event_struct;
typedef ssh_event_struct* ssh_event;
struct ssh_message_struct;
typedef ssh_message_struct* ssh_message;
struct ssh_key_struct;
typedef ssh_key_struct* ssh_key;
struct ssh_channel_callbacks_struct;

class SshServerSession : public IRemoteTransport {
    Q_OBJECT

  public:
    // Takes ownership of the accepted ssh_session.
    SshServerSession(ssh_session session, const QString& password,
                     const QString& authorizedKeysFile, QObject* parent = nullptr);
    ~SshServerSession() override;

    // IRemoteTransport
    QByteArray processIncoming() override;
    QByteArray escapeOutgoing(const QByteArray& data) override;
    void writeRaw(const QByteArray& data) override;
    void close() override;
    [[nodiscard]] QString peerAddress() const override;
    [[nodiscard]] int terminalWidth() const override;
    [[nodiscard]] int terminalHeight() const override;

  private slots:
    void onSocketActivity();

  private:
    enum class Phase {
        KeyExchange,
        Authentication,
        WaitingChannel,
        WaitingShell,
        Established,
        Closing
    };

    void advanceHandshake();
    int handleMessage(ssh_message msg);
    int handleAuth(ssh_message msg);
    int handleChannelOpen(ssh_message msg);
    int handleChannelRequest(ssh_message msg);
    bool checkPassword(const char* password) const;
    bool checkPublicKey(ssh_key key) const;
    std::vector<ssh_key> loadAuthorizedKeys() const;

    // Channel callbacks (static trampolines -> instance methods)
    static int cbChannelData(ssh_session s, ssh_channel ch, void* data, uint32_t len, int is_stderr,
                             void* userdata);
    static void cbChannelEof(ssh_session s, ssh_channel ch, void* userdata);
    static void cbChannelClose(ssh_session s, ssh_channel ch, void* userdata);
    static int cbChannelPtyRequest(ssh_session s, ssh_channel ch, const char* term, int w, int h,
                                   int pw, int ph, void* userdata);
    static int cbChannelPtyWindowChange(ssh_session s, ssh_channel ch, int w, int h, int pw, int ph,
                                        void* userdata);
    static int cbChannelShellRequest(ssh_session s, ssh_channel ch, void* userdata);

    // Message callback (static trampoline)
    static int cbMessage(ssh_session s, ssh_message msg, void* userdata);

    ssh_session m_session = nullptr;
    ssh_channel m_channel = nullptr;
    ssh_event m_event = nullptr;
    // Qt parent-child owned (parent = this).
    QSocketNotifier* m_notifier = nullptr;
    std::unique_ptr<ssh_channel_callbacks_struct> m_channelCb;

    QString m_password;
    QString m_authorizedKeysFile;
    QByteArray m_cleanBuffer;
    Phase m_phase = Phase::KeyExchange;
    int m_termWidth = 80;
    int m_termHeight = 24;
};

#endif // SSH_SERVER_SESSION_H
