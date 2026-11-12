#pragma once
#include <QQuickImageProvider>
#include <QCache>
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
    struct Candidate { QString file; QString member; };
    struct Entry { QList<Candidate> candidates; QByteArray format; };
    struct Decoded { QImage image; QSize original; };
    QMutex m_mutex;
    // Serialize cache misses, including decode, to coalesce concurrent requests
    // and bound transient decoder allocations. Registration uses a separate lock.
    QMutex m_decodeMutex;
    QHash<QString, Entry> m_entries;
    QCache<QString, Decoded> m_cache{64 * 1024}; // KiB, including negative entries
};
