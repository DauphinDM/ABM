#include "playerprofiles.h"
#include <QClipboard>
#include <QGuiApplication>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>

PlayerProfiles::PlayerProfiles(QObject *parent, QNetworkAccessManager *network)
    : QObject(parent), m_network(network ? network : new QNetworkAccessManager(this)) {
    if (auto *clipboard=QGuiApplication::clipboard())
        connect(clipboard,&QClipboard::dataChanged,this,[this]{
            ++m_clipboardRevision;
            if (!m_intent.isEmpty()) {
                ++m_lookupRevision; m_messageKey=m_intent; m_intent.clear(); if(m_reply) m_reply->abort();
                tell(tr("Clipboard changed. Forum ID copy canceled."));
            }
        });
}
void PlayerProfiles::tell(const QString &message) { m_message=message; emit changed(); }
QUrl PlayerProfiles::profileUrl(const QString &name, bool json) {
    if (name.isEmpty() || name.size()>64 || name.trimmed()!=name) return {};
    for (auto c:name) if (c.isNull() || c.unicode()<32 || c=='/' || c=='\\' || c.unicode()==127) return {};
    return QUrl::fromEncoded("https://forum.beammp.com/u/"+QUrl::toPercentEncoding(name)+(json?".json":"/summary"),QUrl::StrictMode);
}
QString PlayerProfiles::accountId(const QByteArray &bytes, const QString &name) {
    if (bytes.size()>262144) return {};
    const auto user=QJsonDocument::fromJson(bytes).object().value("user").toObject();
    const auto id=user.value("id"); const auto n=id.toDouble();
    if (user.value("username").toString()!=name || !id.isDouble() || !std::isfinite(n) || n<1 || n>9007199254740991.0 || n!=std::floor(n)) return {};
    return QString::number(qint64(n));
}
QVariantMap PlayerProfiles::row(const QString &key) const {
    for (const auto &entry:m_players) if (entry.toMap().value("rowKey").toString()==key) return entry.toMap();
    return {};
}
void PlayerProfiles::setPlayers(const QString &session, const QVariantList &players) {
    const bool sessionChanged = session != m_session;
    m_session=session; m_players=players;
    if (!m_intent.isEmpty() && (sessionChanged || row(m_intent).value("rawName").toString()!=m_intentName || row(m_intent).value("guest").toBool())) {
        ++m_lookupRevision; m_messageKey=m_intent; m_intent.clear(); if(m_reply) m_reply->abort();
        tell(tr("Player or server changed. Forum ID lookup canceled."));
    }
}
void PlayerProfiles::openProfile(const QString &rawName) {
    bool present=false;
    m_messageKey.clear();
    for(const auto &p:m_players) if (p.toMap().value("rawName").toString()==rawName && !p.toMap().value("guest").toBool()) {
        present=true; m_messageKey=p.toMap().value("rowKey").toString(); break;
    }
    const auto url=profileUrl(rawName);
    if (!present || !url.isValid() || url.isEmpty()) { tell(tr("Profile unavailable for this name.")); return; }
    tell(QDesktopServices::openUrl(url)?tr("Opened public username profile; account ownership is not verified."):tr("Browser could not open the profile."));
}
void PlayerProfiles::copySessionId(const QString &key) {
    m_messageKey=key;
    const auto p=row(key); const auto id=p.value("sessionPlayerId");
    if (!id.isValid() || !QGuiApplication::clipboard()) { tell(tr("Session ID unavailable.")); return; }
    copyText(id.toString(),tr("Copied session ID, not permanent account ID."));
}
void PlayerProfiles::copyText(const QString &text, const QString &message) {
    if (text.isEmpty() || !QGuiApplication::clipboard()) { tell(tr("Player information unavailable.")); return; }
    ++m_lookupRevision; m_intent.clear(); if (m_reply) m_reply->abort();
    auto *clipboard = QGuiApplication::clipboard();
    clipboard->setText(text);
    tell(clipboard->text() == text ? message : tr("Clipboard did not retain the player information."));
}
void PlayerProfiles::copyName(const QString &key) {
    m_messageKey=key;
    copyText(row(key).value("rawName").toString(), tr("Copied player name."));
}
void PlayerProfiles::copyDetails(const QString &key) {
    m_messageKey=key;
    const auto p = row(key);
    if (p.isEmpty()) { tell(tr("Player information unavailable.")); return; }
    QStringList lines;
    lines << tr("Name: %1").arg(p.value("rawName").toString());
    if (p.contains("sessionPlayerId")) lines << tr("Session ID: %1 (not a permanent account ID)").arg(p.value("sessionPlayerId").toString());
    if (!p.value("role").toString().isEmpty()) lines << tr("Role: %1").arg(p.value("role").toString());
    if (p.contains("vehicles")) lines << tr("Vehicles: %1").arg(p.value("vehicles").toString());
    if (p.contains("ping")) lines << tr("Ping: %1 ms (%2)").arg(p.value("ping").toString(), p.value("pingSource").toString());
    copyText(lines.join('\n'), tr("Copied session player details."));
}
void PlayerProfiles::copyAccount(const QString &key) {
    m_messageKey=key;
    ++m_lookupRevision; m_intent.clear(); if(m_reply) m_reply->abort();
    const auto p=row(key); const auto name=p.value("rawName").toString(); const auto url=profileUrl(name,true);
    if (p.isEmpty() || p.value("guest").toBool() || url.isEmpty() || !QGuiApplication::clipboard()) { tell(tr("Public forum ID unavailable (guest or unknown identity).")); return; }
    const auto session=m_session; const auto revision=m_lookupRevision, clip=m_clipboardRevision;
    m_intent=key; m_intentName=name;
    QNetworkRequest request(url); request.setTransferTimeout(10000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
    request.setRawHeader("User-Agent","ABM public-profile lookup");
    auto *reply=m_network->get(request); m_reply=reply; reply->setReadBufferSize(262145);
    connect(reply,&QNetworkReply::readyRead,reply,[reply]{ if(reply->bytesAvailable()>262144) reply->abort(); });
    tell(tr("Resolving public username → forum ID. Clipboard changes cancel this copy."));
    connect(reply,&QNetworkReply::finished,this,[this,reply,key,name,session,revision,clip] {
        const auto bytes=reply->readAll();
        const bool valid=reply->error()==QNetworkReply::NoError && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200;
        reply->deleteLater();
        if (revision!=m_lookupRevision || session!=m_session || m_intent!=key || row(key).value("rawName").toString()!=name) return;
        m_messageKey=key;
        if (clip!=m_clipboardRevision) { m_intent.clear(); tell(tr("Clipboard changed. Forum ID copy canceled.")); return; }
        m_intent.clear(); const auto id=valid?accountId(bytes,name):QString();
        if (id.isEmpty()) { tell(tr("Account lookup failed or canonical name differs. No session ID was substituted.")); return; }
        auto *clipboard=QGuiApplication::clipboard(); clipboard->setText(id);
        tell(clipboard->text()==id?tr("Copied public forum account ID %1 (name mapping, not ownership proof).").arg(id):tr("Clipboard did not retain the account ID."));
    });
}
