// Theme watching follows Omacut (MIT, David Heinemeier Hansson).
#include "theme.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

Theme::Theme(const QString &directory, QObject *parent) : QObject(parent),
    m_directory(directory.isEmpty() ? QDir::homePath() + "/.local/state/omarchy/current" : directory) {
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(80);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { m_debounce.start(); });
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_debounce.start(); });
    connect(&m_debounce, &QTimer::timeout, this, &Theme::reload);
    m_timeout.setSingleShot(true);
    m_timeout.setInterval(500);
    connect(&m_timeout, &QTimer::timeout, &m_process, &QProcess::kill);
    connect(&m_process, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        m_timeout.stop();
        const auto v = QJsonDocument::fromJson(m_process.readAllStandardOutput()).object().value("int");
        if (code == 0 && status == QProcess::NormalExit && v.isDouble() && v.toInt(-1) >= 0 && v.toInt() != m_radius) {
            m_radius = v.toInt(); emit changed();
        }
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this] { m_timeout.stop(); });
    connect(&m_poll, &QTimer::timeout, this, [this] {
        if (m_process.state() == QProcess::NotRunning) {
            m_process.start("hyprctl", {"-j", "getoption", "decoration:rounding"});
            m_timeout.start();
        }
    });
    if (directory.isEmpty() && !qEnvironmentVariableIsEmpty("HYPRLAND_INSTANCE_SIGNATURE")) m_poll.start(2000);
    reload();
}
Theme::~Theme() {
    if (m_process.state() != QProcess::NotRunning) { m_process.kill(); m_process.waitForFinished(1000); }
}
QString Theme::readAccent(const QString &path) {
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        const QRegularExpression expression(R"re(^\s*accent\s*=\s*["'](#[0-9a-fA-F]{6})["'])re", QRegularExpression::MultilineOption);
        const auto match = expression.match(QString::fromUtf8(file.readAll()));
        if (match.hasMatch()) return match.captured(1);
    }
    return "#FFD60A";
}
void Theme::reload() {
    const auto watched = m_watcher.files() + m_watcher.directories();
    if (!watched.isEmpty()) m_watcher.removePaths(watched);
    QString ancestor = m_directory;
    while (!QFileInfo::exists(ancestor) && ancestor != "/") ancestor = QFileInfo(ancestor).absolutePath();
    QStringList paths{ancestor, QFileInfo(ancestor).absolutePath(), m_directory, m_directory + "/theme", m_directory + "/theme/colors.toml"};
    paths.removeDuplicates();
    for (const auto &path : paths) if (QFileInfo::exists(path)) m_watcher.addPath(path);
    const auto accent = readAccent(m_directory + "/theme/colors.toml");
    if (accent != m_accent) { m_accent = accent; emit changed(); }
}
