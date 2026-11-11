#include "telemetry.h"
#include "protocol.h"
#include <QDir>
#include <QDateTime>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkDatagram>
#include <QSaveFile>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QUuid>
#include <cmath>

namespace {
QString bounded(const QJsonValue &value, int limit = 160) {
    QString text = value.toString().left(limit);
    for (auto &c : text)
        if (c.unicode() < 32 || (c.unicode() >= 0x202a && c.unicode() <= 0x202e)
            || (c.unicode() >= 0x2066 && c.unicode() <= 0x2069)) c = QChar(' ');
    return text;
}
void number(QJsonObject &out, const QJsonObject &in, const QString &key, double low, double high) {
    const auto v = in.value(key);
    if (v.isDouble() && std::isfinite(v.toDouble()) && v.toDouble() >= low && v.toDouble() <= high)
        out.insert(key, v);
}
}

QString Telemetry::defaultPath() {
    const auto override = qEnvironmentVariable("BEAMSIDE_RUNTIME_DIR");
    if (!override.isEmpty()) return override + "/state.json";
#ifdef Q_OS_WIN
    auto root = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#else
    auto root = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
#endif
    if (root.isEmpty()) root = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    return root + "/abm/state.json";
}
QString Telemetry::socketPath(const QString &path) {
#ifdef Q_OS_WIN
    return "beamside-" + QString::fromLatin1(QCryptographicHash::hash(QFileInfo(path).absoluteFilePath().toUtf8(), QCryptographicHash::Sha256).toHex().left(32));
#else
    return QFileInfo(path).dir().filePath("control.sock");
#endif
}
QString Telemetry::cleanName(const QString &text) {
    QString clean = text;
    // BeamMP uses caret-prefixed style/color tokens, not HTML.
    clean.remove(QRegularExpression("\\^[0-9A-Za-z*]"));
    return clean.simplified();
}

