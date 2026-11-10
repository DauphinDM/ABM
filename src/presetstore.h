#pragma once
#include <QObject>
#include <QJsonObject>
#include <QVariantList>

class PresetStore : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList entries READ entries NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
public:
    explicit PresetStore(const QString &directory, QObject *parent=nullptr);
    QVariantList entries() const { return m_entries; }
    QString message() const { return m_message; }
    QJsonObject get(const QString &id) const;
    Q_INVOKABLE void reload();
    Q_INVOKABLE bool renamePreset(const QString &id,const QString &name);
    Q_INVOKABLE bool duplicate(const QString &id);
    Q_INVOKABLE bool remove(const QString &id,bool confirmed);
    Q_INVOKABLE bool annotate(const QString &id,const QString &tags,const QString &notes);
    Q_INVOKABLE QString exportJson(const QString &id) const;
    Q_INVOKABLE bool importJson(const QString &text);
    QString save(const QString &name,const QJsonObject &configuration,const QJsonObject &settings);
    static bool validSettings(const QJsonObject &settings, QString *reason=nullptr);
    static bool validate(const QJsonObject &preset, QString *reason=nullptr);
signals:
    void changed();
private:
    bool write(QJsonObject preset,bool replace);
    void tell(const QString &message);
    QString path(const QString &id) const;
    QString m_directory,m_message;
    QVariantList m_entries;
};
