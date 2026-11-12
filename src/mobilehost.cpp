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
#include <cmath>

MobileHost::MobileHost(Backend *backend, const QString &directory, QObject *parent)
    : QObject(parent), m_backend(backend), m_directory(directory.isEmpty()
      ? qEnvironmentVariable("BEAMSIDE_CONFIG_DIR", QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)) + "/mobile" : directory) {
    m_server.setMaxPendingConnections(4);
    m_server.setHandshakeTimeout(5000);
    connect(&m_server, &QTcpServer::pendingConnectionAvailable, this, [this] {
        while (m_server.hasPendingConnections()) {
            auto *socket = qobject_cast<QSslSocket *>(m_server.nextPendingConnection());
            if (!socket) continue;
            if (!localAddress(socket->peerAddress()) || m_buffers.size() >= 4) { socket->abort(); socket->deleteLater(); continue; }
            m_buffers.insert(socket, {});
            socket->setReadBufferSize(32769);
            connect(socket, &QSslSocket::readyRead, this, [this, socket] { receive(socket); });
            connect(socket, &QSslSocket::disconnected, this, [this, socket] {
                m_buffers.remove(socket);
                if (m_pending == socket) { m_pending.clear(); m_pendingName.clear(); }
                if (m_client == socket) { m_backend->cancelPowerEdits(); m_client.clear(); m_device.clear(); m_message = tr("Phone disconnected. Stop and enable again to pair."); }
                socket->deleteLater(); emit changed();
            });
            QTimer::singleShot(15000, socket, [this, socket] {
                if (m_pending != socket && m_client != socket) socket->abort();
            });
            if (socket->bytesAvailable()) receive(socket);
        }
    });
    m_publish.setInterval(250);
    connect(&m_publish, &QTimer::timeout, this, [this] { if (m_client) send(m_client, snapshot()); });
    m_expiry.setSingleShot(true); m_expiry.setInterval(120000);
    connect(&m_expiry, &QTimer::timeout, this, [this] {
        m_secret.clear(); m_link.clear(); reject(); m_message = tr("Pairing expired. Stop and enable again for a new link."); emit changed();
    });
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
    m_sequence = 0; m_rate.start(); m_commands = 0; m_expiry.start(); m_publish.start(); emit changed(); return true;
}
void MobileHost::stop() {
    if (m_client) m_backend->cancelPowerEdits();
    m_server.close(); m_publish.stop(); m_expiry.stop();
    m_secret.clear(); m_link.clear(); m_pending.clear(); m_client.clear(); m_pendingName.clear(); m_device.clear();
    const auto sockets = m_buffers.keys(); m_buffers.clear();
    for (auto *socket : sockets) { disconnect(socket,nullptr,this,nullptr); socket->abort(); socket->deleteLater(); }
    m_message = tr("Phone access is off. All approvals revoked."); emit changed();
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
            const auto name = data.value("device").toString();
            if (m_secret.isEmpty() || m_pending || data.value("v").toInt() != 1 || data.value("kind") != "pair"
                || data.value("secret").toString() != m_secret || name.isEmpty() || name.size() > 64) { fail(socket,tr("Pairing refused or expired.")); return; }
            m_pending = socket; m_pendingName = name; m_message = tr("Approve this phone on the PC to allow control.");
            send(socket,{{"kind","pending"},{"message",m_message}}); emit changed();
        } else {
            const double seq = data.value("seq").toDouble(-1);
            if (data.value("kind") != "command" || !std::isfinite(seq) || seq <= m_sequence || seq > 1e9 || std::floor(seq) != seq) { fail(socket,tr("Invalid or repeated command.")); return; }
            m_sequence = qint64(seq);
            if (m_rate.elapsed() >= 1000) { m_rate.restart(); m_commands = 0; }
            if (++m_commands > 10) { fail(socket,tr("Command rate exceeded.")); return; }
            send(socket,{{"kind","receipt"},{"seq",seq},{"message",dispatch(data)}});
        }
    }
}
void MobileHost::approve() {
    if (!m_pending || !m_expiry.isActive()) return;
    m_client = m_pending; m_pending.clear(); m_device = m_pendingName; m_pendingName.clear();
    m_secret.clear(); m_link.clear(); m_expiry.stop(); m_sequence = 0;
    m_message = tr("Phone approved. Stop access to revoke immediately.");
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
        for (const auto &entry : items) { auto item = QJsonObject::fromVariantMap(entry.toMap()); item.remove("preview"); item.remove("thumbnail"); item.remove("modelThumbnail"); result.append(item); }
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
        if (state.value("joining").toBool() || state.value("policyCleanup").toBool() || !state.value("sandboxAllowed").toBool()
            || !state.value("vehicleOwned").toBool() || !speed.isValid() || !std::isfinite(speed.toDouble()) || speed.toDouble() > 1)
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
