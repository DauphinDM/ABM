#pragma once
#include <QQuickImageProvider>
#include <QHash>
#include <QJsonObject>
#include <QMutex>

// Reads local game assets only; no HTTP requests or archive extraction to disk.
class Previews : public QQuickImageProvider {
public:
    Previews();
    QString registerImage(const QJsonObject &descriptor, const QStringList &roots);
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
private:
    struct Entry { QJsonObject descriptor; QStringList roots; };
    QMutex m_mutex;
    QHash<QString, Entry> m_entries;
};
