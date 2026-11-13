#pragma once
#include <QObject>
#include <QSslServer>
#include <QSslSocket>
#include <QPointer>
#include <QTimer>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QHash>
#include <QSet>
#include <QQueue>
class Backend;

// Opt-in TLS control. Never expose the receiver's UDP/socket API.
class MobileHost : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString pairingLink READ pairingLink NOTIFY changed)
    Q_PROPERTY(QString pendingDevice READ pendingDevice NOTIFY changed)
    Q_PROPERTY(QString connectedDevice READ connectedDevice NOTIFY changed)
    Q_PROPERTY(QStringList addresses READ addresses CONSTANT)
    Q_PROPERTY(int rememberedCount READ rememberedCount NOTIFY changed)
    Q_PROPERTY(QString browserUrl READ browserUrl NOTIFY changed)
public:
    explicit MobileHost(Backend *backend, const QString &directory = {}, QObject *parent = nullptr);
    ~MobileHost() override;
    bool running() const { return m_server.isListening(); }
    QString message() const { return m_message; }
    QString pairingLink() const { return m_link; }
    QString pendingDevice() const { return m_pendingName; }
    QString connectedDevice() const { return m_device; }
    QStringList addresses() const;
    int rememberedCount() const { return m_trusted.size(); }
    QString browserUrl() const;
    QHostAddress listenAddress() const { return m_server.serverAddress(); }
    quint16 listenPort() const { return m_server.serverPort(); }
    static bool localAddress(const QHostAddress &address);
    Q_INVOKABLE bool start(const QString &address, int port = 44445);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void approve();
    Q_INVOKABLE void approveRemembered(bool remember);
    Q_INVOKABLE void forgetDevices();
    Q_INVOKABLE void reject();
    Q_INVOKABLE void copyPairingLink();
    Q_INVOKABLE void copyBrowserUrl();
    QJsonObject snapshot() const;
signals:
    void changed();
private:
    void receive(QSslSocket *socket);
    void send(QSslSocket *socket, const QJsonObject &message);
    void fail(QSslSocket *socket, const QString &message);
    void handleHttp(QSslSocket *socket);
    void handleHttpRequest(QSslSocket *socket, const QByteArray &method, const QByteArray &target,
                           const QHash<QByteArray, QByteArray> &headers, const QByteArray &body);
    void respond(QSslSocket *socket, int status, const QByteArray &type, const QByteArray &body,
                 const QByteArray &extra = {}, bool headOnly = false);
    void respondJson(QSslSocket *socket, int status, const QJsonObject &body, const QByteArray &extra = {}, bool headOnly = false);
    bool webCookieMatches(const QHash<QByteArray, QByteArray> &headers) const;
    void activateWeb(const QString &name);
    void endWeb(const QString &message);
    QJsonObject target() const;
    QString dispatch(const QJsonObject &command);
    void activate(QSslSocket *socket, const QString &name);
    void loadDevices();
    bool saveDevices();
    Backend *m_backend;
    QSslServer m_server;
    QHash<QSslSocket *, QByteArray> m_buffers;
    QPointer<QSslSocket> m_pending, m_client;
    QTimer m_publish, m_expiry;
    QElapsedTimer m_rate;
    int m_commands = 0, m_messages = 0;
    qint64 m_sequence = 0;
    QString m_directory, m_secret, m_link, m_message, m_pendingName, m_device;
    QJsonObject m_trusted;
    QByteArray m_libraryHash;
    QSet<QString> m_imageRequests;
    QQueue<QString> m_images;
    QTimer m_imageTimer;
    QByteArray m_webToken;
    bool m_webPending = false, m_webApproved = false;
    QString m_webName, m_webTrustedId, m_webTrustedToken;
    qint64 m_webExpires = 0, m_webSequence = 0;
    QElapsedTimer m_webRate, m_webPairRate;
    int m_webCommands = 0, m_webMessages = 0, m_webPairAttempts = 0;
};
