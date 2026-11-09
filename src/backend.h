#pragma once
#include <QObject>
#include <QTimer>
#include <QVariantMap>
#include <QLocalSocket>
#include <QElapsedTimer>
#include <QSettings>
#include <memory>
#include <QJsonObject>
#include "integration.h"
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
    void setPreviews(Previews *previews) { m_previews = previews; }
    bool gameSeen() const { return m_gameSeen; }
    void setMetric(bool metric);
    Q_INVOKABLE void toggleFavorite(const QString &model);
    Q_INVOKABLE void requestCatalog();
    Q_INVOKABLE void requestConfigs(const QString &model);
    Q_INVOKABLE void requestVehicle(const QString &action, const QString &model);
    Q_INVOKABLE void applyTuning(QVariantMap tuning);
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
    void setDemo();
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
private:
    void accept(const QJsonObject &state);
    void send(const QString &action, const QJsonObject &parameters = {});
    void storeTuning();
    void readStream();
    void reconnect();
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
    Previews *m_previews = nullptr;
    std::unique_ptr<Integration> m_integration;
    bool m_demo = false, m_metric = true, m_gameSeen = false;
    QStringList m_favorites;
    std::unique_ptr<QSettings> m_settings;
};
