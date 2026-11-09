#pragma once
#include <QElapsedTimer>
#include <QHash>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QNetworkReply>

// Directory classification changes; the approved official roster never grows
// automatically. A missing/expired classification is not a non-official server.
class ServerPolicy : public QObject {
    Q_OBJECT
public:
    explicit ServerPolicy(QObject *parent = nullptr);
    void refresh();
    QJsonObject decision(const QJsonObject &server) const;
    static QJsonObject evaluate(const QJsonObject &server, const QJsonArray &directory);
signals:
    void changed();
private:
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QElapsedTimer m_attempt, m_age;
    QJsonArray m_directory;
};
