#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QSaveFile>
#include <QCryptographicHash>
#include <QTimer>
#include <QUrl>
#include <QJsonArray>
#include <memory>

// Mirrored in ABM-Mobile. Remote responses cannot select a repository or executable.
class AppUpdates : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool automatic READ automatic WRITE setAutomatic NOTIFY changed)
    Q_PROPERTY(bool previews READ previews WRITE setPreviews NOTIFY changed)
    Q_PROPERTY(bool autoDownload READ autoDownload WRITE setAutoDownload NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(bool downloaded READ downloaded NOTIFY changed)
    Q_PROPERTY(bool canDownload READ canDownload NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString version READ version NOTIFY changed)
    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QString installHelp READ installHelp CONSTANT)
    Q_PROPERTY(double progress READ progress NOTIFY changed)
public:
    struct Release { QString version, name, sha256; QUrl page, asset; qint64 size=0; };
    explicit AppUpdates(bool mobile, QString current, QString directory={}, QObject *parent=nullptr, QNetworkAccessManager *network=nullptr);
    ~AppUpdates() override;
    bool automatic() const { return m_automatic; }
    bool previews() const { return m_previews; }
    bool autoDownload() const { return m_autoDownload; }
    bool busy() const { return !m_reply.isNull(); }
    bool available() const { return !m_release.version.isEmpty(); }
    bool downloaded() const { return !m_download.isEmpty(); }
    bool canDownload() const { return !m_release.asset.isEmpty(); }
    QString message() const { return m_message; }
    QString version() const { return m_release.version; }
    QString currentVersion() const { return m_current; }
    QString installHelp() const;
    double progress() const { return m_progress; }
    void setAutomatic(bool value);
    void setPreviews(bool value);
    void setAutoDownload(bool value);
    void start();
    Q_INVOKABLE void check();
    Q_INVOKABLE void download();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void openRelease();
    Q_INVOKABLE void openDownload();
    static Release selectRelease(const QJsonArray &feed, const QString &repo, const QString &current, bool previews, const QString &platform);
    static bool trustedDownload(const QUrl &url);
signals:
    void changed();
private:
    void savePreferences();
    void request(const QUrl &url, bool asset, int redirects=0);
    void fail(const QString &message);
    QString m_repo, m_current, m_directory, m_platform, m_message, m_download;
    bool m_automatic=false, m_previews=false, m_autoDownload=false;
    double m_progress=0;
    Release m_release;
    QNetworkAccessManager *m_network;
    QPointer<QNetworkReply> m_reply;
    QByteArray m_feed;
    std::unique_ptr<QSaveFile> m_file;
    QCryptographicHash m_hash{QCryptographicHash::Sha256};
    qint64 m_received=0;
    QTimer m_daily;
};