QJsonObject Telemetry::sanitize(const QByteArray &bytes) {
    if (bytes.size() > 32768) return {};
    const auto doc = QJsonDocument::fromJson(bytes);
    if (!doc.isObject()) return {};
    const auto in = doc.object();
    if (in.value("schema").toInt() != 1 || in.value("source").toString() != "beamdeck") return {};
    QJsonObject out{{"schema", 1}, {"source", "beamdeck"}};
    for (const auto &key : {"map", "vehicle", "vehicleName", "version", "sessionId", "bridgeVersion"})
        out.insert(key, bounded(in.value(key)));
    for (const auto key : {"gameFolder", "userFolder"}) out.insert(key, bounded(in.value(key), 2048));
    for (const auto &key : {"paused", "beammpLoaded", "multiplayer", "joining"})
        out.insert(key, in.value(key).toBool());
    out.insert("sandboxAllowed", in.value("sandboxAllowed").toBool());
    out.insert("policyReason", bounded(in.value("policyReason"), 256));
    out.insert("policyCleanup", in.value("policyCleanup").toBool());
    if (in.value("vehicleOwned").isBool()) out.insert("vehicleOwned", in.value("vehicleOwned"));
    QJsonObject capabilities;
    const auto incomingCaps = in.value("capabilities").toObject();
    for (const auto key : {"power", "engine", "grip", "brakes", "steering", "gears", "dimensions", "deformation", "stability", "ignite", "extinguish", "deflate", "dealer", "repair","rpm","clutch"})
        if (incomingCaps.value(key).isBool()) capabilities.insert(key, incomingCaps.value(key));
    out.insert("capabilities", capabilities);
    const auto incomingWorkshop = in.value("workshop").toObject();
    if (incomingWorkshop.value("contract").toInt() == 1 && !incomingWorkshop.value("generation").toString().isEmpty()) {
        QJsonObject workshop{{"contract",1},{"generation",bounded(incomingWorkshop.value("generation"),160)}};
        if (Protocol::enginePower(incomingWorkshop.value("enginePower"))) workshop.insert("enginePower",incomingWorkshop.value("enginePower"));
        QJsonArray engines, gears;
        for (const auto &value : incomingWorkshop.value("engines").toArray()) {
            if (engines.size() >= 8) break;
            const auto item=value.toObject(); QJsonObject clean{{"name",bounded(item.value("name"),80)}};
            for (const auto key : {"maxRPM","revLimiterRPM","physicalRPM","rpmCeiling","idleRPM","maxTorqueRPM","maxPowerRPM"}) number(clean,item,key,0,30000);
            for (const auto key : {"maxTorque","maxPower"}) number(clean,item,key,0,1e9);
            QJsonArray curve;
            for (const auto &point : item.value("torqueCurve").toArray()) {
                if (curve.size() >= 64) break;
                const auto p=point.toObject(); const auto rpm=p.value("rpm"), torque=p.value("torque");
                if (rpm.isDouble() && std::isfinite(rpm.toDouble()) && rpm.toDouble()>=0 && rpm.toDouble()<=30000
                    && torque.isDouble() && std::isfinite(torque.toDouble()) && std::abs(torque.toDouble())<=1e7)
                    curve.append(QJsonObject{{"rpm",rpm},{"torque",torque}});
            }
            if (!curve.isEmpty()) clean.insert("torqueCurve",curve);
            engines.append(clean);
        }
        for (const auto &value : incomingWorkshop.value("gears").toArray()) {
            if (gears.size() >= 273) break;
            const auto item=value.toObject(); const auto index=item.value("index"), ratio=item.value("ratio");
            if (index.isDouble() && index.toDouble()==index.toInt() && index.toInt()>=-16 && index.toInt()<=256 && ratio.isDouble() && std::isfinite(ratio.toDouble()) && std::abs(ratio.toDouble())<=100)
                gears.append(QJsonObject{{"index",index},{"ratio",ratio}});
        }
        workshop.insert("engines",engines); workshop.insert("gears",gears);
        QJsonArray clutches;
        for (const auto &value : incomingWorkshop.value("clutches").toArray()) {
            if (clutches.size() >= 4) break;
            const auto item=value.toObject(); const auto name=bounded(item.value("name"),80);
            if (name.isEmpty()) continue;
            QJsonObject clean{{"name",name}};
            number(clean,item,"temperature",-100,2000);
            number(clean,item,"maxSafeTemperature",-100,2000);
            number(clean,item,"efficiency",0,1);
            if (item.value("permanentlyDamaged").isBool()) clean.insert("permanentlyDamaged",item.value("permanentlyDamaged"));
            clutches.append(clean);
        }
        workshop.insert("clutches",clutches);
        workshop.insert("gearbox",bounded(incomingWorkshop.value("gearbox"),80));
        const auto gearType=bounded(incomingWorkshop.value("gearboxType"),16);
        if (QStringList{"manual","automatic","dct","sequential","cvt"}.contains(gearType)) workshop.insert("gearboxType",gearType);
        number(workshop,incomingWorkshop,"gearCount",0,256);
        for (const auto key : {"minGearRatio","maxGearRatio","gearRatio"}) number(workshop,incomingWorkshop,key,0.01,100);
        if (incomingWorkshop.value("cvt").isBool()) workshop.insert("cvt",incomingWorkshop.value("cvt"));
        for (const auto key : {"persistence","restoring","clutchProtected"})
            if (incomingWorkshop.value(key).isBool()) workshop.insert(key,incomingWorkshop.value(key));
        workshop.insert("restoreStatus",bounded(incomingWorkshop.value("restoreStatus"),64));
        workshop.insert("restoreError",bounded(incomingWorkshop.value("restoreError"),256));
        out.insert("workshop",workshop);
    }
    number(out, in, "fps", 0, 2000);
    number(out, in, "vehicleCount", 0, 10000);
    number(out, in, "vehicleId", 0, 10000000);
    number(out, in, "sampleHz", 0, 240);
    number(out, in, "cpuMs", 0, 10000);
    number(out, in, "gpuMs", 0, 10000);
    out.insert("controlsAvailable", in.value("controlsAvailable").toBool());
    const auto v = in.value("telemetry").toObject();
    QJsonObject t;
    number(t, v, "speed", 0, 2000);
    number(t, v, "rpm", 0, 100000);
    number(t, v, "gear", -10, 30);
    number(t, v, "fuel", 0, 1);
    number(t, v, "throttle", 0, 1);
    number(t, v, "brake", 0, 1);
    number(t, v, "coolant", -100, 1000);
    number(t, v, "oil", -100, 1000);
    number(t, v, "redline", 100, 100000);
    number(t, v, "steering", -1, 1);
    number(t, v, "lateralG", -100, 100);
    number(t, v, "longitudinalG", -100, 100);
    number(t, v, "boost", -100, 1000);
    number(t, v, "fuelVolume", 0, 100000);
    number(t, v, "fuelCapacity", 0, 100000);
    number(t, v, "wheelSpeed", 0, 2000);
    for (const auto &key : {"lights", "leftSignal", "rightSignal", "abs", "esc", "engineRunning"})
        if (v.value(key).isBool()) t.insert(key, v.value(key));
    out.insert("telemetry", t);
    if (out.value("multiplayer").toBool()) {
        const auto rawName = in.value("server").toString();
        out.insert("serverRaw", rawName.size() <= 512 ? rawName : QString());
        out.insert("server", cleanName(bounded(in.value("server"), 512)));
        out.insert("serverIp", bounded(in.value("serverIp"), 64));
        number(out, in, "serverPort", 1, 65535);
        number(out, in, "ping", 0, 60000);
        const auto list = in.value("players").toArray();
        QJsonArray players;
        for (int i = 0; i < qMin(128, int(list.size())); ++i) {
            if (!list[i].isObject()) continue;
            const auto p = list[i].toObject();
            const auto raw = bounded(p.value("rawName").toString().isEmpty() ? p.value("name").toString() : p.value("rawName").toString(), 64);
            const bool identified = Protocol::playerId(p.value("sessionPlayerId"));
            QJsonObject player{{"name", cleanName(raw)}, {"rawName", raw}, {"local", p.value("local").toBool()},
                               {"guest", p.value("guest").toBool() || !identified}, {"role", bounded(p.value("role"), 32)}};
            if (identified) player.insert("sessionPlayerId", p.value("sessionPlayerId"));
            number(player, p, "ping", 0, 60000);
            number(player, p, "pingAgeMs", 0, 3600000);
            number(player, p, "vehicles", 0, 1000);
            const auto source = bounded(p.value("pingSource"), 16);
            player.insert("pingSource", QStringList{"local", "remote", "server", "unavailable"}.contains(source) ? source : QString("unavailable"));
            // Stable per-session key. Never derived from a ping or a permanently
            // persisted account identifier.
            QString key;
            if (player.value("sessionPlayerId").isDouble()) key = "id:" + QString::number(qint64(player.value("sessionPlayerId").toDouble()));
            else if (!raw.isEmpty()) key = "name:" + QString::fromLatin1(QCryptographicHash::hash(raw.toUtf8(), QCryptographicHash::Sha256).toHex().left(24));
            if (!Protocol::rowKey(key)) continue;
            player.insert("rowKey", key);
            players.append(player);
        }
        out.insert("players", players);
        number(out, in, "playerCount", 0, 10000);
        number(out, in, "mpVehicleCount", 0, 10000);
    }
    QJsonArray activeVehicles;
    const auto active = in.value("activeVehicles").toArray();
    for (int i = 0; i < qMin(128, int(active.size())); ++i) {
        const auto veh = active[i].toObject();
        if (!veh.value("id").isDouble()) continue;
        QJsonObject item{{"name", cleanName(bounded(veh.value("name")))},
                         {"model", bounded(veh.value("model"), 64)},
                         {"owned", veh.value("owned").toBool()}};
        number(item, veh, "id", 0, 10000000);
        activeVehicles.append(item);
    }
    out.insert("activeVehicles", activeVehicles);
    return out;
}

