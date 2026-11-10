#include "playerprofiles.h"
#include <QClipboard>
#include <QGuiApplication>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <cmath>

PlayerProfiles::PlayerProfiles(QObject *parent) : QObject(parent) {
    if (auto *clipboard=QGuiApplication::clipboard())
        connect(clipboard,&QClipboard::dataChanged,this,[this]{ ++m_clipboardRevision; });
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
    m_session=session; m_players=players;
    if (!m_intent.isEmpty() && row(m_intent).isEmpty()) { ++m_lookupRevision; m_intent.clear(); if(m_reply) m_reply->abort(); }
}
void PlayerProfiles::openProfile(const QString &rawName) {
    bool present=false;
    for(const auto &p:m_players) present|=p.toMap().value("rawName").toString()==rawName;
    const auto url=profileUrl(rawName);
    if (!present || !url.isValid() || url.isEmpty()) { tell(tr("Profile unavailable for this name.")); return; }
    tell(QDesktopServices::openUrl(url)?tr("Opened public username profile; account ownership is not verified."):tr("Browser could not open the profile."));
}
void PlayerProfiles::copySessionId(const QString &key) {
    const auto p=row(key); const auto id=p.value("sessionPlayerId");
    if (!id.isValid() || !QGuiApplication::clipboard()) { tell(tr("Session ID unavailable.")); return; }
    ++m_lookupRevision; m_intent.clear(); if(m_reply) m_reply->abort();
    QGuiApplication::clipboard()->setText(id.toString()); tell(tr("Copied session ID, not permanent account ID."));
}
void PlayerProfiles::copyAccount(const QString &key) {
    ++m_lookupRevision; if(m_reply) m_reply->abort();
    const auto p=row(key); const auto name=p.value("rawName").toString(); const auto url=profileUrl(name,true);
    if (p.isEmpty() || p.value("guest").toBool() || url.isEmpty() || !QGuiApplication::clipboard()) { tell(tr("Permanent account ID unavailable (guest or unknown identity).")); return; }
    const auto session=m_session; const auto revision=m_lookupRevision, clip=m_clipboardRevision;
    m_intent=key;
    QNetworkRequest request(url); request.setTransferTimeout(10000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
    request.setRawHeader("User-Agent","ABM public-profile lookup");
    auto *reply=m_network.get(request); m_reply=reply; reply->setReadBufferSize(262145);
    connect(reply,&QNetworkReply::readyRead,reply,[reply]{ if(reply->bytesAvailable()>262144) reply->abort(); });
    tell(tr("Resolving public username → forum ID. Clipboard changes cancel this copy."));
    connect(reply,&QNetworkReply::finished,this,[this,reply,key,name,session,revision,clip] {
        const auto bytes=reply->readAll();
        const bool valid=reply->error()==QNetworkReply::NoError && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()==200;
        reply->deleteLater();
        if (revision!=m_lookupRevision || session!=m_session || m_intent!=key || row(key).value("rawName").toString()!=name || clip!=m_clipboardRevision) return;
        m_intent.clear(); const auto id=valid?accountId(bytes,name):QString();
        if (id.isEmpty()) { tell(tr("Account lookup failed or canonical name differs. No session ID was substituted.")); return; }
        auto *clipboard=QGuiApplication::clipboard(); clipboard->setText(id);
        tell(clipboard->text()==id?tr("Copied public forum account ID %1 (name mapping, not ownership proof).").arg(id):tr("Clipboard did not retain the account ID."));
    });
}
