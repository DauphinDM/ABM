#include "serverpolicy.h"
#include <QFile>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QSet>

namespace {
QString endpoint(const QString &ip, int port) {
    QHostAddress address(ip);
    return !address.isNull() && port > 0 && port <= 65535 ? address.toString() + ':' + QString::number(port) : QString();
}
struct Roster {
    QSet<QString> approved, names;
    Roster() {
        QFile file(":/bridge/official-servers.json");
        if (!file.open(QIODevice::ReadOnly)) return;
        const auto root = QJsonDocument::fromJson(file.readAll()).object();
        if (root.value("schema").toInt() != 1) return;
        const auto groups = root.value("groups").toObject();
        const auto prefix = root.value("prefix").toString();
        for (auto g = groups.begin(); g != groups.end(); ++g) {
            for (const auto &entry : g.value().toArray()) {
                const auto row = entry.toArray();
                if (row.size() != 2) continue;
                const auto name = prefix + row[1].toString();
                // Snapshot addresses are historical metadata, not allow criteria.
                approved.insert(name); names.insert(name);
            }
        }
        for (const auto &entry : root.value("excluded").toArray()) {
            const auto row = entry.toArray();
            if (row.size() != 3) continue;
            names.insert(row[2].toString());
        }
    }
};
}

ServerPolicy::ServerPolicy(QObject *parent) : QObject(parent) {}
QJsonObject ServerPolicy::evaluate(const QJsonObject &server, const QJsonArray &directory) {
    static const Roster roster;
    const auto ip = server.value("serverIp").toString();
    const int port = server.value("serverPort").toInt();
    const auto name = server.value("serverRaw").toString();
    const auto key = endpoint(ip, port);
    QJsonObject out{{"verified", false}, {"allowed", false}, {"official", true},
        {"reason", tr("Server status unverified. Gameplay changes are locked.")}};
    if (!server.value("serverPort").isDouble() || server.value("serverPort").toDouble() != port
        || key.isEmpty() || name.isEmpty() || name.size() > 512 || roster.approved.isEmpty()) return out;
    QJsonObject found;
    bool official = roster.names.contains(name);
    for (const auto &entry : directory) {
        const auto row = entry.toObject();
        if (row.value("official").toBool() && row.value("sname").toString() == name) official = true;
        if (endpoint(row.value("ip").toString(), row.value("port").toInt()) != key) continue;
        // Conflicting duplicate identities are not safe to classify.
        if (!found.isEmpty()) return out;
        found = row;
    }
    if (found.isEmpty() || !found.value("official").isBool() || found.value("sname").toString() != name) return out;
    official |= found.value("official").toBool();
    const bool allowed = !official || roster.approved.contains(name);
    out = {{"verified", true}, {"allowed", allowed}, {"official", official},
        {"reason", allowed ? (official ? tr("Approved regular official server.") : tr("Non-official server: no ABM official-server restriction."))
                           : tr("Official event/new or mismatched server: gameplay changes are blocked.")}};
    return out;
}
QJsonObject ServerPolicy::decision(const QJsonObject &server) const {
    return evaluate(server, m_age.isValid() && m_age.elapsed() < 60000 ? m_directory : QJsonArray());
}
void ServerPolicy::refresh() {
    if (m_reply || (m_attempt.isValid() && m_attempt.elapsed() < 30000)) return;
    m_attempt.start();
    QNetworkRequest request(QUrl("https://backend.beammp.com/servers-info"));
    request.setTransferTimeout(10000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setRawHeader("User-Agent", "ABM/0.7 official-server-policy");
    auto *reply = m_network.get(request); m_reply = reply;
    reply->setReadBufferSize(8 * 1024 * 1024 + 1);
    connect(reply, &QNetworkReply::readyRead, this, [reply] {
        if (reply->bytesAvailable() > 8 * 1024 * 1024) reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const auto bytes = reply->readAll();
        const auto doc = QJsonDocument::fromJson(bytes);
        if (reply->error() == QNetworkReply::NoError && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200
            && bytes.size() <= 8 * 1024 * 1024 && doc.isArray() && !doc.array().isEmpty() && doc.array().size() <= 20000) {
            m_directory = doc.array(); m_age.start();
        } else { m_directory = {}; m_age.invalidate(); }
        m_reply = nullptr; reply->deleteLater(); emit changed();
    });
}
