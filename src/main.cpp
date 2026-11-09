#include <QGuiApplication>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLockFile>
#include <QPalette>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include <QSaveFile>
#include <QStandardPaths>
#include <QEventLoop>
#include <cstdio>
#include "backend.h"
#include "telemetry.h"
#include "theme.h"
#include "translator.h"
#include "protocol.h"

int main(int argc, char *argv[]) {
    QStringList args;
    for (int i = 1; i < argc; ++i) args << QString::fromLocal8Bit(argv[i]);
    const auto command = args.value(0, "open");
    const bool capture = args.contains("--screenshot"), demo = args.contains("--demo");
    const bool help = args.contains("--help") || args.contains("-h");
    const QStringList commands{"open","serve","status","preferences","skill","setup","launch","launch-status"};
    if (command != "open" || capture || help) qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);
    app.setApplicationName("abm");
    app.setApplicationDisplayName("ABM");
    app.setApplicationVersion("0.7.0");
    app.setDesktopFileName("abm");
    app.setWindowIcon(QIcon::fromTheme("abm"));
    if (help) {
        puts("ABM 0.7 — Advanced BeamNG.Drive Manager\n\nabm open [--tab drive|car|garage|session]\nabm serve\nabm status --json\nabm launch --auto --json   Start BeamNG, or BeamMP with --no-launch\nabm launch-status --json\nabm setup --user-folder PATH [--game-folder PATH] [--launcher PATH] --json\nabm preferences --json [--units kmh|mph] [--favorite MODEL/CONFIG]\nabm skill install --json\nabm open --demo --screenshot PATH");
        return 0;
    }
    if (!commands.contains(command)) { fputs("Unknown command; use --help.\n", stderr); return 1; }
    quint16 receiverPort = 44444;
    if (args.contains("--receiver-port")) {
        bool ok = false; receiverPort = args.value(args.indexOf("--receiver-port")+1).toUShort(&ok);
        if (!ok || !receiverPort) { fputs("Invalid receiver port.\n",stderr); return 1; }
    }
    auto print = [](const QJsonObject &data) { puts(QJsonDocument(data).toJson(QJsonDocument::Compact).constData()); };
    if (command == "skill") {
        if (args.value(1) != "install") { fputs("Expected: abm skill install --json\n", stderr); return 1; }
        QFile source(":/agent-skill.md"); if (!source.open(QIODevice::ReadOnly)) return 1;
        const auto bytes = source.readAll(); const auto path = QDir::homePath() + "/.agents/skills/abm/SKILL.md";
        QFile previous(path);
        if (previous.exists() && (!previous.open(QIODevice::ReadOnly) || previous.readAll() != bytes)) {
            fputs("Refusing to overwrite an existing, different agent skill.\n", stderr); return 1;
        }
        QDir().mkpath(QFileInfo(path).absolutePath()); QSaveFile output(path);
        if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size() || !output.commit()) return 1;
        print({{"ok",true},{"path",path}}); return 0;
    }
    if (command == "serve") {
        Telemetry receiver(Telemetry::defaultPath());
        if (!receiver.start(receiverPort)) { fprintf(stderr,"Receiver unavailable: %s\n",qPrintable(receiver.errorString())); return 1; }
        return app.exec();
    }
    if (command == "status" || command == "preferences" || command == "setup" || command == "launch" || command == "launch-status") {
        Backend backend;
        if (command == "status") {
            auto data = QJsonObject::fromVariantMap(backend.state()); data.insert("connected", backend.connected());
            if (!backend.error().isEmpty()) data.insert("error",backend.error());
            print(data); return backend.connected() ? 0 : 1;
        }
        if (command == "preferences") {
            if (args.contains("--units")) {
                const auto units = args.value(args.indexOf("--units")+1);
                if (units != "kmh" && units != "mph") { fputs("Units must be kmh or mph.\n",stderr); return 1; }
                backend.setMetric(units == "kmh");
            }
            if (args.contains("--favorite")) {
                const auto id = args.value(args.indexOf("--favorite")+1);
                if (!Protocol::identity(id)) { fputs("Invalid vehicle identifier.\n",stderr); return 1; }
                backend.toggleFavorite(id);
            }
            print({{"metric",backend.metric()},{"favorites",QJsonArray::fromStringList(backend.favorites())}}); return 0;
        }
        auto *launcher = backend.launcher();
        if (command == "launch-status") { print({{"gameRunning",launcher->gameRunning()},{"beammpRunning",launcher->beammpRunning()}}); return 0; }
        if (command == "setup") {
            if (args.contains("--user-folder")) launcher->setUserFolder(args.value(args.indexOf("--user-folder")+1));
            if (args.contains("--game-folder")) launcher->setGameFolder(args.value(args.indexOf("--game-folder")+1));
            if (args.contains("--launcher")) launcher->setLauncherPath(args.value(args.indexOf("--launcher")+1));
            const bool ok = launcher->installBridge(); print({{"ok",ok},{"message",launcher->message()}}); return ok ? 0 : 1;
        }
        // Give an existing receiver a chance to connect before enabling the mod.
        QEventLoop connection; QTimer::singleShot(100,&connection,&QEventLoop::quit); connection.exec();
        bool ok = backend.launchAuto();
        if (ok && backend.busy()) {
            QEventLoop wait;
            QObject::connect(&backend,&Backend::actionChanged,&wait,[&] { if (!backend.busy()) wait.quit(); });
            QTimer::singleShot(4000,&wait,&QEventLoop::quit); wait.exec();
            ok = !backend.busy() && launcher->beammpRunning();
        }
        print({{"ok",ok},{"message",launcher->message().isEmpty() ? backend.actionMessage() : launcher->message()}}); return ok ? 0 : 1;
    }
    auto tab = args.contains("--tab") ? args.value(args.indexOf("--tab")+1) : QString("drive");
    if (tab == "beammp") tab = "session"; // Compatibility for old shortcuts.
    const int tabIndex = QStringList{"drive","car","garage","session"}.indexOf(tab);
    if (tabIndex < 0) { fputs("Unknown tab: choose drive, car, garage or session.\n",stderr); return 1; }
    if (capture && (!demo || args.indexOf("--screenshot")+1 >= args.size())) {
        fputs("Screenshots require --demo and an output path.\n",stderr); return 1;
    }
    QDir().mkpath(QFileInfo(Telemetry::defaultPath()).absolutePath());
