#include "mobilehost.h"
#include "backend.h"
#include "protocol.h"
#include <QNetworkInterface>
#include <QStandardPaths>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QSslKey>
#include <QGuiApplication>
#include <QClipboard>
#include <QSaveFile>
#include <QUuid>
#include <cmath>

MobileHost::MobileHost(Backend *backend, const QString &directory, QObject *parent)
    : QObject(parent), m_backend(backend), m_directory(directory.isEmpty()
      ? qEnvironmentVariable("BEAMSIDE_CONFIG_DIR", QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)) + "/mobile" : directory) {
    loadDevices();
    m_server.setMaxPendingConnections(4);
    m_server.setHandshakeTimeout(5000);
    connect(&m_server, &QTcpServer::pendingConnectionAvailable, this, [this] {
        while (m_server.hasPendingConnections()) {
            auto *socket = qobject_cast<QSslSocket *>(m_server.nextPendingConnection());
            if (!socket) continue;
            if (!localAddress(socket->peerAddress()) || m_buffers.size() >= 4) { socket->abort(); socket->deleteLater(); continue; }
            m_buffers.insert(socket, {});
            socket->setSocketOption(QAbstractSocket::LowDelayOption,1);
            socket->setReadBufferSize(32769);
            connect(socket, &QSslSocket::readyRead, this, [this, socket] { receive(socket); });
            connect(socket, &QSslSocket::disconnected, this, [this, socket] {
                m_buffers.remove(socket);
                if (m_pending == socket) { m_pending.clear(); m_pendingName.clear(); }
                if (m_client == socket) { m_backend->cancelPowerEdits(); m_client.clear(); m_device.clear(); m_message = tr("Phone disconnected. Remembered devices can reconnect while access is enabled."); }
                socket->deleteLater(); emit changed();
            });
            QTimer::singleShot(15000, socket, [this, socket] {
                if (m_pending != socket && m_client != socket) socket->abort();
            });
            if (socket->bytesAvailable()) receive(socket);
        }
    });
    m_publish.setInterval(100);
    connect(&m_publish, &QTimer::timeout, this, [this] {
        if (!m_client || m_client->bytesToWrite() > 65536) return; // Never queue stale telemetry behind a slow link.
        auto next = snapshot();
        const QJsonObject library{{"catalog",next.value("catalog")},{"configs",next.value("configs")},{"configsModel",next.value("configsModel")}};
        const auto hash = QCryptographicHash::hash(QJsonDocument(library).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256);
        if (m_libraryHash == hash) { next.remove("catalog"); next.remove("configs"); next.remove("configsModel"); }
        else m_libraryHash = hash;
        send(m_client,next);
    });
    m_expiry.setSingleShot(true); m_expiry.setInterval(120000);
    m_imageTimer.setInterval(50);
    connect(&m_imageTimer,&QTimer::timeout,this,[this] {
        if (!m_client || m_images.isEmpty() || m_client->bytesToWrite()>65536 || m_backend->busy()) return;
        const auto id=m_images.dequeue(); m_imageRequests.remove(id);
        const auto bytes=m_backend->mobileThumbnail(id);
        send(m_client,{{"kind","thumbnail"},{"id",id},{"jpeg",QString::fromLatin1(bytes.toBase64())}});
    });
    connect(&m_expiry, &QTimer::timeout, this, [this] {
        m_secret.clear(); m_link.clear(); reject(); m_message = tr("Pairing expired. Stop and enable again for a new link."); emit changed();
    });
}
void MobileHost::loadDevices() {
    QFile file(m_directory+"/devices.json");
    if (!file.open(QIODevice::ReadOnly) || file.size()>16384) return;
    const auto devices=QJsonDocument::fromJson(file.readAll()).object();
    const QRegularExpression hex("^[0-9a-f]{64}$"), id("^[0-9a-f]{32}$");
    for (auto it=devices.begin();it!=devices.end() && m_trusted.size()<16;++it) {
        const auto value=it.value().toObject();
        if (id.match(it.key()).hasMatch() && hex.match(value.value("hash").toString()).hasMatch()
            && !value.value("name").toString().isEmpty() && value.value("name").toString().size()<=64) m_trusted.insert(it.key(),value);
    }
}
bool MobileHost::saveDevices() {
    QSaveFile file(m_directory+"/devices.json");
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner);
    return file.write(QJsonDocument(m_trusted).toJson(QJsonDocument::Compact))>=0 && file.commit();
}
void MobileHost::forgetDevices() {
    const auto old=m_trusted; m_trusted={};
    if (!saveDevices()) { m_trusted=old; stop(); m_message=tr("Cannot revoke saved devices on disk. Access stopped; fix storage and retry."); emit changed(); return; }
    stop(); m_message=tr("All remembered devices revoked. Pair again to grant access."); emit changed();
}
MobileHost::~MobileHost() { stop(); }
bool MobileHost::localAddress(const QHostAddress &address) {
    bool ok = false; const auto ip = address.toIPv4Address(&ok);
    return ok && ((ip >> 24) == 10 || (ip >> 24) == 127 || (ip >> 16) == 0xc0a8 || (ip >> 20) == 0xac1);
}
QStringList MobileHost::addresses() const {
    QStringList result;
    for (const auto &address : QNetworkInterface::allAddresses())
        if (!address.isLoopback() && localAddress(address)) result << address.toString();
    result.removeDuplicates(); return result;
}
bool MobileHost::start(const QString &address, int port) {
    stop();
    const QHostAddress host(address);
    if (!localAddress(host) || port < 0 || port > 65535 || !QSslSocket::supportsSsl()) {
        m_message = tr("Choose a private IPv4 address; TLS support is required."); emit changed(); return false;
    }
    QDir().mkpath(m_directory);
    QFile::setPermissions(m_directory, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    const auto certificatePath = m_directory + "/certificate.pem", keyPath = m_directory + "/private-key.pem";
    if (!QFile::exists(certificatePath) || !QFile::exists(keyPath)) {
        // Generate locally; no shared application-wide key or downloaded certificate.
        QProcess openssl;
        openssl.start("openssl", {"req","-x509","-newkey","rsa:2048","-sha256","-nodes","-days","3650",
                                 "-subj","/CN=ABM Local Companion","-keyout",keyPath,"-out",certificatePath});
        if (!openssl.waitForFinished(15000) || openssl.exitStatus() != QProcess::NormalExit || openssl.exitCode() != 0) {
            m_message = tr("Cannot create a local TLS certificate. Install OpenSSL and put openssl on PATH, then retry."); emit changed(); return false;
        }
    }
    QFile::setPermissions(keyPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    QFile certFile(certificatePath), keyFile(keyPath);
    if (!certFile.open(QIODevice::ReadOnly) || !keyFile.open(QIODevice::ReadOnly)) { m_message = tr("Cannot read the local TLS identity."); emit changed(); return false; }
    QSslCertificate cert(certFile.readAll()); QSslKey key(keyFile.readAll(), QSsl::Rsa);
    if (cert.isNull() || key.isNull()) { m_message = tr("Invalid local TLS identity; no listener started."); emit changed(); return false; }
    auto configuration = QSslConfiguration::defaultConfiguration();
    configuration.setProtocol(QSsl::TlsV1_2OrLater); configuration.setLocalCertificate(cert); configuration.setPrivateKey(key);
    configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
    m_server.setSslConfiguration(configuration);
    if (!m_server.listen(host, quint16(port))) { m_message = m_server.errorString(); emit changed(); return false; }
    QByteArray random(32, Qt::Uninitialized);
    for (int i = 0; i < random.size(); ++i) random[i] = char(QRandomGenerator::system()->generate() & 255);
    m_secret = QString::fromLatin1(random.toHex());
    const QJsonObject invitation{{"v",1},{"host",address},{"port",m_server.serverPort()},
        {"sha256",QString::fromLatin1(cert.digest(QCryptographicHash::Sha256).toHex())},{"secret",m_secret}};
    m_link = "abm://pair/" + QString::fromLatin1(QJsonDocument(invitation).toJson(QJsonDocument::Compact).toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
    m_message = tr("Pairing available for two minutes. Treat the link as a password.");
    m_sequence = 0; m_rate.start(); m_commands = 0; m_expiry.start(); m_publish.start(); m_imageTimer.start(); emit changed(); return true;
}
void MobileHost::stop() {
    if (m_client) m_backend->cancelPowerEdits();
    m_server.close(); m_publish.stop(); m_expiry.stop(); m_imageTimer.stop();
    m_secret.clear(); m_link.clear(); m_pending.clear(); m_client.clear(); m_pendingName.clear(); m_device.clear();
    const auto sockets = m_buffers.keys(); m_buffers.clear();
    for (auto *socket : sockets) { disconnect(socket,nullptr,this,nullptr); socket->abort(); socket->deleteLater(); }
    m_libraryHash.clear(); m_imageRequests.clear(); m_images.clear();
    m_message = tr("Phone access is off. Remembered devices are retained; use Forget devices to revoke them."); emit changed();
}
void MobileHost::copyPairingLink() { if (!m_link.isEmpty() && QGuiApplication::clipboard()) QGuiApplication::clipboard()->setText(m_link); }
void MobileHost::send(QSslSocket *socket, const QJsonObject &message) {
    if (!socket || !socket->isEncrypted()) return;
    const auto bytes = QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n';
    if (socket->bytesToWrite() > 2*1024*1024 || bytes.size() > 1024*1024) { socket->abort(); return; }
    socket->write(bytes);
}
void MobileHost::fail(QSslSocket *socket, const QString &message) { send(socket, {{"kind","error"},{"message",message}}); socket->disconnectFromHost(); }
void MobileHost::receive(QSslSocket *socket) {
    if (!m_buffers.contains(socket)) return;
    m_buffers[socket] += socket->readAll();
    if (m_buffers.value(socket).size() > 32768) { socket->abort(); return; }
    // A failed send can synchronously disconnect and erase the socket's buffer.
    // Do not retain a reference into the hash across dispatch or socket writes.
    while (m_buffers.contains(socket) && m_buffers.value(socket).contains('\n')) {
        const int index = m_buffers.value(socket).indexOf('\n'); QJsonParseError error;
        const auto doc = QJsonDocument::fromJson(m_buffers.value(socket).left(index), &error);
        m_buffers[socket].remove(0,index+1);
        if (error.error != QJsonParseError::NoError || !doc.isObject()) { fail(socket,tr("Invalid message.")); return; }
        const auto data = doc.object();
        if (socket != m_client) {
            if (data.value("kind")=="resume" && data.value("v").toInt()==1 && !m_client && !m_pending) {
                const auto record=m_trusted.value(data.value("id").toString()).toObject();
                const auto token=data.value("token").toString();
                if (token.size()!=64 || record.isEmpty() || record.value("hash").toString()!=QString::fromLatin1(QCryptographicHash::hash(token.toLatin1(),QCryptographicHash::Sha256).toHex())) {
                    fail(socket,tr("Saved device revoked or invalid. Pair again.")); return;
                }
                activate(socket,record.value("name").toString()); continue;
            }
            const auto name = data.value("device").toString();
            if (m_client || m_secret.isEmpty() || m_pending || data.value("v").toInt() != 1 || data.value("kind") != "pair"
                || data.value("secret").toString() != m_secret || name.isEmpty() || name.size() > 64) { fail(socket,tr("Pairing refused or expired.")); return; }
            m_pending = socket; m_pendingName = name; m_message = tr("Approve this phone on the PC to allow control.");
            send(socket,{{"kind","pending"},{"message",m_message}}); emit changed();
        } else {
            if (m_rate.elapsed() >= 1000) { m_rate.restart(); m_commands = m_messages = 0; }
            if (++m_messages > 64) { fail(socket,tr("Message rate exceeded.")); return; }
            if (data.value("kind")=="ping") { send(socket,{{"kind","pong"},{"stamp",data.value("stamp")}}); continue; }
            if (data.value("kind")=="thumbnail") {
                const auto id=data.value("id").toString();
                if (m_images.size()>=32 || m_imageRequests.contains(id) || !QRegularExpression("^[0-9a-f]{64}$").match(id).hasMatch()) continue;
                m_imageRequests.insert(id); m_images.enqueue(id); continue;
            }
            const double seq = data.value("seq").toDouble(-1);
            if (data.value("kind") != "command" || !std::isfinite(seq) || seq <= m_sequence || seq > 1e9 || std::floor(seq) != seq) { fail(socket,tr("Invalid or repeated command.")); return; }
            m_sequence = qint64(seq);
            if (++m_commands > 10) { fail(socket,tr("Command rate exceeded.")); return; }
            send(socket,{{"kind","receipt"},{"seq",seq},{"message",dispatch(data)}});
        }
    }
}
void MobileHost::approve() {
    approveRemembered(false);
}
void MobileHost::approveRemembered(bool remember) {
    if (!m_pending || !m_expiry.isActive()) return;
    auto socket=m_pending; const auto name=m_pendingName;
    if (remember) {
        if (m_trusted.size()>=16) { m_message=tr("Remembered-device limit reached. Forget old devices first."); emit changed(); return; }
        const auto id=QUuid::createUuid().toString(QUuid::Id128);
        QByteArray token(32,Qt::Uninitialized); for (auto &byte:token) byte=char(QRandomGenerator::system()->generate()&255);
        const auto secret=QString::fromLatin1(token.toHex());
        m_trusted.insert(id,QJsonObject{{"name",name},{"hash",QString::fromLatin1(QCryptographicHash::hash(secret.toLatin1(),QCryptographicHash::Sha256).toHex())}});
        if (!saveDevices()) { m_trusted.remove(id); m_message=tr("Cannot save this device. Retry or approve without remembering."); emit changed(); return; }
        send(socket,{{"kind","trusted"},{"id",id},{"token",secret}});
    }
    activate(socket,name);
}
void MobileHost::activate(QSslSocket *socket, const QString &name) {
    m_client=socket; m_device=name; m_pending.clear(); m_pendingName.clear();
    m_secret.clear(); m_link.clear(); m_expiry.stop(); m_sequence = 0;
    m_libraryHash.clear(); m_imageRequests.clear(); m_images.clear(); m_rate.restart(); m_commands=m_messages=0;
    m_message = tr("Phone connected. Stop access to disconnect; Forget devices revokes saved access.");
    send(m_client,{{"kind","ready"}}); send(m_client,snapshot()); emit changed();
}
void MobileHost::reject() { if (m_pending) { auto socket = m_pending; m_pending.clear(); m_pendingName.clear(); fail(socket,tr("PC declined pairing.")); } emit changed(); }
QJsonObject MobileHost::target() const {
    const auto state = m_backend->state(); const auto workshop = state.value("workshop").toMap();
    return {{"session",state.value("sessionId").toString()},{"vehicle",QJsonValue::fromVariant(state.value("vehicleId"))},
            {"generation",workshop.value("generation").toString()},{"map",state.value("map").toString()},
            {"server",state.value("serverIp").toString()},{"port",state.value("serverPort").toInt()}};
}
QJsonObject MobileHost::snapshot() const {
    QJsonObject state; const auto source = QJsonObject::fromVariantMap(m_backend->state());
    for (const auto key : {"connected","sessionId","vehicleId","vehicle","vehicleOwned","map","multiplayer","joining","sandboxAllowed","policyCleanup","policyReason","bridgeVersion","controlsAvailable","telemetry","workshop","capabilities","server","serverIp","serverPort","ping","fps","trip","players","activeVehicles"})
        if (source.contains(key)) state.insert(key,source.value(key));
    auto safeItems = [](const QVariantList &items) {
        QJsonArray result;
        for (const auto &entry : items) {
            auto item = QJsonObject::fromVariantMap(entry.toMap());
            for (const auto key : {"thumbnail","modelThumbnail"}) {
                const auto url=item.take(key).toString();
                if (url.startsWith("image://vehicles/")) item.insert(QString(key)=="thumbnail"?"photo":"modelPhoto",url.mid(17));
            }
            item.remove("preview"); result.append(item);
        }
        return result;
    };
    return {{"kind","snapshot"},{"v",1},{"state",state},{"target",target()},{"busy",m_backend->busy()},
            {"actionMessage",m_backend->actionMessage()},{"catalog",safeItems(m_backend->catalog())},
            {"configs",safeItems(m_backend->configs())},{"configsModel",m_backend->configsModel()}};
}
QString MobileHost::dispatch(const QJsonObject &command) {
    const auto state = m_backend->state(); const auto action = command.value("action").toString(); const auto args = command.value("args").toObject();
    if (command.value("target").toObject() != target()) return tr("Game or vehicle changed. Review the current state and retry.");
    if (!m_backend->connected() || !state.value("controlsAvailable").toBool() || m_backend->busy() || m_backend->presetRunning()) return tr("Game bridge unavailable or busy.");
    if (action == "catalog") m_backend->requestCatalog();
    else if (action == "configs" && Protocol::model(args.value("model").toString())) m_backend->requestConfigs(args.value("model").toString());
    else {
        const auto speed = state.value("telemetry").toMap().value("speed");
        const bool spawning=action=="spawn";
        const bool noVehicle=!state.contains("vehicleId");
        if (state.value("joining").toBool() || state.value("policyCleanup").toBool() || !state.value("sandboxAllowed").toBool()
            || state.value("map").toString().isEmpty()
            || (!(spawning && noVehicle) && (!state.value("vehicleOwned").toBool() || !speed.isValid() || !std::isfinite(speed.toDouble()) || speed.toDouble()<0 || speed.toDouble() > 1)))
            return tr("Remote changes require a stopped, owned vehicle and server permission.");
        if (action == "power" && Protocol::enginePower(args.value("value"))) m_backend->setLivePower(args.value("value").toDouble());
        else if (action == "rpm" && Protocol::rpm(args.value("value"))) m_backend->setMaxRPM(args.value("value").toDouble());
        else if (action == "clutch" && args.value("enabled").isBool()) m_backend->setClutchProtection(args.value("enabled").toBool());
        else if (action == "persistence" && args.value("enabled").isBool()) m_backend->setResetPersistence(args.value("enabled").toBool());
        else if (action == "handling" && Protocol::handling(args)) m_backend->applyHandling(args.toVariantMap());
        else if (action == "engine" && Protocol::engine(args)) m_backend->applyEngine(args.toVariantMap());
        else if (action == "gears" && args.value("ratios").isString() && args.value("ratios").toString().size() <= 24000) m_backend->applyGears(args.value("ratios").toString());
        else if (action == "fullReset") m_backend->fullReset();
        else if (action == "resetTrip") m_backend->resetTrip();
        else if (QStringList{"repair","extinguish","stability","dealerStop"}.contains(action)) m_backend->carAction(action);
        else if ((action == "spawn" || action == "replace") && Protocol::identity(args.value("config").toString())) m_backend->requestVehicle(action,args.value("config").toString());
        else if (action == "switch" && args.value("id").isDouble() && args.value("id").toDouble() >= 0 && args.value("id").toDouble() <= 10000000 && std::floor(args.value("id").toDouble()) == args.value("id").toDouble()) {
            bool owned = false; for (const auto &v : m_backend->activeVehicles()) if (v.toMap().value("id").toInt() == args.value("id").toInt() && v.toMap().value("owned").toBool()) owned = true;
            if (!owned) return tr("Select an owned vehicle from the current roster.");
            m_backend->switchVehicle(args.value("id").toInt());
        } else return tr("Unsupported or invalid remote command.");
    }
    return tr("Request passed to the PC backend; game acceptance is not guaranteed.");
}
