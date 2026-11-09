#include "previews.h"
#include <QBuffer>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonDocument>
#include <QMutexLocker>
#include <archive.h>
#include <archive_entry.h>

namespace {
constexpr qint64 MaxBytes = 8 * 1024 * 1024;
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
    if (descriptor.value("path").toString().isEmpty()) return {};
    bool available = false;
    for (const auto &root : roots) available |= !root.isEmpty() && QFileInfo(root).isDir();
    if (!available) return {};
    auto identity = QJsonDocument(descriptor).toJson(QJsonDocument::Compact) + roots.join('|').toUtf8();
    for (const auto key : {"archive", "real"}) identity += QByteArray::number(QFileInfo(descriptor.value(key).toString()).lastModified().toMSecsSinceEpoch());
    const auto id = QString::fromLatin1(QCryptographicHash::hash(identity, QCryptographicHash::Sha256).toHex());
    QMutexLocker lock(&m_mutex);
    if (m_entries.size() >= 8192 && !m_entries.contains(id)) m_entries.clear();
    m_entries.insert(id, {descriptor, roots});
    return "image://vehicles/" + id;
}
QImage Previews::requestImage(const QString &id, QSize *size, const QSize &requestedSize) {
    Entry entry;
    { QMutexLocker lock(&m_mutex); entry = m_entries.value(id); }
    const auto virtualPath = entry.descriptor.value("path").toString();
    if (!virtualPath.startsWith("/vehicles/") || virtualPath.contains("..") || virtualPath.contains('\\')) return {};
    const auto key = virtualPath.mid(1);
    const auto archivePath = entry.descriptor.value("archive").toString();
    const auto real = entry.descriptor.value("real").toString();
    QByteArray bytes;
    // The game's mounted origin wins over any loose fallback with the same name.
    if (!archivePath.isEmpty() && archivePath.endsWith(".zip", Qt::CaseInsensitive) && inside(archivePath, entry.roots)) {
        bytes = member(archivePath, key);
    } else {
        QStringList candidates{real};
        for (auto it = entry.roots.crbegin(); it != entry.roots.crend(); ++it) candidates << *it + '/' + key;
        for (const auto &file : candidates) {
            if (!inside(file, entry.roots) || !QFileInfo(file).isFile() || QFileInfo(file).size() > MaxBytes) continue;
            QFile input(file); if (input.open(QIODevice::ReadOnly)) { bytes = input.read(MaxBytes); if (!bytes.isEmpty()) break; }
        }
        if (bytes.isEmpty()) {
            const auto model = key.section('/', 1, 1);
            for (const auto &root : entry.roots) {
                const auto zip = root + "/content/vehicles/" + model + ".zip";
                if (inside(zip, entry.roots)) bytes = member(zip, key);
                if (!bytes.isEmpty()) break;
            }
        }
    }
    QBuffer input(&bytes); input.open(QIODevice::ReadOnly); QImageReader decoder(&input);
    decoder.setAutoDetectImageFormat(false);
    decoder.setFormat(virtualPath.endsWith(".png",Qt::CaseInsensitive) ? "png" : "jpeg");
    const auto original = decoder.size();
    if (!original.isValid() || original.width() > 8192 || original.height() > 8192 || qint64(original.width()) * original.height() > 16000000) return {};
    if (size) *size = original;
    const auto target = requestedSize.isValid() ? requestedSize.boundedTo(QSize(960, 640)) : QSize(480, 320);
    decoder.setScaledSize(original.scaled(target, Qt::KeepAspectRatio));
    return decoder.read();
}