#ifdef Q_OS_WIN
    const auto guiSocket = Telemetry::socketPath(Telemetry::defaultPath() + ".dashboard");
#else
    const auto guiSocket = QFileInfo(Telemetry::defaultPath()).dir().filePath("dashboard.sock");
#endif
    if (!capture && !demo) {
        QLocalSocket existing; existing.connectToServer(guiSocket);
        if (existing.waitForConnected(250)) {
            existing.write(QJsonDocument(QJsonObject{{"tab",tabIndex}}).toJson(QJsonDocument::Compact) + '\n');
            existing.waitForBytesWritten(250); return 0;
        }
    }
    QLockFile lock(Telemetry::defaultPath() + ".window.lock");
    if (!capture && !demo && !lock.tryLock(0)) { fputs("Another ABM window is starting.\n",stderr); return 1; }
    std::unique_ptr<Telemetry> ownedReceiver;
    QString receiverError;
    if (!demo) {
        QLocalSocket probe; probe.connectToServer(Telemetry::socketPath(Telemetry::defaultPath()));
        if (!probe.waitForConnected(250)) {
            ownedReceiver = std::make_unique<Telemetry>(Telemetry::defaultPath());
            if (!ownedReceiver->start(receiverPort)) { receiverError = ownedReceiver->errorString(); ownedReceiver.reset(); }
        }
    }
    QQuickStyle::setStyle("Basic");
    Translator translator; app.installTranslator(&translator);
    Theme theme(capture ? QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/beamside-preview-theme" : QString());
    Backend backend;
    QQmlApplicationEngine engine;
    auto *previews = new Previews; engine.addImageProvider("vehicles",previews); backend.setPreviews(previews);
    if (demo) backend.setDemo(args.contains("--dev-open") && args.value(args.indexOf("--dev-open") + 1) == "blocked");
    else if (!receiverError.isEmpty()) backend.launcher()->setMessage(QObject::tr("Cannot start the receiver: %1").arg(receiverError));
    else if (!backend.launcher()->userFolder().isEmpty()) backend.launcher()->installBridge();
    QObject::connect(&translator,&Translator::changed,&engine,&QQmlEngine::retranslate);
    engine.rootContext()->setContextProperty("translator",&translator);
    engine.rootContext()->setContextProperty("theme",&theme);
    engine.rootContext()->setContextProperty("backend",&backend);
    engine.rootContext()->setContextProperty("demoMode",demo);
    const QColor accentColor{theme.accent()};
    QPalette palette; // Omarchy-dark, compact desktop palette; Basic-style controls draw from it.
    palette.setColor(QPalette::Window, QColor("#14161b"));
    palette.setColor(QPalette::Base, QColor("#101116"));
    palette.setColor(QPalette::AlternateBase, QColor("#191b21"));
    palette.setColor(QPalette::Button, QColor("#1e2026"));
    palette.setColor(QPalette::ToolTipBase, QColor("#1c1e24"));
    palette.setColor(QPalette::ToolTipText, QColor("#e8e8ed"));
    palette.setColor(QPalette::WindowText, QColor("#e8e8ed"));
    palette.setColor(QPalette::Text, QColor("#e8e8ed"));
    palette.setColor(QPalette::ButtonText, QColor("#e8e8ed"));
    palette.setColor(QPalette::PlaceholderText, QColor("#9b9ca9"));
    palette.setColor(QPalette::Highlight, accentColor.isValid() ? accentColor : QColor("#FFD60A"));
    palette.setColor(QPalette::HighlightedText, QColor("#0d0e12"));
    palette.setColor(QPalette::Link, accentColor.isValid() ? accentColor : QColor("#FFD60A"));
    const QColor disabledText("#676975");
    for (const QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText, QPalette::PlaceholderText})
        palette.setBrush(QPalette::Disabled, role, disabledText);
    app.setPalette(palette);
    engine.load(QUrl("qrc:/Main.qml")); if (engine.rootObjects().isEmpty()) return 1;
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    window->setProperty("page",tabIndex);
    if (capture && demo && args.contains("--dev-open") && args.indexOf("--dev-open")+1 < args.size()) {
        // Development aid: --dev-open TARGET pre-opens dialogs and menus for screenshots.
        const auto target = args.value(args.indexOf("--dev-open")+1);
        QTimer::singleShot(350, window, [window, target] { window->setProperty("devOpen",target); });
    }
    if (capture && demo && args.contains("--pick") && args.indexOf("--pick")+1 < args.size()) {
        // Development aid: --pick MODEL_ID opens a model's configurations for screenshots.
        const auto id = args.value(args.indexOf("--pick")+1);
        backend.requestCatalog();
        QTimer::singleShot(120, window, [&backend, window, id] {
            for (const auto &item : backend.catalog()) {
                auto entry = item.toMap();
                if (entry.value("id").toString() == id) {
                    backend.requestConfigs(id);
                    window->setProperty("selectedModel", entry);
                    break;
                }
            }
        });
    }
    QLocalServer activation; activation.setSocketOptions(QLocalServer::UserAccessOption);
    if (!capture && !demo) {
        QLocalServer::removeServer(guiSocket);
        if (!activation.listen(guiSocket)) { fputs("Cannot create activation socket.\n",stderr); return 1; }
        QObject::connect(&activation,&QLocalServer::newConnection,&app,[&] {
            while (auto *client = activation.nextPendingConnection()) {
                auto activate = [window,client] {
                    if (!client->canReadLine()) return;
                    const auto bytes = client->readLine(1024);
                    const int page = QJsonDocument::fromJson(bytes).object().value("tab").toInt(-1);
                    if (page >= 0 && page <= 3) window->setProperty("page",page);
                    window->showNormal(); window->raise(); window->requestActivate(); client->disconnectFromServer();
                };
                QObject::connect(client,&QLocalSocket::readyRead,&app,activate);
                QObject::connect(client,&QLocalSocket::disconnected,client,&QObject::deleteLater);
                QTimer::singleShot(1000,client,[client] { client->abort(); client->deleteLater(); });
                activate();
            }
        });
    }
    if (capture) QTimer::singleShot(1800,&app,[&] { app.exit(window->grabWindow().save(args[args.indexOf("--screenshot")+1]) ? 0 : 1); });
    return app.exec();
}
