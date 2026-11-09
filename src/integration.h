#pragma once
#include <QObject>
#include <QSettings>
#include <QTimer>
#include <QElapsedTimer>
#include <functional>

class Integration : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool gameRunning READ gameRunning NOTIFY changed)
    Q_PROPERTY(bool beammpRunning READ beammpRunning NOTIFY changed)
    Q_PROPERTY(bool launching READ launching NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString userFolder READ userFolder WRITE setUserFolder NOTIFY changed)
    Q_PROPERTY(QString gameFolder READ gameFolder WRITE setGameFolder NOTIFY changed)
    Q_PROPERTY(QString launcherPath READ launcherPath WRITE setLauncherPath NOTIFY changed)
public:
    explicit Integration(const QString &settingsPath, bool discover = true, QObject *parent = nullptr);
    bool gameRunning() const { return m_game; }
    bool beammpRunning() const { return m_mp; }
    bool launching() const { return m_launching; }
    QString message() const { return m_message; }
    QString userFolder() const { return m_settings.value("userFolder").toString(); }
    QString gameFolder() const { return m_settings.value("gameFolder").toString(); }
    QString launcherPath() const { return m_settings.value("launcherPath").toString(); }
    void setUserFolder(const QString &value);
    void setGameFolder(const QString &value);
    void setLauncherPath(const QString &value);
    Q_INVOKABLE bool installBridge();
    bool startGame();
    bool startBeamMP();
    bool beamMPEnabled() const;
    void setMessage(const QString &message);
    Q_INVOKABLE void refreshProcesses();
    static QStringList processNames();
    // Tests inject the OS boundary; no real game is launched by the suite.
    std::function<QStringList()> processes = processNames;
    std::function<bool(const QString &, const QStringList &, const QString &)> execute;
signals:
    void changed();
private:
    void discoverPaths();
    bool launch(const QString &program, const QStringList &args, const QString &workingDirectory);
    bool writeBridgeFile(const QString &resource, const QString &relative, const QByteArray &marker);
    QSettings m_settings;
    QTimer m_poll, m_startup;
    QElapsedTimer m_mpLaunchAge;
    QString m_message;
    bool m_game = false, m_mp = false, m_launching = false;
};
