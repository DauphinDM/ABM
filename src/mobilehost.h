#pragma once
#include <QObject>
#include <QSslServer>
#include <QSslSocket>
#include <QPointer>
#include <QTimer>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QHash>
class Backend;

// Opt-in, session-only LAN control. Never expose the receiver's UDP/socket API.
class MobileHost : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString pairingLink READ pairingLink NOTIFY changed)
    Q_PROPERTY(QString pendingDevice READ pendingDevice NOTIFY changed)
    Q_PROPERTY(QString connectedDevice READ connectedDevice NOTIFY changed)
    Q_PROPERTY(QStringList addresses READ addresses CONSTANT)
public:
    explicit MobileHost(Backend *backend, const QString &directory = {}, QObject *parent = nullptr);
    ~MobileHost() override;
    bool running() const { return m_server.isListening(); }
    QString message() const { return m_message; }
    QString pairingLink() const { return m_link; }
    QString pendingDevice() const { return m_pendingName; }
    QString connectedDevice() const { return m_device; }
    QStringList addresses() const;
    static bool localAddress(const QHostAddress &address);
    Q_INVOKABLE bool start(const QString &address, int port = 44445);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void approve();
    Q_INVOKABLE void reject();
    Q_INVOKABLE void copyPairingLink();
    QJsonObject snapshot() const;
signals:
    void changed();
private:
    void receive(QSslSocket *socket);
    void send(QSslSocket *socket, const QJsonObject &message);
    void fail(QSslSocket *socket, const QString &message);
    QJsonObject target() const;
    QString dispatch(const QJsonObject &command);
    Backend *m_backend;
    QSslServer m_server;
    QHash<QSslSocket *, QByteArray> m_buffers;
    QPointer<QSslSocket> m_pending, m_client;
    QTimer m_publish, m_expiry;
    QElapsedTimer m_rate;
    int m_commands = 0;
    qint64 m_sequence = 0;
    QString m_directory, m_secret, m_link, m_message, m_pendingName, m_device;
};
