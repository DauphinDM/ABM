#include "previews.h"
#include <QBuffer>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonArray>
#include <QMutexLocker>
#include <archive.h>
#include <archive_entry.h>

namespace {
constexpr qint64 MaxBytes = 8 * 1024 * 1024;
constexpr int MaxCandidates = 8;
bool inside(const QString &file, const QStringList &roots) {
    const auto canonical = QFileInfo(file).canonicalFilePath();
    if (canonical.isEmpty()) return false;
    for (const auto &root : roots) {
        const auto base = QFileInfo(root).canonicalFilePath();
        if (base.isEmpty()) continue;
#ifdef Q_OS_WIN
        if (canonical.startsWith(base + '/', Qt::CaseInsensitive)) return true;
#else
        if (canonical.startsWith(base + '/')) return true;
#endif
    }
    return false;
}
QByteArray member(const QString &path, const QString &key) {
    auto *reader = archive_read_new(); archive_read_support_format_zip(reader); archive_read_support_filter_all(reader);
#ifdef Q_OS_WIN
    const auto filename = path.toStdWString();
    const int opened = archive_read_open_filename_w(reader, filename.c_str(), 16384);
#else
    const int opened = archive_read_open_filename(reader, QFile::encodeName(path).constData(), 16384);
#endif
    QByteArray result;
    if (opened == ARCHIVE_OK) {
        archive_entry *entry = nullptr;
        int count = 0;
        while (++count <= 100000 && archive_read_next_header(reader, &entry) == ARCHIVE_OK) {
            const auto name = QString::fromUtf8(archive_entry_pathname(entry));
            if (name == key && archive_entry_filetype(entry) == AE_IFREG
                && !archive_entry_symlink(entry) && !archive_entry_hardlink(entry)) {
                const auto bytes = archive_entry_size(entry);
                if (bytes > 0 && bytes <= MaxBytes) {
                    result.resize(int(bytes)); qint64 offset = 0;
                    while (offset < bytes) {
                        const auto n = archive_read_data(reader, result.data() + offset, size_t(bytes - offset));
                        if (n <= 0) break;
                        offset += n;
                    }
                    if (offset != bytes) result.clear();
                }
                break;
            }
            archive_read_data_skip(reader);
        }
    }
    archive_read_free(reader); return result;
}
}
Previews::Previews() : QQuickImageProvider(QQuickImageProvider::Image, QQmlImageProviderBase::ForceAsynchronousImageLoading) {}
QString Previews::registerImage(const QJsonObject &descriptor, const QStringList &roots) {
    const auto virtualPath = descriptor.value("path").toString();
    if (!virtualPath.startsWith("/vehicles/") || virtualPath.contains("..") || virtualPath.contains('\\')) return {};
    const auto key = virtualPath.mid(1);
    Entry entry;
    entry.format = virtualPath.endsWith(".png", Qt::CaseInsensitive) ? "png" : "jpeg";
    QJsonArray sources;
    const auto add = [&](const QString &file, const QString &member = QString()) {
        if (entry.candidates.size() >= MaxCandidates || file.isEmpty() || !inside(file, roots)) return;
        const QFileInfo info(file);
        if (!info.isFile()) return;
        const auto canonical = info.canonicalFilePath();
        for (const auto &candidate : entry.candidates)
            if (candidate.file == canonical && candidate.member == member) return;
        entry.candidates.append({canonical, member});
        sources.append(QJsonObject{{"file",canonical},{"member",member},{"bytes",QString::number(info.size())},
            {"modified",QString::number(info.lastModified().toMSecsSinceEpoch())}});
    };
    // Preserve the game's reported mounted origin first. Only then try the same
    // virtual image in approved loose roots and the stock model archive.
    const auto archivePath = descriptor.value("archive").toString();
    if (archivePath.endsWith(".zip", Qt::CaseInsensitive)) add(archivePath, key);
    add(descriptor.value("real").toString());
    for (auto it = roots.crbegin(); it != roots.crend(); ++it)
        if (!it->isEmpty()) add(*it + '/' + key);
    const auto model = key.section('/', 1, 1);
    for (const auto &root : roots)
        if (!root.isEmpty()) add(root + "/content/vehicles/" + model + ".zip", key);
    if (entry.candidates.isEmpty()) return {};
    const auto identity = QJsonDocument(QJsonObject{{"path",virtualPath},{"sources",sources}}).toJson(QJsonDocument::Compact);
    const auto id = QString::fromLatin1(QCryptographicHash::hash(identity, QCryptographicHash::Sha256).toHex());
    QMutexLocker lock(&m_mutex);
    if (m_entries.size() >= 8192 && !m_entries.contains(id)) m_entries.clear();
    m_entries.insert(id, entry);
    return "image://vehicles/" + id;
}
QImage Previews::requestImage(const QString &id, QSize *size, const QSize &requestedSize) {
    if (size) *size = {};
    Entry entry;
    { QMutexLocker lock(&m_mutex); entry = m_entries.value(id); }
    if (entry.candidates.isEmpty()) return {};
    const auto target = !requestedSize.isEmpty() ? requestedSize.boundedTo(QSize(960, 640)) : QSize(480, 320);
    const auto cacheKey = id + '/' + QString::number(target.width()) + 'x' + QString::number(target.height());
    QMutexLocker decodeLock(&m_decodeMutex);
    if (const auto *cached = m_cache.object(cacheKey)) {
        if (size) *size = cached->original;
        return cached->image;
    }
    Decoded result;
    for (const auto &candidate : entry.candidates) {
        QByteArray bytes;
        // Recheck containment at read time as well: a source can be replaced by
        // a symlink between catalog registration and asynchronous loading.
        if (QFileInfo(candidate.file).canonicalFilePath() != candidate.file) continue;
        if (!candidate.member.isEmpty()) {
            bytes = member(candidate.file, candidate.member);
        } else {
            QFile input(candidate.file);
            if (!input.open(QIODevice::ReadOnly) || input.size() > MaxBytes) continue;
            bytes = input.read(MaxBytes + 1);
        }
        if (bytes.isEmpty() || bytes.size() > MaxBytes) continue;
        QBuffer input(&bytes); input.open(QIODevice::ReadOnly); QImageReader decoder(&input);
        decoder.setAutoDetectImageFormat(false);
        decoder.setFormat(entry.format);
        const auto original = decoder.size();
        if (!original.isValid() || original.width() > 8192 || original.height() > 8192
            || qint64(original.width()) * original.height() > 16000000) continue;
        decoder.setScaledSize(original.scaled(target, Qt::KeepAspectRatio));
        auto image = decoder.read();
        if (image.isNull()) continue;
        result = {image, original};
        break;
    }
    if (size) *size = result.original;
    // Cache failures too, so corrupt assets are not decoded on every card reuse.
    // Re-registering changed file metadata produces a different provider ID.
    const auto cost = int(qMax<qint64>(1, (result.image.sizeInBytes() + 1023) / 1024));
    m_cache.insert(cacheKey, new Decoded(result), cost);
    return result.image;
}
