// Live JSON translation follows the Omarchy app template (MIT, DHH).
#include "translator.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QLocale>
#include <QRegularExpression>

Translator::Translator(const QString &file, const QString &catalogs, QObject *parent) : QTranslator(parent),
    m_file(file.isEmpty() ? QDir::homePath() + "/.config/omarchy/language" : file), m_catalogs(catalogs) {
    m_debounce.setSingleShot(true); m_debounce.setInterval(80);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { m_debounce.start(); });
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_debounce.start(); });
    connect(&m_debounce, &QTimer::timeout, this, &Translator::reload);
    reload();
}
QString Translator::translate(const char *, const char *source, const char *, int) const {
    const auto value = m_strings.value(QString::fromUtf8(source));
    return value.isString() ? value.toString() : QString();
}
void Translator::reload() {
    const auto paths = m_watcher.files() + m_watcher.directories();
    if (!paths.isEmpty()) m_watcher.removePaths(paths);
    for (const auto &p : {QFileInfo(m_file).absolutePath(), m_file})
        if (QFileInfo::exists(p)) m_watcher.addPath(p);
    QFile file(m_file);
    QString language = file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readLine()).trimmed() : "en";
    if (!QRegularExpression("^[a-z]{2,3}(-[A-Za-z]{2})?$").match(language).hasMatch()) language = "en";
    if (language == m_language) return;
    m_language = language; m_strings = {};
    QStringList codes{"en", language.section('-', 0, 0), language}; codes.removeDuplicates();
    for (const auto &code : codes) {
        QFile catalog(m_catalogs + "/" + code + ".json");
        if (!catalog.open(QIODevice::ReadOnly)) continue;
        const auto object = QJsonDocument::fromJson(catalog.readAll()).object();
        for (auto it = object.begin(); it != object.end(); ++it)
            if (it.value().isString() && !it.value().toString().isEmpty()) m_strings.insert(it.key(), it.value());
    }
    QLocale locale(language);
    QFile registry(qEnvironmentVariable("OMARCHY_PATH", "/usr/share/omarchy") + "/default/i18n/locales.json");
    if (registry.open(QIODevice::ReadOnly)) {
        const auto name = QJsonDocument::fromJson(registry.readAll()).object().value(language).toObject().value("formatLocale").toString();
        if (!name.isEmpty()) locale = QLocale(name);
    }
    QLocale::setDefault(locale);
    QGuiApplication::setLayoutDirection(locale.textDirection());
    emit changed();
}
