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
    Q_PROPERTY(bool resolving READ resolving NOTIFY changed)
    Q_PROPERTY(QString messageRowKey READ messageRowKey NOTIFY changed)
public:
    explicit PlayerProfiles(QObject *parent = nullptr, QNetworkAccessManager *network = nullptr);
    QString message() const { return m_message; }
    bool resolving() const { return !m_intent.isEmpty(); }
    QString messageRowKey() const { return m_messageKey; }
    void setPlayers(const QString &session, const QVariantList &players);
    Q_INVOKABLE void openProfile(const QString &rawName);
    Q_INVOKABLE void copyAccount(const QString &rowKey);
    Q_INVOKABLE void copySessionId(const QString &rowKey);
    Q_INVOKABLE void copyName(const QString &rowKey);
    Q_INVOKABLE void copyDetails(const QString &rowKey);
    static QUrl profileUrl(const QString &name, bool json = false);
    static QString accountId(const QByteArray &bytes, const QString &name);
signals:
    void changed();
private:
    void tell(const QString &message);
    void copyText(const QString &text, const QString &message);
    QVariantMap row(const QString &key) const;
    QNetworkAccessManager *m_network;
    QPointer<QNetworkReply> m_reply;
    QString m_session, m_message, m_intent, m_intentName, m_messageKey;
    QVariantList m_players;
    quint64 m_clipboardRevision = 0, m_lookupRevision = 0;
};
