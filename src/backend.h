#pragma once
#include <QObject>
#include <QTimer>
#include <QVariantMap>
#include <QLocalSocket>
#include <QElapsedTimer>
#include <QSettings>
#include <memory>
#include <QJsonObject>
#include <QVector>
#include "integration.h"
#include "playerprofiles.h"
#include "presetstore.h"
#include "previews.h"

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap state READ state NOTIFY changed)
    Q_PROPERTY(bool connected READ connected NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(bool monitoring READ monitoring NOTIFY changed)
    Q_PROPERTY(QVariantList history READ history NOTIFY historyChanged)
    Q_PROPERTY(QVariantList catalog READ catalog NOTIFY catalogChanged)
    Q_PROPERTY(QString actionMessage READ actionMessage NOTIFY actionChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY actionChanged)
    Q_PROPERTY(bool metric READ metric WRITE setMetric NOTIFY preferencesChanged)
    Q_PROPERTY(QStringList favorites READ favorites NOTIFY preferencesChanged)
    Q_PROPERTY(QVariantList players READ players NOTIFY playersChanged)
    Q_PROPERTY(QVariantList activeVehicles READ activeVehicles NOTIFY activeVehiclesChanged)
    Q_PROPERTY(QVariantList configs READ configs NOTIFY configsChanged)
    Q_PROPERTY(QString configsModel READ configsModel NOTIFY configsChanged)
    Q_PROPERTY(QString pendingAction READ pendingAction NOTIFY actionChanged)
    Q_PROPERTY(QVariantMap tuning READ tuning NOTIFY tuningChanged)
    Q_PROPERTY(Integration* launcher READ launcher CONSTANT)
    Q_PROPERTY(PlayerProfiles* profiles READ profiles CONSTANT)
    Q_PROPERTY(PresetStore* presets READ presets CONSTANT)
    Q_PROPERTY(bool presetRunning READ presetRunning NOTIFY presetChanged)
    Q_PROPERTY(QString presetMessage READ presetMessage NOTIFY presetChanged)
    Q_PROPERTY(QString presetProgress READ presetProgress NOTIFY presetChanged)
    Q_PROPERTY(QString convoyNotes READ convoyNotes WRITE setConvoyNotes NOTIFY preferencesChanged)
public:
    explicit Backend(const QString &path = {}, QObject *parent = nullptr);
    ~Backend() override;
    QVariantMap state() const { return m_state; }
    bool connected() const { return m_state.value("connected").toBool(); }
    QString error() const { return m_error; }
    bool monitoring() const { return m_timer.isActive(); }
    QVariantList history() const { return m_history; }
    QVariantList catalog() const { return m_catalog; }
    QString actionMessage() const { return m_actionMessage; }
    bool busy() const { return !m_pending.isEmpty(); }
    bool metric() const { return m_metric; }
    QStringList favorites() const { return m_favorites; }
    QVariantList players() const { return m_players; }
    QVariantList activeVehicles() const { return m_activeVehicles; }
    QVariantList configs() const { return m_configs; }
    QString configsModel() const { return m_configsModel; }
    QString pendingAction() const { return m_pendingAction; }
    QVariantMap tuning() const { return m_tuning; }
    Integration *launcher() const { return m_integration.get(); }
    PlayerProfiles *profiles() const { return m_profiles.get(); }
    PresetStore *presets() const { return m_presets.get(); }
    bool presetRunning() const { return m_presetRunning; }
    QString presetMessage() const { return m_presetMessage; }
    QString presetProgress() const { return m_presetProgress; }
    QString convoyNotes() const { return m_convoyNotes; }
    void setPreviews(Previews *previews) { m_previews = previews; }
    bool gameSeen() const { return m_gameSeen; }
    void setMetric(bool metric);
    Q_INVOKABLE void toggleFavorite(const QString &model);
    Q_INVOKABLE void requestCatalog();
    Q_INVOKABLE void requestConfigs(const QString &model);
    Q_INVOKABLE void requestVehicle(const QString &action, const QString &model);
    Q_INVOKABLE void applyTuning(QVariantMap tuning);
    Q_INVOKABLE void setLivePower(double multiplier);
    Q_INVOKABLE void cancelPowerEdits() { cancelLivePower(); }
    Q_INVOKABLE void fullReset();
    Q_INVOKABLE void setMaxRPM(double value);
    Q_INVOKABLE void setClutchProtection(bool enabled);
    Q_INVOKABLE void setResetPersistence(bool enabled);
    Q_INVOKABLE void applyHandling(QVariantMap settings);
    Q_INVOKABLE void applyEngine(QVariantMap settings);
    Q_INVOKABLE void applyDimensions(QVariantMap settings);
    Q_INVOKABLE void applyGears(const QString &ratios);
    Q_INVOKABLE bool saveCurrentPreset(const QString &name);
    Q_INVOKABLE void applyPreset(const QString &id);
    Q_INVOKABLE void cancelPresetApply();
    Q_INVOKABLE QVariantMap randomTune(const QString &seed) const;
    Q_INVOKABLE QString supportBundle() const;
    Q_INVOKABLE bool copyServerAddress();
    Q_INVOKABLE void setConvoyNotes(const QString &notes);
    Q_INVOKABLE void carAction(const QString &name);
    Q_INVOKABLE void switchVehicle(int id);
    Q_INVOKABLE void resetTrip();
    Q_INVOKABLE void loadExtension();
    Q_INVOKABLE bool startGame();
    Q_INVOKABLE bool startBeamMP();
    Q_INVOKABLE bool launchAuto();
    Q_INVOKABLE bool launcherAvailable();
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void toggleMonitoring();
    void setDemo(bool blocked = false);
signals:
    void changed();
    void historyChanged();
    void catalogChanged();
    void actionChanged();
    void preferencesChanged();
    void playersChanged();
    void activeVehiclesChanged();
    void configsChanged();
    void tuningChanged();
    void gameSeenChanged();
    void presetChanged();
private:
    void accept(const QJsonObject &state);
    void send(const QString &action, const QJsonObject &parameters = {});
    void storeTuning();
    void sendNextPresetStep();
    void finishPreset(bool cancelled = false);
    void clearPreset();
    QJsonObject presetConfiguration() const;
    QJsonObject presetSettings() const;
    void readStream();
    void reconnect();
    QJsonObject workshopTarget() const;
    void cancelLivePower();
    void flushLivePower();
    QVariantList decorate(const QJsonArray &items, const QString &model = {});
    QString m_path, m_error;
    QVariantMap m_state;
    QTimer m_timer;
    QLocalSocket m_socket;
    QByteArray m_buffer;
    QVariantList m_history, m_catalog, m_configs;
    QVariantList m_players, m_activeVehicles;
    QElapsedTimer m_streamAge, m_historyAge;
    QString m_pending, m_pendingAction, m_actionMessage;
    QString m_configsModel, m_requestedModel;
    QVariantMap m_tuning;
    QJsonObject m_nextTuning;
    QTimer m_powerTimer, m_tuningSaveTimer;
    QJsonObject m_queuedPower, m_powerTarget, m_pendingTarget;
    QString m_clientId;
    qint64 m_powerQueuedAt = 0, m_powerSentAt = 0;
    int m_intentRevision = 0;
    double m_pendingPower = 1;
    Previews *m_previews = nullptr;
    std::unique_ptr<Integration> m_integration;
    std::unique_ptr<PlayerProfiles> m_profiles;
    std::unique_ptr<PresetStore> m_presets;
    struct PresetStep { QString action, key; QJsonObject parameters; };
    QVector<PresetStep> m_presetSteps;
    QString m_presetId, m_presetMessage, m_presetProgress, m_convoyNotes;
    QStringList m_presetDone, m_presetFailed, m_presetSkipped;
    int m_presetIndex = 0;
    bool m_presetRunning = false;
    bool m_demo = false, m_metric = true, m_gameSeen = false;
    QStringList m_favorites;
    std::unique_ptr<QSettings> m_settings;
};