Telemetry::Telemetry(const QString &path, QObject *parent) : QObject(parent), m_path(path) {
    connect(&m_policy, &ServerPolicy::changed, this, [this] { m_policySent.invalidate(); updatePolicy(); broadcast(state()); });
    connect(&m_socket, &QUdpSocket::readyRead, this, &Telemetry::drain);
    m_timer.setInterval(250);
    connect(&m_timer, &QTimer::timeout, this, &Telemetry::publish);
    m_server.setSocketOptions(QLocalServer::UserAccessOption);
    connect(&m_server, &QLocalServer::newConnection, this, [this] {
        while (m_server.hasPendingConnections()) {
            auto *client = m_server.nextPendingConnection();
            if (m_clients.size() >= 8) { client->disconnectFromServer(); client->deleteLater(); continue; }
            m_clients.insert(client);
            client->write(QJsonDocument(state()).toJson(QJsonDocument::Compact) + '\n');
            if (!m_catalog.isEmpty() && m_lastPacket.isValid() && m_lastPacket.elapsed() < 3000) client->write(QJsonDocument(QJsonObject{{"kind", "catalog"}, {"sessionId",m_peerSession}, {"items", m_catalog}, {"complete", true}}).toJson(QJsonDocument::Compact) + '\n');
            connect(client, &QLocalSocket::readyRead, this, [this, client] {
                auto &buffer = m_buffers[client];
                buffer += client->readAll();
                if (buffer.size() > 32768) { client->abort(); return; }
                while (buffer.contains('\n')) {
                    const int index = buffer.indexOf('\n');
                    const auto line = buffer.left(index); buffer.remove(0, index + 1);
                    command(client, QJsonDocument::fromJson(line).object());
                }
            });
            connect(client, &QLocalSocket::disconnected, this, [this, client] {
                for (auto it = m_requests.begin(); it != m_requests.end();) {
                    if (it->client == client) {
                        if (m_catalogRequest == it.key()) m_catalogRequest.clear();
                        it = m_requests.erase(it);
                    } else ++it;
                }
                m_clients.remove(client); m_buffers.remove(client); client->deleteLater();
            });
        }
    });
}
bool Telemetry::start(quint16 port) {
    if (!m_socket.bind(QHostAddress::LocalHost, port, QUdpSocket::DontShareAddress)) { m_error = m_socket.errorString(); return false; }
    const auto directory = QFileInfo(m_path).absolutePath();
    if (!QDir().mkpath(directory)) { m_error = tr("Cannot create runtime directory."); return false; }
    QFile::setPermissions(directory, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    QLocalServer::removeServer(socketPath(m_path));
    if (!m_server.listen(socketPath(m_path))) { m_error = m_server.errorString(); return false; }
    m_timer.start();
    publish();
    return true;
}
Telemetry::~Telemetry() {
    // Child sockets otherwise emit disconnected after our containers are gone.
    QObject::disconnect(&m_server, nullptr, this, nullptr);
    for (auto *client : std::as_const(m_clients)) QObject::disconnect(client, nullptr, this, nullptr);
}
void Telemetry::drain() {
    // Bound work per event-loop turn even if a local process floods the port.
    for (int i = 0; i < 64 && m_socket.hasPendingDatagrams(); ++i) {
        const auto packet = m_socket.receiveDatagram(32769);
        if (!packet.senderAddress().isLoopback() || packet.data().size() > 32768) continue;
        const auto envelope = QJsonDocument::fromJson(packet.data()).object();
        const auto kind = envelope.value("kind").toString();
        if (envelope.value("source") == "beamdeck" && envelope.value("schema").toInt() == 1
            && (kind == "result" || kind == "catalog" || kind == "configs")) {
            const auto id = envelope.value("requestId").toString();
            auto request = m_requests.find(id);
            if (packet.senderPort() != m_peerPort || envelope.value("sessionId").toString() != m_peerSession
                || !m_lastPacket.isValid() || m_lastPacket.elapsed() >= 3000 || request == m_requests.end()) continue;
            if (kind == "result") {
                const auto client = request->client;
                m_requests.erase(request);
                if (m_catalogRequest == id) m_catalogRequest.clear();
                if (client) client->write(QJsonDocument(QJsonObject{{"kind", "result"}, {"requestId", id},
                    {"ok", envelope.value("ok").toBool()}, {"message", bounded(envelope.value("message"), 256)}}).toJson(QJsonDocument::Compact) + '\n');
                continue;
            }
            if (request->action != kind || envelope.value("chunk").toInt(-1) != request->chunk
                || (kind == "configs" && envelope.value("model").toString() != request->model)) continue;
            ++request->chunk;
            for (const auto &entry : envelope.value("items").toArray()) {
                if (request->items.size() >= 2048) break;
                const auto item = entry.toObject();
                const auto model = kind == "catalog" ? item.value("id").toString() : request->model;
                if (!Protocol::model(model)) continue;
                QJsonObject clean{{"id", model}, {"name", bounded(item.value("name"))},
                    {"brand", bounded(item.value("brand"), 80)}, {"type", bounded(item.value("type"), 80)},
                    {"preview", Protocol::preview(item.value("preview"), model)}};
                QJsonObject facets;
                const auto incomingFacets = item.value("facets").toObject();
                for (const auto field : {"type", "brand", "bodyStyle", "country", "source", "drivetrain", "transmission", "configType", "fuelType", "propulsion"}) {
                    QJsonArray values;
                    for (const auto &value : incomingFacets.value(field).toArray()) {
                        const auto text = bounded(value, 80);
                        if (!text.isEmpty() && !values.contains(text)) values.append(text);
                        if (values.size() >= 32) break;
                    }
                    facets.insert(field, values);
                }
                clean.insert("facets", facets);
                if (kind == "configs") {
                    const auto key = item.value("key").toString();
                    if (!Protocol::config(key)) continue;
                    clean.insert("id", model + '/' + key); clean.insert("key", key); clean.insert("model", model);
                    clean.insert("custom", item.value("custom").toBool());
                    for (const auto field : {"source", "description", "drivetrain", "transmission"}) clean.insert(field, bounded(item.value(field), 256));
                }
                request->items.append(clean);
            }
            if (envelope.value("complete").toBool()) {
                QJsonObject result{{"kind", kind}, {"requestId", id}, {"sessionId", m_peerSession},
                    {"items", request->items}, {"model", request->model}, {"complete", true},
                    {"truncated", envelope.value("truncated").toBool()}};
                const auto client = request->client;
                const auto items = request->items;
                m_requests.erase(request);
                if (kind == "catalog") { m_catalog = items; m_catalogRequest.clear(); broadcast(result); }
                else if (client) client->write(QJsonDocument(result).toJson(QJsonDocument::Compact) + '\n');
            }
            continue;
        }
        if (!envelope.value("kind").isUndefined()) continue;
        auto data = sanitize(packet.data());
        if (data.isEmpty()) continue;
        if (!m_gameWasSeen) m_gameWasSeen = true;
        if (m_lastPacket.isValid() && m_lastPacket.elapsed() < 3000 && m_peerPort && packet.senderPort() != m_peerPort) continue;
        const bool reset = !m_lastPacket.isValid() || m_lastPacket.elapsed() > 3000
            || data.value("sessionId") != m_data.value("sessionId");
        if (reset) {
            for (auto it = m_requests.cbegin(); it != m_requests.cend(); ++it)
                if (it->client) it->client->write(QJsonDocument(QJsonObject{{"kind", "result"}, {"requestId", it.key()},
                    {"ok", false}, {"message", tr("Game session changed. Please try again.")}}).toJson(QJsonDocument::Compact) + '\n');
            m_requests.clear();
            resetTrip();
            m_catalog = {};
            m_catalogRequest.clear();
        }
        m_peerPort = packet.senderPort();
        m_peerSession = data.value("sessionId").toString();
        const auto t = data.value("telemetry").toObject();
        const double speed = t.value("speed").toDouble();
        const bool movingSample = t.contains("speed") && !data.value("paused").toBool();
        const double dt = m_sample.isValid() ? m_sample.elapsed() / 1000.0 : 0;
        if (!reset && movingSample && m_previousSpeedValid && dt > 0 && dt < 1) {
            m_distance += (speed + m_previousSpeed) * .5 * dt;
            m_acceleration = (speed - m_previousSpeed) / dt;
            if (std::abs(m_acceleration) > 60) { m_sprintArmed = false; m_sprintStart = -1; }
        }
        if (movingSample && speed < .5) { m_sprintArmed = true; m_sprintStart = -1; }
        if (movingSample && m_sprintArmed && speed >= .5 && m_sprintStart < 0) m_sprintStart = m_session.elapsed() / 1000.0;
        if (movingSample && m_sprintArmed && speed >= 100.0 / 3.6 && m_sprintStart >= 0) {
            const double seconds = m_session.elapsed() / 1000.0 - m_sprintStart;
            if (seconds > .3 && (!m_bestSprint || seconds < m_bestSprint)) m_bestSprint = seconds;
            m_sprintArmed = false;
        }
        if (!movingSample) { m_sprintStart = -1; m_sprintArmed = false; m_acceleration = 0; }
        if (data.contains("fps")) { m_fpsSum += data.value("fps").toDouble(); ++m_fpsSamples; m_fpsMin = qMin(m_fpsMin, data.value("fps").toDouble()); }
        m_previousSpeedValid = movingSample;
        m_previousSpeed = speed;
        m_topSpeed = qMax(m_topSpeed, speed);
        m_sample.start();
        m_data = data;
        m_lastPacket.start();
        updatePolicy();
    }
    broadcast(state());
    if (m_socket.hasPendingDatagrams()) QTimer::singleShot(0, this, &Telemetry::drain);
}
void Telemetry::resetTrip() {
    m_session.start(); m_distance = m_topSpeed = m_bestSprint = m_acceleration = 0;
    m_fpsSum = 0; m_fpsMin = 2000; m_fpsSamples = 0;
    m_previousSpeedValid = m_sprintArmed = false; m_sprintStart = -1;
}
QJsonObject Telemetry::state() const {
    const bool connected = m_lastPacket.isValid() && m_lastPacket.elapsed() < 3000;
    QJsonObject state = connected ? m_data : QJsonObject();
    state.insert("connected", connected);
    state.insert("kind", "state");
    state.insert("gameSeen", m_gameWasSeen);
    state.insert("updatedAt", QDateTime::currentMSecsSinceEpoch());
    if (connected) {
        const auto guard = policy();
        state.insert("sandboxAllowed", guard.value("allowed").toBool()
            && (!m_data.value("multiplayer").toBool() || m_data.value("sandboxAllowed").toBool()));
        state.insert("policyReason", m_data.value("policyCleanup").toBool()
            ? tr("Clearing previous ABM changes. Gameplay stays locked until cleanup succeeds.") : guard.value("reason").toString());
        state.insert("sessionSeconds", m_session.elapsed() / 1000);
        state.insert("distance", m_distance);
        state.insert("topSpeed", m_topSpeed);
        state.insert("averageSpeed", m_session.elapsed() > 0 ? m_distance * 1000.0 / m_session.elapsed() : 0);
        state.insert("acceleration", m_acceleration);
        if (m_bestSprint > 0) state.insert("bestZeroHundred", m_bestSprint);
        if (m_fpsSamples) { state.insert("fpsAverage", m_fpsSum / m_fpsSamples); state.insert("fpsMinimum", m_fpsMin); }
        state.insert("packetAgeMs", m_lastPacket.elapsed());
    }
    return state;
}
QJsonObject Telemetry::policy() const {
    if (m_data.value("joining").toBool()) return {{"allowed", false}, {"reason", tr("Joining BeamMP: gameplay changes are locked.")}};
    if (!m_data.value("multiplayer").toBool()) return {{"allowed", true}, {"reason", tr("Single player: no server restriction.")}};
    if (!Protocol::workshopBridge(m_data.value("bridgeVersion").toString()) && m_data.value("bridgeVersion").toString() != "0.7.1")
        return {{"allowed", false}, {"reason", tr("Multiplayer changes need bridge 0.7.1. Install it and restart BeamNG.")}};
    return m_policy.decision(m_data);
}
void Telemetry::updatePolicy() {
    if (!m_lastPacket.isValid() || m_lastPacket.elapsed() >= 3000 || (!Protocol::workshopBridge(m_data.value("bridgeVersion").toString()) && m_data.value("bridgeVersion").toString() != "0.7.1")
        || !m_data.value("multiplayer").toBool()) return;
    m_policy.refresh();
    if (m_policySent.isValid() && m_policySent.elapsed() < 1000) return;
    m_policySent.start();
    auto record = policy();
    record.insert("source", "beamdeck-control"); record.insert("schema", 1);
    record.insert("sessionId", m_peerSession); record.insert("action", "serverPolicy");
    record.insert("serverIp", m_data.value("serverIp")); record.insert("serverPort", m_data.value("serverPort"));
    record.insert("serverName", m_data.value("serverRaw")); record.insert("ttl", 3);
    m_socket.writeDatagram(QJsonDocument(record).toJson(QJsonDocument::Compact), QHostAddress::LocalHost, m_peerPort);
}
void Telemetry::broadcast(const QJsonObject &message) {
    const auto bytes = QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n';
    const auto clients = m_clients;
    for (auto *client : clients) {
        if (client->bytesToWrite() > 262144) { client->abort(); continue; }
        client->write(bytes);
    }
}
void Telemetry::command(QLocalSocket *client, const QJsonObject &message) {
    const auto action = message.value("action").toString();
    const auto id = message.value("requestId").toString();
    auto reply = [&](bool ok, const QString &text) {
        client->write(QJsonDocument(QJsonObject{{"kind", "result"}, {"requestId", id}, {"ok", ok}, {"message", text}}).toJson(QJsonDocument::Compact) + '\n');
    };
    if (id.size() > 80 || id.isEmpty()) return;
    if (action == "resetTrip") { resetTrip(); broadcast(state()); reply(true, tr("Trip reset.")); return; }
    if (!m_lastPacket.isValid() || m_lastPacket.elapsed() >= 3000) {
        reply(false, tr("Game is not running; start BeamNG first.")); return;
    }
    if (!QStringList{"catalog", "configs", "enableBeamMP", "spawn", "replace", "switch", "tuning", "repair", "ignite", "extinguish", "deflate", "handling", "gears", "stability", "dealer", "dealerStop", "engine", "dimensions", "power", "fullReset","rpm","clutch","persistence"}.contains(action) || m_requests.size() >= 16 || m_requests.contains(id)) {
        reply(false, tr("Unsupported action or too many pending requests.")); return;
    }
    const bool driveAction = QStringList{"spawn", "replace", "switch"}.contains(action);
    const auto bridgeVersion = m_data.value("bridgeVersion").toString();
    const bool workshopBridge = Protocol::workshopBridge(bridgeVersion);
    if (QStringList{"rpm","clutch","persistence"}.contains(action) && bridgeVersion!="0.10.0" && bridgeVersion!="0.11.0") { reply(false,tr("RPM, clutch protection and reset persistence need bridge 0.10.0. Restart BeamNG after installing it.")); return; }
    if ((action == "power" || action == "fullReset") && !workshopBridge) { reply(false,tr("Live power and full reset need bridge 0.9.0. Install it and restart BeamNG.")); return; }
    const bool mutates = !QStringList{"catalog", "configs", "enableBeamMP", "dealerStop"}.contains(action);
    if (mutates && (!policy().value("allowed").toBool() || m_data.value("policyCleanup").toBool()
        || (m_data.value("multiplayer").toBool() && !m_data.value("sandboxAllowed").toBool()))) {
        reply(false, state().value("policyReason").toString()); return;
    }
    if (action == "gears" && !workshopBridge && bridgeVersion != "0.6.2" && bridgeVersion != "0.7.0" && bridgeVersion != "0.7.1") {
        reply(false, tr("Safe extra gears need bridge 0.6.2. Install it and restart BeamNG when ready.")); return;
    }
    if ((action == "dimensions" || action == "dealerStop") && !workshopBridge && bridgeVersion != "0.6.1" && bridgeVersion != "0.6.2" && bridgeVersion != "0.7.0" && bridgeVersion != "0.7.1") {
        reply(false, tr("Resizing and bounce stop need bridge 0.6.1. Install it and restart BeamNG when ready.")); return;
    }
    if (action == "engine" && !workshopBridge && bridgeVersion != "0.6.0" && bridgeVersion != "0.6.1" && bridgeVersion != "0.6.2" && bridgeVersion != "0.7.0" && bridgeVersion != "0.7.1") {
        reply(false, tr("Engine durability needs bridge 0.6. Install it and restart BeamNG when ready.")); return;
    }
    if (QStringList{"handling", "stability", "dealer"}.contains(action) && !workshopBridge && bridgeVersion != "0.5.0" && bridgeVersion != "0.6.0" && bridgeVersion != "0.6.1" && bridgeVersion != "0.6.2" && bridgeVersion != "0.7.0" && bridgeVersion != "0.7.1") {
        reply(false, tr("Install the bridge and restart BeamNG for the 0.5 sandbox controls.")); return;
    }
    if (QStringList{"tuning", "repair", "ignite", "extinguish", "deflate"}.contains(action) && !workshopBridge && bridgeVersion != "0.4.0" && bridgeVersion != "0.5.0" && bridgeVersion != "0.6.0" && bridgeVersion != "0.6.1" && bridgeVersion != "0.6.2" && bridgeVersion != "0.7.0" && bridgeVersion != "0.7.1") {
        reply(false, tr("Restart BeamNG to load the updated Car controls.")); return;
    }
    if (!m_data.value("controlsAvailable").toBool() || !Protocol::controlsBridge(bridgeVersion)) {
        reply(false, tr("Reload the ABM game extension to enable car controls.")); return;
    }
    if (action == "catalog" && !m_catalogRequest.isEmpty()) { reply(false, tr("A catalog request is already running.")); return; }
    QJsonObject command{{"source", "beamdeck-control"}, {"schema", 1}, {"sessionId", m_peerSession}, {"action", action}, {"requestId", id}};
    const bool boundAction = QStringList{"power","fullReset","tuning","handling","gears","engine","dimensions","stability","rpm","clutch","persistence"}.contains(action);
    if (workshopBridge && boundAction) {
        const auto generation=m_data.value("workshop").toObject().value("generation").toString();
        const auto expiry=message.value("expiresAt").toDouble();
        const auto revision=message.value("revision").toDouble();
        const auto now=QDateTime::currentSecsSinceEpoch();
        if (message.value("contract").toInt()!=1 || generation.isEmpty() || message.value("generation").toString()!=generation
            || message.value("targetSession").toString()!=m_peerSession || message.value("targetVehicle")!=m_data.value("vehicleId")
            || expiry<=now || expiry>now+5 || !std::isfinite(expiry) || revision<1 || revision>2147483647 || revision!=std::floor(revision)
            || message.value("client").toString().isEmpty() || message.value("client").toString().size()>80) {
            reply(false,tr("Workshop target changed or request expired.")); return;
        }
        for (const auto key : {"contract","generation","targetVehicle","expiresAt","revision","client"}) command.insert(key,message.value(key));
    }
    if (action == "power") {
        if (!Protocol::enginePower(message.value("enginePower"))) { reply(false,tr("Enter a finite positive power multiplier.")); return; }
        command.insert("enginePower",message.value("enginePower"));
    }
    if (mutates && action!="fullReset" && action!="persistence" && m_data.value("workshop").toObject().value("restoring").toBool()) { reply(false,tr("Reset restoration is in progress; wait or use Full reset.")); return; }
    if (action=="rpm") {
        if (!Protocol::rpm(message.value("maxRPM"))) { reply(false,tr("Invalid RPM limit.")); return; }
        command.insert("maxRPM",message.value("maxRPM"));
    }
    if (action=="clutch" || action=="persistence") {
        const QString key=action=="clutch"?"clutchIndestructible":"enabled";
        if (!message.value(key).isBool()) { reply(false,tr("Expected an on/off setting.")); return; }
        command.insert(key,message.value(key));
    }
    if (action == "fullReset") {
        const auto speed=m_data.value("telemetry").toObject().value("speed");
        if (!speed.isDouble() || speed.toDouble()>1) { reply(false,tr("Stop before clearing ABM vehicle settings.")); return; }
    }
    if (action == "configs") {
        const auto model = message.value("model").toString();
        if (!Protocol::model(model)) { reply(false, tr("Invalid model identifier.")); return; }
        command.insert("model", model);
    }
    if (action == "spawn" || action == "replace") {
        const auto modelKey = message.value("model").toString();
        if (!Protocol::model(modelKey) || !Protocol::config(message.value("config").toString())) { reply(false, tr("Invalid configuration identifier.")); return; }
        command.insert("model", modelKey);
        command.insert("config", message.value("config"));
    }
    if (action == "switch") {
        const int vehicle = message.value("vehicleId").toInt(-1);
        if (vehicle < 0 || vehicle > 10000000) { reply(false, tr("Invalid vehicle identifier.")); return; }
        command.insert("vehicleId", vehicle);
    }
    if (action == "tuning") {
        const auto power = message.value("enginePower");
        if (!power.isUndefined() && !power.isNull() && !Protocol::enginePower(power)) {
            reply(false, tr("Enter a finite positive power multiplier.")); return;
        }
        if (bridgeVersion == "0.4.0" && power.isDouble() && (power.toDouble() < .5 || power.toDouble() > 3)) {
            reply(false, tr("Power beyond 50–300% needs bridge 0.5. Install it and restart BeamNG.")); return;
        }
        const auto gravity = message.value("gravity");
        if (!gravity.isUndefined() && !gravity.isNull() && !Protocol::gravity(gravity)) {
            reply(false, tr("Gravity must be between -30 and 0 m/s².")); return;
        }
        const auto immunity = message.value("damageImmunity");
        if (!immunity.isUndefined() && !immunity.isNull() && !immunity.isBool()) {
            reply(false, tr("Damage immunity must be on or off.")); return;
        }
        if (!power.isUndefined() && !power.isNull()) command.insert("enginePower", power.toDouble());
        if (!gravity.isUndefined() && !gravity.isNull()) command.insert("gravity", gravity.toDouble());
        if (!immunity.isUndefined() && !immunity.isNull()) command.insert("damageImmunity", immunity.toBool());
    }
    if (action == "handling") {
        if (!Protocol::handling(message)) { reply(false, tr("Invalid handling values.")); return; }
        for (const auto key : {"tireGrip", "slidingGrip", "brakePower", "steeringResponse"}) command.insert(key, message.value(key));
    }
    if (action == "engine") {
        if (!Protocol::engine(message)) { reply(false, tr("Invalid engine durability settings.")); return; }
        command.insert("engineStrength", message.value("engineStrength"));
        command.insert("engineIndestructible", message.value("engineIndestructible"));
    }
    if (action == "gears") {
        if (!Protocol::gears(message.value("ratios"))) { reply(false, tr("Invalid ratios or command size exceeded.")); return; }
        command.insert("ratios", message.value("ratios"));
    }
    if (action == "dimensions") {
        if (!Protocol::dimensions(message)) { reply(false, tr("Dimensions must be 0.5–2× per axis.")); return; }
        if (m_data.value("multiplayer").toBool() && !workshopBridge && bridgeVersion != "0.6.2" && bridgeVersion != "0.7.0" && bridgeVersion != "0.7.1") { reply(false, tr("Multiplayer resizing needs bridge 0.6.2.")); return; }
        const auto speed = m_data.value("telemetry").toObject().value("speed");
        if (!speed.isDouble() || speed.toDouble() > 1) { reply(false, tr("Stop the vehicle before resizing.")); return; }
        for (const auto key : {"width", "length", "height"}) command.insert(key, message.value(key));
    }
    if (driveAction || action == "gears") {
        const auto t = m_data.value("telemetry").toObject();
        if (t.contains("speed") && t.value("speed").toDouble() > 1) { reply(false, tr("Stop before changing vehicles.")); return; }
    }
    m_requests.insert(id, Request{client, action, message.value("model").toString(), {}, 0});
    if (action == "catalog") m_catalogRequest = id;
    if (m_socket.writeDatagram(QJsonDocument(command).toJson(QJsonDocument::Compact), QHostAddress::LocalHost, m_peerPort) < 0) {
        m_requests.remove(id); if (m_catalogRequest == id) m_catalogRequest.clear();
        reply(false, tr("Could not send the game request.")); return;
    }
    QTimer::singleShot(15000, this, [this, id] {
        if (m_catalogRequest == id) m_catalogRequest.clear();
        auto client = m_requests.take(id).client;
        if (client) client->write(QJsonDocument(QJsonObject{{"kind", "result"}, {"requestId", id}, {"ok", false}, {"message", tr("Game request timed out. Reload the extension and retry.")}}).toJson(QJsonDocument::Compact) + '\n');
    });
}
void Telemetry::publish() {
    updatePolicy();
    const auto snapshot = state();
    broadcast(snapshot);
    const QString directory = QFileInfo(m_path).absolutePath();
    QDir().mkpath(directory);
    QFile::setPermissions(directory, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly)) { qWarning("Cannot write telemetry snapshot"); return; }
    file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
    file.write(QJsonDocument(snapshot).toJson(QJsonDocument::Compact));
    if (!file.commit()) qWarning("Cannot publish telemetry snapshot");
}
