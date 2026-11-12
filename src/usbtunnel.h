#pragma once
#include <QObject>
#include <QProcess>
#include <QTcpSocket>
#include <QTimer>
class MobileHost;

// Explicit, unprivileged cable transport. TLS still terminates at MobileHost.
class UsbTunnel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString pairingLink READ pairingLink NOTIFY changed)
public:
    explicit UsbTunnel(MobileHost *host, QObject *parent=nullptr);
    ~UsbTunnel() override;
    bool running() const { return m_running; }
    QString message() const { return m_message; }
    QString pairingLink() const;
    Q_INVOKABLE bool start(const QString &mode);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void copyPairingLink();
signals:
    void changed();
private:
    void resetBridge();
    void pump(QTcpSocket &from, QTcpSocket &to);
    MobileHost *m_host;
    QProcess m_proxy;
    QTcpSocket m_phone, m_local;
    QTimer m_retry, m_startup;
    QString m_mode, m_message, m_tool, m_serial, m_stage;
    QByteArray m_output;
    bool m_running=false, m_resetting=false, m_ownedReverse=false;
};
