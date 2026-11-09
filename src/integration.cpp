#include "integration.h"
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>
#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#endif

Integration::Integration(const QString &path, bool discover, QObject *parent)
    : QObject(parent), m_settings(path, QSettings::IniFormat) {
    execute = [](const QString &program, const QStringList &args, const QString &cwd) {
        return QProcess::startDetached(program, args, cwd);
    };
    if (discover) discoverPaths();
    m_poll.setInterval(2000);
    connect(&m_poll, &QTimer::timeout, this, &Integration::refreshProcesses);
    m_poll.start();
    m_startup.setSingleShot(true); m_startup.setInterval(30000);
    connect(&m_startup, &QTimer::timeout, this, [this] {
        m_launching = false;
        setMessage(tr("Launch request sent. If the game did not open, check Steam."));
    });
    refreshProcesses();
}
void Integration::setMessage(const QString &message) { m_message = message; emit changed(); }
void Integration::setUserFolder(const QString &value) { m_settings.setValue("userFolder", QDir::fromNativeSeparators(value.trimmed())); m_settings.sync(); emit changed(); }
void Integration::setGameFolder(const QString &value) { m_settings.setValue("gameFolder", QDir::fromNativeSeparators(value.trimmed())); m_settings.sync(); emit changed(); }
void Integration::setLauncherPath(const QString &value) { m_settings.setValue("launcherPath", QDir::fromNativeSeparators(value.trimmed())); m_settings.sync(); emit changed(); }
QStringList Integration::processNames() {
    QStringList names;
#ifdef Q_OS_WIN
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return names;
    PROCESSENTRY32W entry{}; entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot, &entry)) do { names << QString::fromWCharArray(entry.szExeFile).toLower(); } while (Process32NextW(snapshot, &entry));
    CloseHandle(snapshot);
#else
    const auto entries = QDir("/proc").entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto &pid : entries) {
        if (pid.isEmpty() || !pid.front().isDigit()) continue;
        QFile cmd("/proc/" + pid + "/cmdline");
        if (!cmd.open(QIODevice::ReadOnly)) continue;
        const auto args = cmd.read(4096).split('\0');
        // Wine/Proton exposes the Windows executable as an argument.
        for (const auto &arg : args) {
            const auto name = QFileInfo(QDir::fromNativeSeparators(QString::fromLocal8Bit(arg))).fileName().toLower();
            if (name.startsWith("beamng.drive") || name.startsWith("beammp-launcher")) names << name;
        }
    }
