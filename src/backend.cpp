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
#include <QBuffer>
#include <QStandardPaths>
#include <QUuid>
#include <QCryptographicHash>
#include <QClipboard>
#include <QGuiApplication>
#include <cmath>
#include <limits>

Backend::Backend(const QString &path, QObject *parent) : QObject(parent), m_path(path.isEmpty() ? Telemetry::defaultPath() : path) {
    m_clientId=QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_powerTimer.setInterval(50);
    connect(&m_powerTimer,&QTimer::timeout,this,&Backend::flushLivePower);
    m_tuningSaveTimer.setSingleShot(true); m_tuningSaveTimer.setInterval(1500);
    connect(&m_tuningSaveTimer,&QTimer::timeout,this,&Backend::storeTuning);
    const auto directory = path.isEmpty() ? qEnvironmentVariable("BEAMSIDE_CONFIG_DIR", QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)) : QFileInfo(path).absolutePath();
    m_settings = std::make_unique<QSettings>(directory + "/preferences.ini", QSettings::IniFormat);
    m_integration = std::make_unique<Integration>(directory + "/integration.ini", path.isEmpty());
    m_presets = std::make_unique<PresetStore>(directory + "/presets");
    m_profiles = std::make_unique<PlayerProfiles>();
    m_metric = m_settings->value("metric", true).toBool();
    m_favorites = m_settings->value("favorites").toStringList().mid(0, 256);
    m_convoyNotes = m_settings->value("convoy/notes").toString().left(2048);
    // Cheat-menu sliders keep their last applied values between launches.
    if (m_settings->contains("tuning/power"))
        m_tuning = {{"enginePower", m_settings->value("tuning/power").toDouble()},
                    {"damageImmunity", m_settings->value("tuning/damageImmunity", false).toBool()},
                    {"gravity", m_settings->value("tuning/gravity", -9.81).toDouble()}};
    connect(&m_socket, &QLocalSocket::readyRead, this, &Backend::readStream);
    connect(&m_socket, &QLocalSocket::connected, this, [this] { m_buffer.clear(); });
    connect(&m_socket, &QLocalSocket::disconnected, this, [this] {
        cancelLivePower(); cancelPresetApply();
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
    const auto previousTarget=workshopTarget();
    m_error.clear(); m_state = data.toVariantMap();
    if (previousTarget!=workshopTarget() || !data.value("sandboxAllowed").toBool() || !data.value("connected").toBool()) {
        cancelLivePower(); cancelPresetApply();
        if (!m_pendingTarget.isEmpty()) { m_pending.clear(); m_pendingAction.clear(); m_pendingTarget={}; emit actionChanged(); }
    }
    if (m_integration->gameFolder().isEmpty() && QFileInfo::exists(data.value("gameFolder").toString() + "/content/vehicles"))
        m_integration->setGameFolder(data.value("gameFolder").toString());
    if (m_integration->userFolder().isEmpty() && QFileInfo::exists(data.value("userFolder").toString() + "/beamng.log"))
        m_integration->setUserFolder(data.value("userFolder").toString());
    const bool gameSeen = data.value("gameSeen").toBool();
    if (gameSeen != m_gameSeen) { m_gameSeen = gameSeen; emit gameSeenChanged(); }
    const auto players = data.value("connected").toBool() ? data.value("players").toArray().toVariantList() : QVariantList{};
    // Session IDs are reused on each BeamMP server, even while the bridge
    // process stays loaded. Bind asynchronous profile work before UI signals.
    const auto profileSession = QString::fromUtf8(QJsonDocument(QJsonArray{
        data.value("sessionId"), data.value("serverIp"), data.value("serverPort"), data.value("server")}).toJson(QJsonDocument::Compact));
    m_profiles->setPlayers(profileSession, data.value("connected").toBool() ? players : QVariantList{});
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
            const bool accepted=data.value("ok").toBool() && (m_pendingTarget.isEmpty() || m_pendingTarget==workshopTarget());
            m_pending.clear(); m_pendingAction.clear(); m_pendingTarget={}; m_actionMessage = data.value("message").toString().left(256);
            if (action == "enableBeamMP") {
                if (data.value("ok").toBool()) m_integration->startBeamMP();
                else m_integration->setMessage(m_actionMessage);
            }
            if (action == "tuning" && accepted) {
                m_tuning = QJsonObject{m_nextTuning}.toVariantMap(); m_nextTuning = {};
                storeTuning();
            }
            if (action == "power") {
                if (accepted) { m_tuning.insert("enginePower",m_pendingPower); emit tuningChanged(); m_tuningSaveTimer.start(); }
                else cancelLivePower();
            }
            if (action == "fullReset" && accepted) {
                m_tuning.insert("enginePower",1); m_tuning.insert("damageImmunity",false); storeTuning();
            }
            if (m_presetRunning) {
                if (accepted) { m_presetDone.append(action); sendNextPresetStep(); }
                else {
                    m_presetFailed.append(action);
                    m_presetMessage = tr("Preset step failed: %1. Later steps were not applied.").arg(m_actionMessage);
                    finishPreset();
                }
            }
            emit actionChanged();
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
        m_profiles->setPlayers({}, {});
        if (!m_players.isEmpty()) { m_players.clear(); emit playersChanged(); }
        if (!m_activeVehicles.isEmpty()) { m_activeVehicles.clear(); emit activeVehiclesChanged(); }
        if (!m_configs.isEmpty()) { m_configs.clear(); emit configsChanged(); }
    }
    emit changed();
}
void Backend::toggleMonitoring() {
    if (m_timer.isActive()) { cancelLivePower(); m_timer.stop(); }
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
QJsonObject Backend::workshopTarget() const {
    const auto workshop=m_state.value("workshop").toMap();
    if (!Protocol::workshopBridge(m_state.value("bridgeVersion").toString()) || workshop.value("contract").toInt()!=1 || workshop.value("generation").toString().isEmpty()) return {};
    return {{"targetSession",m_state.value("sessionId").toString()},{"targetVehicle",QJsonValue::fromVariant(m_state.value("vehicleId"))},{"generation",workshop.value("generation").toString()}};
}
void Backend::cancelLivePower() {
    m_queuedPower={}; m_powerTarget={}; m_powerTimer.stop(); m_tuningSaveTimer.stop();
}
void Backend::setLivePower(double multiplier) {
    const auto target=workshopTarget();
    if (!Protocol::enginePower(multiplier) || target.isEmpty() || !connected() || !m_state.value("sandboxAllowed").toBool()
        || m_state.value("policyCleanup").toBool() || m_state.value("workshop").toMap().value("restoring").toBool() || !m_state.value("vehicleOwned").toBool() || !m_state.value("capabilities").toMap().value("power").toBool()) {
        cancelLivePower(); m_actionMessage=tr("Live power unavailable: select a supported own vehicle with bridge 0.9.0 and server permission."); emit actionChanged(); return;
    }
    m_queuedPower={{"enginePower",multiplier}}; m_powerTarget=target; m_powerQueuedAt=QDateTime::currentMSecsSinceEpoch();
    m_powerTimer.start(); flushLivePower();
}
void Backend::flushLivePower() {
    const auto now=QDateTime::currentMSecsSinceEpoch();
    if (m_queuedPower.isEmpty()) { m_powerTimer.stop(); return; }
    if (now-m_powerQueuedAt>3000 || workshopTarget()!=m_powerTarget || !connected() || !m_state.value("sandboxAllowed").toBool()
        || m_state.value("policyCleanup").toBool() || m_state.value("workshop").toMap().value("restoring").toBool()
        || !m_state.value("vehicleOwned").toBool() || !m_state.value("capabilities").toMap().value("power").toBool()) { cancelLivePower(); return; }
    if (busy() || now-m_powerSentAt<250) return;
    const auto parameters=m_queuedPower; m_queuedPower={}; m_pendingPower=parameters.value("enginePower").toDouble(); m_powerSentAt=now;
    send("power",parameters);
}
void Backend::fullReset() {
    cancelLivePower();
    if (m_pendingAction=="power") { m_pending.clear(); m_pendingAction.clear(); m_pendingTarget={}; }
    const auto speed=m_state.value("telemetry").toMap().value("speed");
    if (workshopTarget().isEmpty() || !speed.isValid() || speed.toDouble()>1) { m_actionMessage=tr("Stop a supported vehicle before full reset."); emit actionChanged(); return; }
    send("fullReset");
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
void Backend::setMaxRPM(double value) {
    if (!Protocol::rpm(value)) { m_actionMessage=tr("RPM must be a whole number from 100 to 30000, within the engine's reported ceiling."); emit actionChanged(); return; }
    send("rpm",QJsonObject{{"maxRPM",value}});
}
void Backend::setClutchProtection(bool enabled) { send("clutch",QJsonObject{{"clutchIndestructible",enabled}}); }
void Backend::setResetPersistence(bool enabled) { send("persistence",QJsonObject{{"enabled",enabled}}); }
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
    if (QStringList{"power","fullReset","tuning","handling","gears","engine","dimensions","stability","rpm","clutch","persistence"}.contains(action) && Protocol::workshopBridge(m_state.value("bridgeVersion").toString())) {
        m_pendingTarget=workshopTarget();
        for (auto it=m_pendingTarget.begin();it!=m_pendingTarget.end();++it) message.insert(it.key(),it.value());
        message.insert("contract",1); message.insert("client",m_clientId); message.insert("revision",++m_intentRevision);
        message.insert("expiresAt",QDateTime::currentSecsSinceEpoch()+4);
    } else m_pendingTarget={};
    m_socket.write(QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n');
    m_actionMessage = tr("Waiting for the game…"); emit actionChanged();
    const auto pending = m_pending;
    QTimer::singleShot(16000, this, [this, pending] {
        if (m_pending == pending) { cancelLivePower(); m_pendingTarget={}; m_pending.clear(); m_pendingAction.clear(); m_actionMessage = tr("Game request timed out."); emit actionChanged(); }
    });
}
void Backend::requestCatalog() { send("catalog"); }
QByteArray Backend::mobileThumbnail(const QString &id) {
    if (!m_previews || !QRegularExpression("^[0-9a-f]{64}$").match(id).hasMatch()) return {};
    const auto source = "image://vehicles/" + id;
    bool allowed = false;
    for (const auto &list : {m_catalog, m_configs}) for (const auto &entry : list) {
        const auto item = entry.toMap();
        allowed |= item.value("thumbnail").toString() == source || item.value("modelThumbnail").toString() == source;
    }
    if (!allowed) return {};
    auto image = m_previews->requestImage(id, nullptr, QSize(320,200));
    QByteArray bytes; QBuffer buffer(&bytes); buffer.open(QIODevice::WriteOnly);
    if (!image.isNull()) image.save(&buffer,"JPEG",70);
    return bytes.size() <= 65536 ? bytes : QByteArray{};
}
void Backend::requestConfigs(const QString &model) {
    if (!Protocol::model(model) || busy()) return;
    m_configs.clear(); m_configsModel.clear(); m_requestedModel = model; emit configsChanged();
    if (m_demo) {
        m_configsModel = model;
        m_configs = decorate(QJsonArray{QJsonObject{{"id",model + "/base"},{"model",model},{"key","base"},{"name",tr("Factory configuration")},
            {"source","BeamNG - Official"},{"drivetrain","RWD"},{"transmission","Manual"},
            {"facets",QJsonObject{{"source",QJsonArray{"BeamNG - Official"}},{"drivetrain",QJsonArray{"RWD"}},{"transmission",QJsonArray{"Manual"}},{"configType",QJsonArray{"Factory"}}}},
            {"preview",QJsonObject{{"path","/vehicles/" + model + "/default.jpg"}}}},
            QJsonObject{{"id",model + "/Weekend build"},{"model",model},{"key","Weekend build"},{"name",tr("Weekend build")},{"custom",true},{"source","Custom"},
                {"facets",QJsonObject{{"source",QJsonArray{"Custom"}},{"drivetrain",QJsonArray{"AWD"}},{"transmission",QJsonArray{"Manual"}},{"configType",QJsonArray{"Custom"}}}}}}, model);
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
QJsonObject Backend::presetConfiguration() const {
    const auto workshop = m_state.value("workshop").toMap();
    QStringList parts{m_state.value("vehicle").toString()};
    if (!workshop.value("gearbox").toString().isEmpty()) parts << workshop.value("gearbox").toString();
    for (const auto &value : workshop.value("engines").toList()) {
        const auto name = value.toMap().value("name").toString();
        if (!name.isEmpty()) parts << name;
    }
    parts.sort(Qt::CaseInsensitive);
    return {{"model", m_state.value("vehicle").toString()},
            {"identity", QString::fromLatin1(QCryptographicHash::hash(parts.join('|').toUtf8(), QCryptographicHash::Sha256).toHex())},
            {"label", m_state.value("vehicleName").toString().left(160)},
            {"completeness", "workshop-overrides"}};
}
QJsonObject Backend::presetSettings() const {
    const auto workshop = m_state.value("workshop").toMap();
    QJsonObject settings;
    const auto power = workshop.value("enginePower");
    if (Protocol::enginePower(QJsonValue::fromVariant(power)))
        settings.insert("power", QJsonObject{{"enginePower", power.toDouble()}});
    const auto clutch = workshop.value("clutchProtected");
    if (clutch.isValid() && clutch.metaType().id() == QMetaType::Bool)
        settings.insert("clutch", QJsonObject{{"clutchIndestructible", clutch.toBool()}});
    const auto engines = workshop.value("engines").toList();
    if (engines.size() == 1) {
        const auto rpm = engines.first().toMap().value("maxRPM");
        if (Protocol::rpm(QJsonValue::fromVariant(rpm)))
            settings.insert("rpm", QJsonObject{{"maxRPM", rpm.toDouble()}});
    }
    QJsonArray ratios;
    for (const auto &value : workshop.value("gears").toList()) {
        const auto gear = value.toMap();
        const auto index = gear.value("index");
        const auto ratioValue = gear.value("ratio");
        const double ratio = ratioValue.toDouble();
        if (index.isValid() && index.toInt() > 0 && ratioValue.isValid() && std::isfinite(ratio) && ratio >= .01 && ratio <= 100)
            ratios.append(ratio);
    }
    if (Protocol::gears(ratios)) settings.insert("gears", QJsonObject{{"ratios", ratios}});
    return settings;
}
bool Backend::saveCurrentPreset(const QString &name) {
    const auto configuration = presetConfiguration();
    const auto settings = presetSettings();
    if (workshopTarget().isEmpty() || !connected() || !m_state.value("vehicleOwned").toBool()
        || !Protocol::model(configuration.value("model").toString())) {
        m_presetMessage = tr("Select a supported own vehicle before saving a preset.");
        emit presetChanged(); return false;
    }
    if (settings.isEmpty()) {
        m_presetMessage = tr("No accepted ABM values are readable yet. Apply a setting and wait for readback before saving.");
        emit presetChanged(); return false;
    }
    const auto id = m_presets->save(name, configuration, settings);
    m_presetMessage = id.isEmpty() ? m_presets->message() : tr("Preset saved: accepted ABM overrides only, not a full native vehicle clone.");
    emit presetChanged();
    return !id.isEmpty();
}
void Backend::applyPreset(const QString &id) {
    if (m_presetRunning || busy()) { m_presetMessage = tr("Wait for the current game request before loading a preset."); emit presetChanged(); return; }
    if (m_demo) { m_presetMessage = tr("Demo mode: presets can be browsed but not applied."); emit presetChanged(); return; }
    if (m_state.value("bridgeVersion").toString() != "0.11.0") {
        m_presetMessage = tr("Loading presets needs bridge 0.11.0. Older bridges stay read-only."); emit presetChanged(); return;
    }
    const auto preset = m_presets->get(id);
    if (preset.isEmpty()) { m_presetMessage = tr("Preset is missing, invalid or was written by a newer version."); emit presetChanged(); return; }
    const auto target = workshopTarget();
    const auto telemetry = m_state.value("telemetry").toMap();
    const auto speed = telemetry.value("speed");
    if (target.isEmpty() || !m_state.value("sandboxAllowed").toBool() || !m_state.value("vehicleOwned").toBool()
        || m_state.value("policyCleanup").toBool() || m_state.value("workshop").toMap().value("restoring").toBool()
        || !speed.isValid() || speed.toDouble() > 1) {
        m_presetMessage = tr("Preset loading needs a stopped, owned vehicle with server permission and no restoration in progress.");
        emit presetChanged(); return;
    }
    const auto settings = preset.value("settings").toObject();
    const auto capabilities = m_state.value("capabilities").toMap();
    QVector<PresetStep> steps; QStringList skipped;
    auto add = [&](const QString &key, const QString &action, const QJsonObject &parameters, const bool supported) {
        if (!settings.contains(key)) return;
        if (supported) steps.append({action, key, parameters}); else skipped.append(key);
    };
    add("power", "tuning", QJsonObject{{"enginePower", settings.value("power").toObject().value("enginePower")}}, capabilities.value("power").toBool());
    add("rpm", "rpm", QJsonObject{{"maxRPM", settings.value("rpm").toObject().value("maxRPM")}}, capabilities.value("rpm").toBool());
    add("clutch", "clutch", QJsonObject{{"clutchIndestructible", settings.value("clutch").toObject().value("clutchIndestructible")}}, capabilities.value("clutch").toBool());
    add("gears", "gears", QJsonObject{{"ratios", settings.value("gears").toObject().value("ratios")}}, capabilities.value("gears").toBool());
    add("handling", "handling", settings.value("handling").toObject(),
        capabilities.value("grip").toBool() && capabilities.value("brakes").toBool() && capabilities.value("steering").toBool());
    add("engine", "engine", settings.value("engine").toObject(), capabilities.value("engine").toBool());
    add("dimensions", "dimensions", settings.value("dimensions").toObject(), capabilities.value("dimensions").toBool());
    add("beam", "tuning", QJsonObject{{"damageImmunity", settings.value("beam").toObject().value("damageImmunity")}}, capabilities.value("deformation").toBool());
    if (steps.isEmpty()) {
        m_presetMessage = tr("This preset has no steps supported by the current vehicle. Nothing was changed.");
        emit presetChanged(); return;
    }
    m_presetSteps = steps; m_presetId = id; m_presetIndex = 0; m_presetRunning = true;
    m_presetDone.clear(); m_presetFailed.clear(); m_presetSkipped = skipped;
    m_presetProgress = QString("0/%1").arg(steps.size());
    m_presetMessage = tr("Applying preset one verified step at a time. Cancelling stops unsent steps; it cannot undo acknowledged steps.");
    emit presetChanged();
    sendNextPresetStep();
}
void Backend::sendNextPresetStep() {
    if (!m_presetRunning) return;
    if (m_presetIndex >= m_presetSteps.size()) { finishPreset(); return; }
    if (workshopTarget().isEmpty() || !m_state.value("sandboxAllowed").toBool()
        || m_state.value("workshop").toMap().value("restoring").toBool()) {
        m_presetFailed.append(m_presetSteps.at(m_presetIndex).key);
        m_presetMessage = tr("Vehicle or policy target changed; remaining preset steps were not sent.");
        finishPreset(); return;
    }
    const auto step = m_presetSteps.at(m_presetIndex++);
    m_presetProgress = QString("%1/%2").arg(m_presetIndex).arg(m_presetSteps.size());
    emit presetChanged();
    send(step.action, step.parameters);
    if (!busy()) { m_presetFailed.append(step.key); finishPreset(); }
}
void Backend::finishPreset(bool cancelled) {
    const QVector<PresetStep> steps = m_presetSteps;
    const QStringList done = m_presetDone, failed = m_presetFailed, skipped = m_presetSkipped;
    m_presetRunning = false;
    m_presetSteps.clear(); m_presetIndex = 0; m_presetId.clear();
    m_presetProgress.clear();
    QStringList parts;
    if (!done.isEmpty()) parts << tr("applied %1").arg(done.join(", "));
    if (!failed.isEmpty()) parts << tr("failed %1").arg(failed.join(", "));
    if (!skipped.isEmpty()) parts << tr("skipped unsupported %1").arg(skipped.join(", "));
    if (cancelled) parts.prepend(tr("cancelled"));
    m_presetMessage = parts.isEmpty() ? tr("Preset finished without changes.")
        : tr("Preset %1. Acknowledged steps are not atomic and cannot be rolled back by ABM.").arg(parts.join(" · "));
    Q_UNUSED(steps);
    emit presetChanged();
}
void Backend::cancelPresetApply() {
    if (!m_presetRunning) { clearPreset(); return; }
    finishPreset(true);
}
void Backend::clearPreset() {
    if (m_presetSteps.isEmpty() && m_presetId.isEmpty() && m_presetMessage.isEmpty()) return;
    m_presetSteps.clear(); m_presetIndex = 0; m_presetId.clear();
    m_presetMessage.clear(); m_presetProgress.clear();
    m_presetDone.clear(); m_presetFailed.clear(); m_presetSkipped.clear();
    emit presetChanged();
}
QVariantMap Backend::randomTune(const QString &seed) const {
    const auto value = seed.isEmpty() ? QString::number(QDateTime::currentMSecsSinceEpoch()) : seed.left(64);
    const auto bytes = QCryptographicHash::hash(value.toUtf8(), QCryptographicHash::Sha256);
    auto unit = [&](int index) {
        const auto b = reinterpret_cast<const uchar *>(bytes.constData()) + (index * 4) % 32;
        const quint32 n = (quint32(b[0]) << 24) | (quint32(b[1]) << 16) | (quint32(b[2]) << 8) | quint32(b[3]);
        return double(n) / double(std::numeric_limits<quint32>::max());
    };
    auto between = [&](int index, double low, double high) { return std::round((low + (high - low) * unit(index)) * 100.0) / 100.0; };
    return {{"seed", value}, {"enginePower", between(0, .8, 1.5)}, {"tireGrip", between(1, .6, 1.4)},
            {"slidingGrip", between(2, .6, 1.4)}, {"brakePower", between(3, .7, 1.5)},
            {"steeringResponse", between(4, .8, 1.2)}};
}
QString Backend::supportBundle() const {
    QJsonObject data;
    data.insert("app", "ABM"); data.insert("version", QCoreApplication::applicationVersion());
    data.insert("bridge", m_state.value("bridgeVersion").toString());
    data.insert("connected", connected());
    data.insert("multiplayer", m_state.value("multiplayer").toBool());
    data.insert("sandboxAllowed", m_state.value("sandboxAllowed").toBool());
    data.insert("policyReason", m_state.value("policyReason").toString().left(256));
    data.insert("players", int(m_players.size()));
    data.insert("presets", int(m_presets->entries().size()));
    data.insert("presetError", m_presets->message().left(200));
    data.insert("error", m_error.left(200));
    return QString::fromUtf8(QJsonDocument(data).toJson(QJsonDocument::Indented));
}
bool Backend::copyServerAddress() {
    const auto ip = m_state.value("serverIp").toString();
    const auto port = m_state.value("serverPort").toInt();
    if (!connected() || !m_state.value("multiplayer").toBool() || ip.isEmpty() || ip.size() > 64 || port < 1 || port > 65535) {
        m_actionMessage = tr("No verified server address in the current session."); emit actionChanged(); return false;
    }
    auto *clipboard = QGuiApplication::clipboard();
    if (!clipboard) { m_actionMessage = tr("Clipboard is unavailable."); emit actionChanged(); return false; }
    const auto address = (ip.contains(':') ? '[' + ip + ']' : ip) + ':' + QString::number(port);
    clipboard->setText(address);
    if (clipboard->text()!=address) { m_actionMessage=tr("Clipboard did not retain the server address."); emit actionChanged(); return false; }
    m_actionMessage = tr("Copied server address only. No password, chat or invented invite link."); emit actionChanged();
    return true;
}
void Backend::setConvoyNotes(const QString &notes) {
    const auto bounded = notes.left(2048);
    if (m_convoyNotes == bounded) return;
    m_convoyNotes = bounded; m_settings->setValue("convoy/notes", bounded); m_settings->sync(); emit preferencesChanged();
}
void Backend::setDemo(bool blocked) {
    m_demo = true;
    m_socket.abort();
    m_timer.stop();
    m_state.clear(); m_history.clear(); m_catalog.clear(); m_players.clear(); m_activeVehicles.clear();
    m_pending.clear(); m_pendingAction.clear();
    m_error.clear();
    m_state = QJsonDocument::fromJson(R"({"connected":true,"multiplayer":true,"beammpLoaded":true,"server":"Coastal cruising · EU","map":"west_coast_usa","vehicle":"etk800","fps":89,"vehicleCount":14,"sessionSeconds":1452,"distance":18420,"topSpeed":62.5,"serverIp":"203.0.113.7","serverPort":30814,"playerCount":3,"mpVehicleCount":5,"ping":32,"telemetry":{"speed":28.6,"rpm":3240,"gear":4,"fuel":0.72,"throttle":0.42,"brake":0,"coolant":88,"oil":96},"players":[{"name":"CoastDriver","rawName":"CoastDriver","sessionPlayerId":1,"local":true,"ping":32,"pingSource":"local","pingAgeMs":400,"vehicles":1,"role":"USER"},{"name":"Apex","rawName":"Apex","sessionPlayerId":2,"ping":48,"pingSource":"remote","pingAgeMs":700,"vehicles":2,"role":"USER"},{"name":"Tourer","rawName":"Tourer","sessionPlayerId":3,"vehicles":2,"guest":true,"role":"USER"}]})").object().toVariantMap();
    m_state.insert("vehicleName", "ETK 800 Series"); m_state.insert("bridgeVersion", "0.11.0");
    m_state.insert("workshop",QVariantMap{{"contract",1},{"generation","demo-instance:0"},{"enginePower",1},
        {"persistence",true},{"restoring",false},{"restoreStatus","saved"},{"clutchProtected",false},{"gearbox","6-speed manual"},{"gearboxType","manual"},{"gearCount",6},
        {"engines",QVariantList{QVariantMap{{"name","mainEngine"},{"maxRPM",7200},{"revLimiterRPM",7200},{"physicalRPM",7560},{"rpmCeiling",8000},{"idleRPM",800}}}},
        {"gears",QVariantList{QVariantMap{{"index",-1},{"ratio",-3.2}},QVariantMap{{"index",0},{"ratio",0}},QVariantMap{{"index",1},{"ratio",3.5}},QVariantMap{{"index",2},{"ratio",2.1}},QVariantMap{{"index",3},{"ratio",1.4}},QVariantMap{{"index",4},{"ratio",1.0}},QVariantMap{{"index",5},{"ratio",.8}}}}});
    m_state.insert("sandboxAllowed", !blocked);
    m_state.insert("policyReason", blocked ? tr("Official event/new or mismatched server: gameplay changes are blocked.") : tr("Non-official server: no ABM official-server restriction."));
    if (blocked) m_state.insert("server", "Official event · demo");
    m_state.insert("controlsAvailable", true); m_state.insert("sampleHz", 20);
    m_state.insert("vehicleId",42); m_state.insert("vehicleOwned",true);
    QVariantMap capabilities;
    for (const auto key : {"power","engine","grip","brakes","steering","gears","dimensions","deformation","stability","ignite","extinguish","deflate","dealer","repair","rpm","clutch"}) capabilities.insert(key,true);
    m_state.insert("capabilities",capabilities);
    m_state.insert("gameSeen", true); m_state.insert("fpsAverage", 86); m_state.insert("fpsMinimum", 63);
    m_gameSeen = true;
    m_state.insert("cpuMs", 9.4); m_state.insert("gpuMs", 11.2);
    m_state.insert("averageSpeed", 19.2); m_state.insert("bestZeroHundred", 5.7);
    auto telemetry = m_state.value("telemetry").toMap();
    telemetry.insert("redline", 7200); telemetry.insert("steering", -.12);
    telemetry.insert("lateralG", .32); telemetry.insert("longitudinalG", .14);
    telemetry.insert("fuelVolume", 42); telemetry.insert("boost", .8);
    m_state.insert("telemetry", telemetry);
    QJsonArray models{
        QJsonObject{{"id","etk800"},{"name","800 Series"},{"brand","ETK"},{"type","Car"},{"source","BeamNG - Official"},
            {"facets",QJsonObject{{"type",QJsonArray{"Car"}},{"brand",QJsonArray{"ETK"}},{"source",QJsonArray{"BeamNG - Official","Custom"}},{"bodyStyle",QJsonArray{"Sedan","Wagon"}},{"country",QJsonArray{"Germany"}},{"drivetrain",QJsonArray{"AWD","RWD"}},{"transmission",QJsonArray{"Manual","Automatic"}},{"fuelType",QJsonArray{"Gasoline","Diesel"}},{"propulsion",QJsonArray{"ICE"}}}}},
        QJsonObject{{"id","scintilla"},{"name","Scintilla"},{"brand","Civetta"},{"type","Car"},{"source","BeamNG - Official"},
            {"facets",QJsonObject{{"bodyStyle",QJsonArray{"Coupe"}},{"country",QJsonArray{"Italy"}},{"transmission",QJsonArray{"DCT"}}}}},
        QJsonObject{{"id","tseries"},{"name","T-Series"},{"brand","Gavril"},{"type","Truck"},{"source","BeamNG - Official"},
            {"facets",QJsonObject{{"bodyStyle",QJsonArray{"Semi Truck"}},{"fuelType",QJsonArray{"Diesel"}},{"country",QJsonArray{"United States"}}}}},
        QJsonObject{{"id","pickup"},{"name","D-Series"},{"brand","Gavril"},{"type","Truck"},{"source","BeamNG - Official"},
            {"facets",QJsonObject{{"bodyStyle",QJsonArray{"Pickup"}},{"country",QJsonArray{"United States"}}}}},
        QJsonObject{{"id","demo_rally"},{"name","Rally truck · demo mod"},{"brand","Sample Works"},{"type","Truck"},{"source","Sample Rally Pack"},
            {"facets",QJsonObject{{"source",QJsonArray{"Sample Rally Pack"}},{"configType",QJsonArray{"Rally"}},{"drivetrain",QJsonArray{"AWD"}}}}}
    };
    for (qsizetype index = 0; index < models.size(); ++index) {
        auto item = models[index].toObject();
        auto facets = item.value("facets").toObject();
        for (const auto key : {"type","brand","source"}) if (!facets.contains(key)) facets.insert(key,QJsonArray{item.value(key)});
        item.insert("facets",facets);
        const auto id = item.value("id").toString();
        item.insert("preview",QJsonObject{{"path","/vehicles/" + (id == "demo_rally" ? QString("tseries") : id) + "/default.jpg"}});
        models[index] = item;
    }
    m_catalog = decorate(models);
    m_configs.clear(); m_configsModel.clear();
    for (int i = 0; i < 300; ++i)
        m_history.append(QVariantMap{{"speed", 20 + 8 * std::sin(i * .06)}, {"rpm", 3000 + 900 * std::sin(i * .06)}, {"fps", 85 + 5 * std::sin(i * .13)}, {"ping", 32 + 4 * std::sin(i * .09)}});
    emit historyChanged(); emit catalogChanged(); emit configsChanged();
    m_players = m_state.value("players").toList();
    // Demo rows need the same session identity keys as sanitized live packets.
    for (auto &entry : m_players) {
        auto player = entry.toMap();
        player.insert("rowKey", "id:" + player.value("sessionPlayerId").toString());
        entry = player;
    }
    m_state.insert("players", m_players);
    emit playersChanged();
    m_profiles->setPlayers(m_state.value("sessionId").toString(), m_players);
    m_activeVehicles = {QVariantMap{{"id",42},{"name","ETK 800 Series"},{"model","etk800"},{"owned",true}}};
    emit activeVehiclesChanged();
    emit changed();
}
