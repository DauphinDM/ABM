#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QNetworkReply>
#include <QVariantList>

// Public forum name mapping is not proof of ownership of an in-game account.
class PlayerProfiles : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString message READ message NOTIFY changed)
public:
    explicit PlayerProfiles(QObject *parent = nullptr);
    QString message() const { return m_message; }
    void setPlayers(const QString &session, const QVariantList &players);
    Q_INVOKABLE void openProfile(const QString &rawName);
    Q_INVOKABLE void copyAccount(const QString &rowKey);
    Q_INVOKABLE void copySessionId(const QString &rowKey);
    static QUrl profileUrl(const QString &name, bool json = false);
    static QString accountId(const QByteArray &bytes, const QString &name);
signals:
    void changed();
private:
    void tell(const QString &message);
    QVariantMap row(const QString &key) const;
    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
    QString m_session, m_message, m_intent;
    QVariantList m_players;
    quint64 m_clipboardRevision = 0, m_lookupRevision = 0;
};
