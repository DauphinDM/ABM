#pragma once
#include <QObject>
#include <QProcess>
class MobileHost;

// Permission is requested only from Connect, never when opening the app/listener.
class CompanionFirewall : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool supported READ supported NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString scope READ scope NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
public:
    struct Rule {
        QString address, subnet, interfaceName, application;
        int port = 0;
        bool valid() const { return !subnet.isEmpty(); }
        QString scope() const;
    };
    explicit CompanionFirewall(MobileHost *host, QObject *parent = nullptr);
    ~CompanionFirewall() override;
    bool supported() const;
    bool busy() const { return m_busy; }
    QString scope() const { return m_rule.scope(); }
    QString message() const { return m_message; }
    Q_INVOKABLE void requestAccess(const QString &confirmedScope);
    static Rule ruleFor(const QString &address, int prefix, const QString &interfaceName,
                        const QString &application, int port);
    static QStringList ufwArguments(const Rule &rule);
    static QString windowsScript(const Rule &rule);
    static QString encodedScript(const QString &script);
signals:
    void changed();
private:
    void refresh();
    MobileHost *m_host;
    QProcess m_process;
    Rule m_rule;
    QString m_tool, m_elevation, m_message, m_binding;
    bool m_busy = false;
};
