#pragma once
#include <QElapsedTimer>
#include <QJsonObject>
#include <QTimer>
#include <QUdpSocket>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSet>
#include <QHash>
#include <QJsonArray>
#include <QPointer>
#include "serverpolicy.h"

// Only the receiver owns the socket. Apps and bar instances share its snapshot.
class Telemetry : public QObject {
    Q_OBJECT
public:
    explicit Telemetry(const QString &path, QObject *parent = nullptr);
    ~Telemetry() override;
    bool start(quint16 port = 44444);
    QString errorString() const { return m_error; }
    static QJsonObject sanitize(const QByteArray &bytes);
    static QString defaultPath();
    static QString socketPath(const QString &statePath);
    static QString cleanName(const QString &text);
private:
    void drain();
    void publish();
    QJsonObject state() const;
    void broadcast(const QJsonObject &message);
    void command(QLocalSocket *client, const QJsonObject &message);
    void resetTrip();
    void updatePolicy();
    QJsonObject policy() const;
    QString m_path;
    QString m_error;
    QUdpSocket m_socket;
    QTimer m_timer;
    QElapsedTimer m_lastPacket, m_session, m_sample;
    QJsonObject m_data;
    ServerPolicy m_policy;
    QElapsedTimer m_policySent;
    QLocalServer m_server;
    QSet<QLocalSocket *> m_clients;
    QHash<QLocalSocket *, QByteArray> m_buffers;
    struct Request {
        QPointer<QLocalSocket> client;
        QString action, model;
        QJsonArray items;
        int chunk = 0;
    };
    QHash<QString, Request> m_requests;
    QJsonArray m_catalog;
    QString m_catalogRequest;
    quint16 m_peerPort = 0;
    QString m_peerSession;
    double m_distance = 0, m_topSpeed = 0;
    double m_fpsSum = 0, m_fpsMin = 2000;
    int m_fpsSamples = 0;
    double m_sprintStart = -1, m_bestSprint = 0, m_acceleration = 0;
    bool m_sprintArmed = false;
    double m_previousSpeed = 0;
    bool m_previousSpeedValid = false;
    bool m_gameWasSeen = false;
};
