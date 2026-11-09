#include "backend.h"
#include "telemetry.h"
#include "protocol.h"
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUuid>
#include <cmath>

Backend::Backend(const QString &path, QObject *parent) : QObject(parent), m_path(path.isEmpty() ? Telemetry::defaultPath() : path) {
    const auto directory = path.isEmpty() ? qEnvironmentVariable("BEAMSIDE_CONFIG_DIR", QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)) : QFileInfo(path).absolutePath();
    m_settings = std::make_unique<QSettings>(directory + "/preferences.ini", QSettings::IniFormat);
    m_integration = std::make_unique<Integration>(directory + "/integration.ini", path.isEmpty());
    m_metric = m_settings->value("metric", true).toBool();
    m_favorites = m_settings->value("favorites").toStringList().mid(0, 256);
    // Cheat-menu sliders keep their last applied values between launches.
    if (m_settings->contains("tuning/power"))
        m_tuning = {{"enginePower", m_settings->value("tuning/power").toDouble()},
                    {"damageImmunity", m_settings->value("tuning/damageImmunity", false).toBool()},
                    {"gravity", m_settings->value("tuning/gravity", -9.81).toDouble()}};
    connect(&m_socket, &QLocalSocket::readyRead, this, &Backend::readStream);
    connect(&m_socket, &QLocalSocket::connected, this, [this] { m_buffer.clear(); });
    connect(&m_socket, &QLocalSocket::disconnected, this, [this] {
        m_pending.clear(); m_pendingAction.clear(); m_buffer.clear(); emit actionChanged();
    });
    m_timer.setInterval(250);
    connect(&m_timer, &QTimer::timeout, this, &Backend::refresh);
    m_timer.start();
    refresh();
}
Backend::~Backend() {
    // Socket teardown can emit disconnected after later-declared state is gone.
    QObject::disconnect(&m_socket, nullptr, this, nullptr);
}
void Backend::reconnect() {
    if (m_demo || m_socket.state() != QLocalSocket::UnconnectedState) return;
    m_socket.connectToServer(Telemetry::socketPath(m_path));
}
void Backend::accept(const QJsonObject &data) {
    if (!m_timer.isActive() && !m_demo) return;
    const auto age = QDateTime::currentMSecsSinceEpoch() - qint64(data.value("updatedAt").toDouble());
    if (age < -1000 || age > 3000 || !data.value("connected").isBool()) return;
    const auto previousSession = m_state.value("sessionId").toString();
    if (!previousSession.isEmpty() && previousSession != data.value("sessionId").toString()) {
        m_catalog.clear(); m_configs.clear(); m_history.clear();
        m_configsModel.clear(); m_pending.clear(); m_pendingAction.clear(); emit actionChanged();
        emit catalogChanged(); emit configsChanged(); emit historyChanged();
    }
    m_error.clear(); m_state = data.toVariantMap();
    if (m_integration->gameFolder().isEmpty() && QFileInfo::exists(data.value("gameFolder").toString() + "/content/vehicles"))
        m_integration->setGameFolder(data.value("gameFolder").toString());
    if (m_integration->userFolder().isEmpty() && QFileInfo::exists(data.value("userFolder").toString() + "/beamng.log"))
        m_integration->setUserFolder(data.value("userFolder").toString());
    const bool gameSeen = data.value("gameSeen").toBool();
    if (gameSeen != m_gameSeen) { m_gameSeen = gameSeen; emit gameSeenChanged(); }
    const auto players = data.value("players").toArray().toVariantList();
    if (players != m_players) { m_players = players; emit playersChanged(); }
    const auto vehicles = data.value("activeVehicles").toArray().toVariantList();
    if (vehicles != m_activeVehicles) { m_activeVehicles = vehicles; emit activeVehiclesChanged(); }
    if (data.value("connected").toBool() && (!m_historyAge.isValid() || m_historyAge.elapsed() >= 100)) {
        const auto t = data.value("telemetry").toObject();
        QVariantMap sample{{"timestamp", QDateTime::currentMSecsSinceEpoch()}};
        for (const auto key : {"speed", "rpm"}) if (t.value(key).isDouble()) sample.insert(key,t.value(key).toDouble());
        for (const auto key : {"fps", "ping"}) if (data.value(key).isDouble()) sample.insert(key,data.value(key).toDouble());
        m_history.append(sample);
        while (m_history.size() > 300) m_history.removeFirst();
        m_historyAge.start(); emit historyChanged();
    }
    emit changed();
}
void Backend::readStream() {
    if (m_demo) { m_socket.readAll(); return; }
    m_buffer += m_socket.readAll();
    if (m_buffer.size() > 16777216) { m_buffer.clear(); m_socket.abort(); return; }
    while (m_buffer.contains('\n')) {
        const int index = m_buffer.indexOf('\n');
        const auto line = m_buffer.left(index); m_buffer.remove(0, index + 1);
        const auto data = QJsonDocument::fromJson(line).object();
        if (data.value("kind") == "state") { m_streamAge.start(); accept(data); }
        else if (data.value("kind") == "catalog") {
            m_catalog = decorate(data.value("items").toArray());
            if (m_catalog.size() > 2048) m_catalog = m_catalog.mid(0, 2048);
            if (m_pendingAction == "catalog") { m_pending.clear(); m_pendingAction.clear(); }
            m_actionMessage = data.value("truncated").toBool() ? tr("Catalog limit reached; some items are not shown.") : tr("Vehicle catalog loaded.");
            emit catalogChanged(); emit actionChanged();
        } else if (data.value("kind") == "configs" && data.value("requestId").toString() == m_pending && m_pendingAction == "configs"
                   && data.value("model").toString() == m_requestedModel) {
            m_pending.clear(); m_pendingAction.clear();
            m_configsModel = m_requestedModel;
            m_configs = decorate(data.value("items").toArray(), m_configsModel);
            m_actionMessage = data.value("truncated").toBool() ? tr("Configuration limit reached; some items are not shown.") : tr("Configurations loaded.");
            emit configsChanged(); emit actionChanged();
        } else if (data.value("kind") == "result" && data.value("requestId").toString() == m_pending) {
            const auto action = m_pendingAction;
            m_pending.clear(); m_pendingAction.clear(); m_actionMessage = data.value("message").toString().left(256); emit actionChanged();
            if (action == "enableBeamMP") {
                if (data.value("ok").toBool()) m_integration->startBeamMP();
                else m_integration->setMessage(m_actionMessage);
            }
            if (action == "tuning" && data.value("ok").toBool()) {
                m_tuning = QJsonObject{m_nextTuning}.toVariantMap(); m_nextTuning = {};
                storeTuning();
            }
        }
    }
}
QVariantList Backend::decorate(const QJsonArray &items, const QString &model) {
    QVariantList out;
    const QStringList roots{m_integration->gameFolder(), m_integration->userFolder()};
    QString fallback;
    for (const auto &entry : std::as_const(m_catalog)) if (entry.toMap().value("id").toString() == model) fallback = entry.toMap().value("thumbnail").toString();
    for (const auto &value : items) {
        auto item = value.toObject().toVariantMap();
        item.insert("thumbnail", m_previews ? m_previews->registerImage(value.toObject().value("preview").toObject(), roots) : QString());
        item.insert("modelThumbnail", fallback); out.append(item);
        if (out.size() >= 2048) break;
    }
    return out;
}
void Backend::refresh() {
    if (!m_demo) reconnect();
    if (m_socket.state() == QLocalSocket::ConnectedState && m_streamAge.isValid() && m_streamAge.elapsed() < 1000) return;
    QFile file(m_path);
    m_state.clear();
    m_error.clear();
    if (!file.open(QIODevice::ReadOnly)) {
        m_error = tr("Receiver unavailable. Run: systemctl --user restart abm.service");
    } else if (file.size() > 65536) {
        m_error = tr("Invalid telemetry snapshot.");
    } else {
        const auto document = QJsonDocument::fromJson(file.readAll());
        const auto data = document.object();
        const auto age = QDateTime::currentMSecsSinceEpoch() - qint64(data.value("updatedAt").toDouble());
        if (!document.isObject() || !data.value("connected").isBool())
            m_error = tr("Invalid telemetry snapshot.");
        else if (age < -1000 || age > 3000)
            m_error = tr("Receiver stopped updating. Restart abm.service.");
        else { accept(data); return; }
    }
    if (!connected()) {
        if (!m_players.isEmpty()) { m_players.clear(); emit playersChanged(); }
        if (!m_activeVehicles.isEmpty()) { m_activeVehicles.clear(); emit activeVehiclesChanged(); }
        if (!m_configs.isEmpty()) { m_configs.clear(); emit configsChanged(); }
    }
    emit changed();
}
void Backend::toggleMonitoring() {
    if (m_timer.isActive()) m_timer.stop();
    else { m_timer.start(); refresh(); }
    emit changed();
}
void Backend::setMetric(bool metric) {
    if (m_metric == metric) return;
    m_metric = metric; m_settings->setValue("metric", metric); m_settings->sync(); emit preferencesChanged();
}
void Backend::storeTuning() {
    m_settings->setValue("tuning/power", m_tuning.value("enginePower").toDouble());
    m_settings->setValue("tuning/gravity", m_tuning.value("gravity").toDouble());
    m_settings->setValue("tuning/damageImmunity", m_tuning.value("damageImmunity").toBool());
    m_settings->sync(); emit tuningChanged();
}
void Backend::applyTuning(QVariantMap tuning) {
    if (busy()) return;
    QJsonObject parameters;
    if (!Protocol::enginePower(QJsonValue::fromVariant(tuning.value("enginePower"))) ||
        !Protocol::gravity(QJsonValue::fromVariant(tuning.value("gravity"))) ||
        tuning.value("damageImmunity").metaType().id() != QMetaType::Bool) {
        m_actionMessage = tr("Invalid tuning values."); emit actionChanged(); return;
    }
    const auto power = tuning.value("enginePower").toDouble();
    if (!Protocol::enginePower(power)) { m_actionMessage = tr("Enter a finite positive power multiplier."); emit actionChanged(); return; }
    parameters.insert("enginePower", power);
    const auto gravity = tuning.value("gravity").toDouble();
    if (!Protocol::gravity(gravity)) { m_actionMessage = tr("Gravity must be between -30 and 0 m/s²."); emit actionChanged(); return; }
    parameters.insert("gravity", gravity);
    parameters.insert("damageImmunity", tuning.value("damageImmunity").toBool());
    m_nextTuning = parameters; // remember the request so the reply can confirm it
    send("tuning", parameters);
}
void Backend::carAction(const QString &name) {
    if (!QStringList{"repair", "ignite", "extinguish", "deflate", "dealer", "dealerStop", "stability"}.contains(name)) return;
    send(name);
}
void Backend::applyHandling(QVariantMap settings) {
    const auto data = QJsonObject::fromVariantMap(settings);
    if (!Protocol::handling(data)) { m_actionMessage = tr("Invalid handling values."); emit actionChanged(); return; }
    send("handling", data);
}
void Backend::applyEngine(QVariantMap settings) {
    const auto data = QJsonObject::fromVariantMap(settings);
    if (!Protocol::engine(data)) { m_actionMessage = tr("Engine strength must be 0.1–100×, with protection on or off."); emit actionChanged(); return; }
    send("engine", data);
}
void Backend::applyDimensions(QVariantMap settings) {
    const auto data = QJsonObject::fromVariantMap(settings);
    if (!Protocol::dimensions(data)) { m_actionMessage = tr("Width, length and height must each be 0.5–2×."); emit actionChanged(); return; }
    send("dimensions", data);
}
void Backend::applyGears(const QString &ratios) {
    if (ratios.size() > 24000) { m_actionMessage = tr("Gear ratios exceed the command size limit."); emit actionChanged(); return; }
    QJsonArray values;
    for (const auto &part : ratios.trimmed().split(QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts)) {
        bool ok; const auto ratio = part.toDouble(&ok);
        if (!ok) { m_actionMessage = tr("Separate numeric gear ratios with spaces or commas."); emit actionChanged(); return; }
        values.append(ratio);
    }
    if (!Protocol::gears(values)) { m_actionMessage = tr("Enter forward ratios between 0.01 and 100, within 24 KB."); emit actionChanged(); return; }
    send("gears", QJsonObject{{"ratios", values}});
}
void Backend::toggleFavorite(const QString &model) {
    if (!Protocol::identity(model)) return;
    if (m_favorites.contains(model)) m_favorites.removeAll(model);
    else if (m_favorites.size() < 256) m_favorites.append(model);
    m_settings->setValue("favorites", m_favorites); m_settings->sync(); emit preferencesChanged();
}
void Backend::send(const QString &action, const QJsonObject &parameters) {
    if (m_demo) { m_actionMessage = tr("Demo mode: no game actions are sent."); emit actionChanged(); return; }
    if (busy()) return;
    if (m_socket.state() != QLocalSocket::ConnectedState) {
        m_actionMessage = tr("Receiver control connection is unavailable."); emit actionChanged(); reconnect(); return;
    }
    m_pending = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_pendingAction = action;
    auto message = parameters; message.insert("action", action); message.insert("requestId", m_pending);
    m_socket.write(QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n');
    m_actionMessage = tr("Waiting for the game…"); emit actionChanged();
    const auto pending = m_pending;
    QTimer::singleShot(16000, this, [this, pending] {
        if (m_pending == pending) { m_pending.clear(); m_pendingAction.clear(); m_actionMessage = tr("Game request timed out."); emit actionChanged(); }
    });
}
void Backend::requestCatalog() { send("catalog"); }
void Backend::requestConfigs(const QString &model) {
    if (!Protocol::model(model) || busy()) return;
    m_configs.clear(); m_configsModel.clear(); m_requestedModel = model; emit configsChanged();
    if (m_demo) {
        m_configsModel = model;
        m_configs = decorate(QJsonArray{QJsonObject{{"id",model + "/base"},{"model",model},{"key","base"},{"name",tr("Factory configuration")},
            {"preview",QJsonObject{{"path","/vehicles/" + model + "/default.jpg"}}}},
            QJsonObject{{"id",model + "/Weekend build"},{"model",model},{"key","Weekend build"},{"name",tr("Weekend build")},{"custom",true}}}, model);
        emit configsChanged(); return;
    }
    send("configs", QJsonObject{{"model", model}});
}
void Backend::loadExtension() { m_integration->installBridge(); }
void Backend::requestVehicle(const QString &action, const QString &model) {
    if ((action != "spawn" && action != "replace") || !Protocol::identity(model) || !model.contains('/')) return;
    bool found = false;
    for (const auto &config : std::as_const(m_configs)) found |= config.toMap().value("id").toString() == model;
    if (!found) { m_actionMessage = tr("Choose a configuration from the current catalog."); emit actionChanged(); return; }
    send(action, QJsonObject{{"model", model.section('/',0,0)}, {"config",model.section('/',1)}});
}
void Backend::switchVehicle(int id) { if (id >= 0) send("switch", QJsonObject{{"vehicleId", id}}); }
void Backend::resetTrip() { send("resetTrip"); }
bool Backend::startGame() {
    if (m_demo) { m_actionMessage = tr("Demo mode: no game actions are sent."); emit actionChanged(); return false; }
    return m_integration->startGame();
}
bool Backend::launcherAvailable() {
    return QFileInfo(m_integration->launcherPath()).isExecutable();
}
bool Backend::startBeamMP() {
    if (m_demo) { m_actionMessage = tr("Demo mode: no game actions are sent."); emit actionChanged(); return false; }
    if (busy()) return false;
    if (connected() && m_state.value("bridgeVersion").toString() == "0.3.0" && !m_integration->beamMPEnabled()) {
        send("enableBeamMP"); return busy();
    }
    return m_integration->startBeamMP();
}
bool Backend::launchAuto() { m_integration->refreshProcesses(); return m_integration->gameRunning() ? startBeamMP() : startGame(); }
void Backend::setDemo() {
    m_demo = true;
    m_socket.abort();
    m_timer.stop();
    m_state.clear(); m_history.clear(); m_catalog.clear(); m_players.clear(); m_activeVehicles.clear();
    m_pending.clear(); m_pendingAction.clear();
    m_error.clear();
    m_state = QJsonDocument::fromJson(R"({"connected":true,"multiplayer":true,"beammpLoaded":true,"server":"Coastal cruising · EU","map":"west_coast_usa","vehicle":"etk800","fps":89,"vehicleCount":14,"sessionSeconds":1452,"distance":18420,"topSpeed":62.5,"playerCount":3,"mpVehicleCount":5,"ping":32,"telemetry":{"speed":28.6,"rpm":3240,"gear":4,"fuel":0.72,"throttle":0.42,"brake":0,"coolant":88,"oil":96},"players":[{"name":"CoastDriver","local":true,"ping":32,"vehicles":1},{"name":"Apex","ping":48,"vehicles":2},{"name":"Tourer","ping":61,"vehicles":2}]})").object().toVariantMap();
    m_state.insert("vehicleName", "ETK 800 Series"); m_state.insert("bridgeVersion", "0.6.1");
    m_state.insert("controlsAvailable", true); m_state.insert("sampleHz", 20);
    m_state.insert("gameSeen", true); m_state.insert("fpsAverage", 86); m_state.insert("fpsMinimum", 63);
    m_gameSeen = true;
    m_state.insert("cpuMs", 9.4); m_state.insert("gpuMs", 11.2);
    m_state.insert("averageSpeed", 19.2); m_state.insert("bestZeroHundred", 5.7);
    auto telemetry = m_state.value("telemetry").toMap();
    telemetry.insert("redline", 7200); telemetry.insert("steering", -.12);
    telemetry.insert("lateralG", .32); telemetry.insert("longitudinalG", .14);
    telemetry.insert("fuelVolume", 42); telemetry.insert("boost", .8);
    m_state.insert("telemetry", telemetry);
    m_catalog = decorate(QJsonArray{QJsonObject{{"id","etk800"},{"name","800 Series"},{"brand","ETK"},{"type","Car"},
            {"preview",QJsonObject{{"path","/vehicles/etk800/default.jpg"}}}},
        QJsonObject{{"id","scintilla"},{"name","Scintilla"},{"brand","Civetta"},{"type","Car"},
            {"preview",QJsonObject{{"path","/vehicles/scintilla/default.jpg"}}}}});
    m_configs.clear(); m_configsModel.clear();
    for (int i = 0; i < 300; ++i)
        m_history.append(QVariantMap{{"speed", 20 + 8 * std::sin(i * .06)}, {"rpm", 3000 + 900 * std::sin(i * .06)}, {"fps", 85 + 5 * std::sin(i * .13)}, {"ping", 32 + 4 * std::sin(i * .09)}});
    emit historyChanged(); emit catalogChanged(); emit configsChanged();
    m_players = m_state.value("players").toList(); emit playersChanged();
    m_activeVehicles = {QVariantMap{{"id",42},{"name","ETK 800 Series"},{"model","etk800"},{"owned",true}}};
    emit activeVehiclesChanged();
    emit changed();
}