#endif
    return names;
}
void Integration::refreshProcesses() {
    const auto names = processes();
    bool game = false, mp = false;
    for (const auto &name : names) {
        game |= name == "beamng.drive.x64" || name == "beamng.drive.x64.exe" || name == "beamng.drive.exe"
            || name == "beamng.drive.arm64.exe";
        mp |= name == "beammp-launcher" || name == "beammp-launcher.exe";
    }
    mp |= m_mpLaunchAge.isValid() && m_mpLaunchAge.elapsed() < 5000;
    if (game && m_launching) { m_launching = false; m_startup.stop(); m_message = tr("Game is running. Waiting for the bridge…"); }
    if (m_game != game || m_mp != mp) { m_game = game; m_mp = mp; emit changed(); }
}
void Integration::discoverPaths() {
    const auto home = QDir::homePath();
    if (userFolder().isEmpty()) {
#ifdef Q_OS_WIN
        const auto local = qEnvironmentVariable("LOCALAPPDATA");
        QStringList candidates{local + "/BeamNG/BeamNG.drive/current", local + "/BeamNG.drive/current"};
#else
        QStringList candidates{home + "/.local/share/BeamNG/BeamNG.drive/current"};
#endif
        for (const auto &path : candidates) if (QFileInfo::exists(path + "/beamng.log")) { setUserFolder(QFileInfo(path).canonicalFilePath()); break; }
    }
    QStringList steamRoots;
#ifdef Q_OS_WIN
    QSettings steam("HKEY_CURRENT_USER\\Software\\Valve\\Steam", QSettings::NativeFormat);
    steamRoots << QDir::fromNativeSeparators(steam.value("SteamPath").toString());
#else
    steamRoots << home + "/.steam/steam" << home + "/.local/share/Steam";
#endif
    auto libraries = steamRoots;
    for (const auto &root : steamRoots) {
        QFile file(root + "/steamapps/libraryfolders.vdf");
        if (!file.open(QIODevice::ReadOnly)) continue;
        auto matches = QRegularExpression("\"path\"\\s+\"([^\"]+)\"").globalMatch(QString::fromUtf8(file.readAll()));
        while (matches.hasNext()) libraries << matches.next().captured(1).replace("\\\\", "/");
    }
    if (gameFolder().isEmpty()) for (const auto &library : libraries) {
        const auto folder = library + "/steamapps/common/BeamNG.drive";
        if (QFileInfo::exists(folder + "/content/vehicles")) { setGameFolder(folder); break; }
    }
    if (launcherPath().isEmpty()) {
#ifdef Q_OS_WIN
        const auto program = QStandardPaths::findExecutable("BeamMP-Launcher.exe");
        QStringList candidates{program, qEnvironmentVariable("APPDATA") + "/BeamMP-Launcher/BeamMP-Launcher.exe",
            qEnvironmentVariable("LOCALAPPDATA") + "/BeamMP-Launcher/BeamMP-Launcher.exe"};
#else
        QStringList candidates{QStandardPaths::findExecutable("BeamMP-Launcher"), home + "/beammp-launcher/BeamMP-Launcher"};
#endif
        for (const auto &candidate : candidates) if (!candidate.isEmpty() && QFileInfo(candidate).isExecutable()) { setLauncherPath(candidate); break; }
    }
}
bool Integration::writeBridgeFile(const QString &resource, const QString &relative, const QByteArray &marker) {
    QFile input(resource); if (!input.open(QIODevice::ReadOnly)) return false;
    const auto bytes = input.readAll(); const auto target = userFolder() + '/' + relative;
    QFile current(target);
    if (QFileInfo(target).isSymLink()) { setMessage(tr("Refusing to overwrite a symbolic-link integration file.")); return false; }
    if (current.exists()) {
        if (!current.open(QIODevice::ReadOnly)) return false;
        const auto old = current.readAll(); current.close();
        if (old == bytes) return true;
        if (!old.startsWith(marker)) { setMessage(tr("An unrelated integration file already exists; it was not overwritten.")); return false; }
        const auto backup = target + ".bak." + QDateTime::currentDateTimeUtc().toString("yyyyMMddhhmmsszzz");
        if (!QFile::copy(target, backup)) return false;
    }
    if (!QDir().mkpath(QFileInfo(target).absolutePath())) return false;
    QSaveFile output(target);
    return output.open(QIODevice::WriteOnly) && output.write(bytes) == bytes.size() && output.commit();
}
bool Integration::installBridge() {
    m_message.clear();
    if (userFolder().isEmpty() || !QFileInfo::exists(userFolder() + "/beamng.log")) {
        setMessage(tr("Choose the BeamNG user folder in Settings (run the game once first).")); return false;
    }
    if (!writeBridgeFile(":/bridge/beamdeck.lua", "lua/ge/extensions/beamdeck.lua", "-- Telemetry is read-only")
        || !writeBridgeFile(":/bridge/modScript.lua", "scripts/beamdeck/modScript.lua", "-- User-folder startup script")
        || !writeBridgeFile(":/bridge/abmTuning.lua", "lua/vehicle/extensions/abmTuning.lua", "-- ABM vehicle tuning")
        || !writeBridgeFile(":/bridge/official-servers.json", "lua/ge/extensions/abm-official-servers.json", "{\"schema\":1,")) {
        if (m_message.isEmpty()) setMessage(tr("Could not install the bridge. Check folder permissions."));
        return false;
    }
    setMessage(m_game ? tr("Bridge installed. Restart BeamNG when convenient to load the update.") : tr("Bridge installed; it will connect automatically when BeamNG starts."));
    return true;
}
bool Integration::launch(const QString &program, const QStringList &args, const QString &cwd) {
    if (!execute(program, args, cwd)) { setMessage(tr("Could not start the program. Check its path in Settings.")); return false; }
    return true;
}
bool Integration::startGame() {
    refreshProcesses();
    if (m_game || m_launching) { setMessage(tr("BeamNG is already running or starting.")); return false; }
    if (!installBridge()) return false;
#ifdef Q_OS_WIN
    QSettings steam("HKEY_CURRENT_USER\\Software\\Valve\\Steam", QSettings::NativeFormat);
    const auto program = QDir::fromNativeSeparators(steam.value("SteamExe").toString());
#else
    const auto program = QStandardPaths::findExecutable("steam");
#endif
    if (program.isEmpty()) { setMessage(tr("Steam was not found. Install Steam and BeamNG first.")); return false; }
    if (!launch(program, {"steam://rungameid/284160"}, QFileInfo(program).absolutePath())) return false;
    m_launching = true; m_startup.start(); setMessage(tr("Starting BeamNG…")); return true;
}
bool Integration::beamMPEnabled() const {
    QFile file(userFolder() + "/mods/db.json");
    if (!file.open(QIODevice::ReadOnly)) return false;
    return QJsonDocument::fromJson(file.readAll()).object().value("mods").toObject().value("multiplayerbeammp").toObject().value("active").toBool();
}
bool Integration::startBeamMP() {
    refreshProcesses();
    if (m_mp) { setMessage(tr("BeamMP launcher is already running.")); return true; }
    if (!m_game) { setMessage(tr("Start BeamNG first.")); return false; }
    if (!beamMPEnabled()) { setMessage(tr("Enable the BeamMP mod in the game, then try again. A connected ABM bridge can enable it automatically.")); return false; }
    if (!QFileInfo(launcherPath()).isFile() || !QFileInfo(launcherPath()).isExecutable()) {
        setMessage(tr("Choose the BeamMP launcher executable in Settings.")); return false;
    }
    if (!launch(launcherPath(), {"--no-launch"}, QFileInfo(launcherPath()).absolutePath())) return false;
    m_mpLaunchAge.start(); m_mp = true; setMessage(tr("BeamMP started with --no-launch.")); return true;
}
