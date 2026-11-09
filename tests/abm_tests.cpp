#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkDatagram>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>
#include <QTemporaryDir>
#include <QBuffer>
#include <QtTest>
#include <QJsonValue>
#include "protocol.h"
#include <archive.h>
#include <archive_entry.h>
#include "backend.h"
#include "telemetry.h"
#include "theme.h"
#include "translator.h"

namespace {
QByteArray packet(const QString &session = "test") {
    return QJsonDocument(QJsonObject{{"schema",1},{"source","beamdeck"},{"sessionId",session},
        {"bridgeVersion","0.3.0"},{"controlsAvailable",true},{"multiplayer",true},
        {"server","<img src='https://example.invalid/tracker'>"},
        {"telemetry",QJsonObject{{"speed",0},{"rpm",3200},{"fuel",.5}}},
        {"players",QJsonArray{QJsonObject{{"name","<b>Player</b>"},{"ping",24}}}}}).toJson(QJsonDocument::Compact);
}
struct Fixture {
    QTemporaryDir dir;
    QString path = dir.filePath("state.json");
    Telemetry receiver{path}; QUdpSocket game; quint16 port = 0;
    bool start() {
        QUdpSocket reservation;
        if (!reservation.bind(QHostAddress(QHostAddress::LocalHost),quint16(0))) return false;
        port=reservation.localPort(); reservation.close();
        if (!receiver.start(port) || !game.bind(QHostAddress(QHostAddress::LocalHost),quint16(0))) return false;
        game.writeDatagram(packet(),QHostAddress::LocalHost,port); return true;
    }
    void send(QJsonObject data, QUdpSocket *sender = nullptr) {
        data.insert("source","beamdeck"); data.insert("schema",1);
        if (!data.contains("sessionId")) data.insert("sessionId","test");
        (sender ? sender : &game)->writeDatagram(QJsonDocument(data).toJson(QJsonDocument::Compact),QHostAddress::LocalHost,port);
    }
    QJsonObject command() { return QJsonDocument::fromJson(game.receiveDatagram().data()).object(); }
    void configs(const QJsonObject &command) {
        send({{"kind","configs"},{"requestId",command.value("requestId")},{"model","etk800"},{"chunk",0},{"complete",true},
            {"items",QJsonArray{QJsonObject{{"key","Racely (M)"},{"name","Racely (M)"},{"custom",true},
                {"preview",QJsonObject{{"path","/vehicles/etk800/Racely (M).jpg"}}}}}}});
    }
};
QList<QJsonObject> messages(QLocalSocket &client) {
    QList<QJsonObject> result;
    while (client.canReadLine()) result << QJsonDocument::fromJson(client.readLine()).object();
    return result;
}
}

class AbmTests : public QObject {
    Q_OBJECT
    static void write(const QString &path,const QByteArray &bytes) {
        QDir().mkpath(QFileInfo(path).absolutePath()); QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly)); QCOMPARE(file.write(bytes),bytes.size());
    }
private slots:
    void rejectsInvalidPackets() {
        for (const auto &p : {QByteArray("[]"),QByteArray("null"),QByteArray("{}"),QByteArray("{bad"),QByteArray(40000,'x'),QByteArray(R"({"schema":2,"source":"beamdeck"})")})
            QVERIFY(Telemetry::sanitize(p).isEmpty());
    }
    void validatesBoundsAndIgnoresUnknownKeys() {
        auto input=QJsonDocument::fromJson(packet()).object();
        input.insert("telemetry",QJsonObject{{"speed",-20},{"fuel",5},{"rpm","abc"},{"brake",.5},{"run","bad"}}); input.insert("run","bad");
        const auto out=Telemetry::sanitize(QJsonDocument(input).toJson());
        QCOMPARE(out.value("telemetry").toObject().size(),1); QVERIFY(!out.contains("run"));
    }
    void boundsCollectionsAndStrings() {
        auto input=QJsonDocument::fromJson(packet()).object(); QJsonArray players;
        for(int i=0;i<200;++i) players.append(QJsonObject{{"name",QString(100,'a')}});
        input.insert("players",players); input.insert("server","hello\nserver\u202E");
        const auto out=Telemetry::sanitize(QJsonDocument(input).toJson(QJsonDocument::Compact));
        QCOMPARE(out.value("players").toArray().size(),128);
        QCOMPARE(out.value("players").toArray()[0].toObject().value("name").toString().size(),64);
        QVERIFY(!out.value("server").toString().contains('\n')); QVERIFY(!out.value("server").toString().contains(QChar(0x202e)));
    }
    void markupRemainsLiteralAndNamesClean() {
        const auto out=Telemetry::sanitize(packet());
        QVERIFY(out.value("server").toString().startsWith("<img"));
        QCOMPARE(out.value("players").toArray()[0].toObject().value("name").toString(),"<b>Player</b>");
        QCOMPARE(Telemetry::cleanName("^l^e[Server] ^r Italy"),"[Server] Italy");
        QCOMPARE(Telemetry::cleanName("^1Red ^2Green ^nUnderlined"),"Red Green Underlined");
    }
    void tuningBoundsMatchTheBridge() {
        // The bridge accepts 0.5–3 power, -30–0 gravity, boolean immunity only.
        QVERIFY(Protocol::enginePower(QJsonValue(1.0)) && Protocol::enginePower(QJsonValue(0.5)) && Protocol::enginePower(QJsonValue(3.0)));
        QVERIFY(!Protocol::enginePower(QJsonValue(0.49)) && !Protocol::enginePower(QJsonValue(3.01)) && !Protocol::enginePower(QJsonValue(-1)));
        QVERIFY(Protocol::gravity(QJsonValue(-9.81)) && Protocol::gravity(QJsonValue(0.0)) && Protocol::gravity(QJsonValue(-30.0)));
        QVERIFY(!Protocol::gravity(QJsonValue(-30.01)) && !Protocol::gravity(QJsonValue(0.01)) && !Protocol::gravity(QJsonValue(9)));
        QVERIFY(!Protocol::enginePower(QJsonValue("1")) && !Protocol::gravity(QJsonValue(true)));
    }
    void tuningRoundTripAndBusyRequest() {
        Fixture f; QVERIFY(f.start());
        auto state=QJsonDocument::fromJson(packet()).object(); state.insert("bridgeVersion","0.4.0"); f.send(state);
        Backend backend(f.path); QTRY_VERIFY(backend.connected());
        QTest::qWait(100);
        backend.applyTuning({{"enginePower",1.5},{"gravity",-1.62},{"damageImmunity",true}});
        QTRY_VERIFY(f.game.hasPendingDatagrams()); const auto command=f.command();
        QCOMPARE(command.value("action").toString(),QString("tuning"));
        QCOMPARE(command.value("enginePower").toDouble(),1.5);
        backend.applyTuning({{"enginePower",2.0},{"gravity",0.0},{"damageImmunity",false}});
        f.send({{"kind","result"},{"requestId",command.value("requestId")},{"ok",true},{"message","Queued"}});
        QTRY_VERIFY(!backend.busy()); QCOMPARE(backend.tuning().value("enginePower").toDouble(),1.5);
    }
    void preferencesStayInTestDirectory() {
        QTemporaryDir dir; Backend first(dir.filePath("state.json"));
        first.setMetric(false); first.toggleFavorite("etk800"); first.toggleFavorite("etk800/Racely (M)"); first.toggleFavorite("../bad");
        Backend second(dir.filePath("state.json")); QVERIFY(!second.metric()); QCOMPARE(second.favorites().size(),2);
        second.toggleFavorite("etk800"); QCOMPARE(second.favorites(),QStringList{"etk800/Racely (M)"});
    }
    void demoDoesNotIncludeRuntimeData() {
        QTemporaryDir dir; auto data=QJsonDocument::fromJson(packet()).object();
        data.insert("connected",true); data.insert("updatedAt",QDateTime::currentMSecsSinceEpoch());
        data.insert("telemetry",QJsonObject{{"speed",500}}); write(dir.filePath("state.json"),QJsonDocument(data).toJson());
        Backend backend(dir.filePath("state.json")); backend.setDemo();
        QCOMPARE(backend.history().size(),300); QCOMPARE(backend.activeVehicles().size(),1);
        QCOMPARE(backend.activeVehicles().first().toMap().value("name").toString(),QString("ETK 800 Series"));
        for(const auto &entry:backend.history()) QVERIFY(entry.toMap().value("speed").toDouble()<40);
    }
    void singlePlayerClearsMultiplayer() {
        auto data=QJsonDocument::fromJson(packet()).object(); data.insert("multiplayer",false);
        const auto out=Telemetry::sanitize(QJsonDocument(data).toJson());
        QVERIFY(!out.contains("server")); QVERIFY(!out.contains("players")); QVERIFY(!out.contains("ping"));
    }
    void receiverAndStaleness() {
        Fixture f; QVERIFY(f.start()); Backend backend(f.path);
        QTRY_VERIFY(backend.connected()); QCOMPARE(backend.state().value("telemetry").toMap().value("rpm").toInt(),3200);
        QTRY_VERIFY_WITH_TIMEOUT(!backend.connected(),4500); QVERIFY(!backend.state().contains("telemetry"));
    }
    void catalogAndInvalidGatewayRequests() {
        Fixture f; QVERIFY(f.start()); Backend backend(f.path); QTRY_VERIFY(backend.connected());
        backend.requestCatalog(); QTRY_VERIFY(f.game.hasPendingDatagrams()); const auto command=f.command();
        f.send({{"kind","catalog"},{"requestId",command.value("requestId")},{"chunk",0},{"complete",true},
            {"items",QJsonArray{QJsonObject{{"id","etk800"},{"name","800"}},QJsonObject{{"id","../bad"}}}}});
        QTRY_COMPARE(backend.catalog().size(),1); QVERIFY(!backend.busy());
        backend.requestConfigs("../bad"); QVERIFY(!backend.busy());
        QLocalSocket client; client.connectToServer(Telemetry::socketPath(f.path)); QTRY_VERIFY(client.canReadLine()); messages(client);
        client.write("{\"action\":\"configs\",\"requestId\":\"bad\",\"model\":\"../bad\"}\n");
        QJsonObject reply;
        QTRY_VERIFY(([&]{for(const auto &m:messages(client)) if(m.value("requestId")=="bad") reply=m; return !reply.isEmpty();})());
        QVERIFY(!reply.value("ok").toBool()); QVERIFY(!f.game.hasPendingDatagrams());
        backend.resetTrip(); QTRY_VERIFY(!backend.busy());
    }
    void configsPopulateAndExactSelectionRoundTrip() {
        Fixture f; QVERIFY(f.start()); Backend backend(f.path); QTRY_VERIFY(backend.connected());
        backend.requestConfigs("etk800"); QTRY_VERIFY(f.game.hasPendingDatagrams()); const auto command=f.command();
        f.configs(command); QTRY_COMPARE(backend.configs().size(),1); QVERIFY(!backend.busy());
        QCOMPARE(backend.configsModel(),"etk800");
        QCOMPARE(backend.configs()[0].toMap().value("id").toString(),"etk800/Racely (M)");
        backend.requestVehicle("replace","etk800/Racely (M)"); QTRY_VERIFY(f.game.hasPendingDatagrams());
        const auto spawn=f.command(); QCOMPARE(spawn.value("model").toString(),"etk800"); QCOMPARE(spawn.value("config").toString(),"Racely (M)");
        f.send({{"kind","result"},{"requestId",spawn.value("requestId")},{"ok",true},{"message","Submitted"}});
        QTRY_VERIFY(!backend.busy()); QCOMPARE(backend.actionMessage(),"Submitted");
        backend.requestVehicle("spawn","etk800/Unknown"); QVERIFY(!backend.busy()); QVERIFY(!f.game.hasPendingDatagrams());
    }
    void chunkedConfigsAndSessionChange() {
        Fixture f; QVERIFY(f.start()); Backend backend(f.path); QTRY_VERIFY(backend.connected());
        backend.requestConfigs("etk800"); QTRY_VERIFY(f.game.hasPendingDatagrams()); const auto command=f.command();
        QJsonObject chunk{{"kind","configs"},{"requestId",command.value("requestId")},{"model","etk800"},{"chunk",0},{"complete",false},
            {"items",QJsonArray{QJsonObject{{"key","base"},{"name","Base"}}}}};
        f.send(chunk); QTest::qWait(50); QVERIFY(backend.configs().isEmpty()); QVERIFY(backend.busy());
        chunk.insert("chunk",1); chunk.insert("complete",true); chunk.insert("items",QJsonArray{QJsonObject{{"key","Weekend build"},{"custom",true}}});
        f.send(chunk); QTRY_COMPARE(backend.configs().size(),2);
        f.game.writeDatagram(packet("new-session"),QHostAddress::LocalHost,f.port);
        QTRY_COMPARE(backend.state().value("sessionId").toString(),"new-session"); QVERIFY(backend.configs().isEmpty());
    }
    void rejectsForeignRepliesAndUnsolicitedCatalogs() {
        Fixture f; QVERIFY(f.start()); Backend backend(f.path); QTRY_VERIFY(backend.connected());
        QUdpSocket stranger;
        f.send({{"kind","catalog"},{"requestId","unknown"},{"chunk",0},{"complete",true},{"items",QJsonArray{QJsonObject{{"id","foreign"}}}}},&stranger);
        backend.requestConfigs("etk800"); QTRY_VERIFY(f.game.hasPendingDatagrams()); const auto command=f.command();
        f.send({{"kind","result"},{"requestId",command.value("requestId")},{"ok",true}},&stranger);
        f.send({{"kind","result"},{"requestId",command.value("requestId")},{"ok",true},{"sessionId","old"}});
        QTest::qWait(100); QVERIFY(backend.busy()); QVERIFY(backend.catalog().isEmpty());
        f.configs(command); QTRY_COMPARE(backend.configs().size(),1);
    }
    void resultsOnlyReachRequesterAndNoReplay() {
        Fixture f; QVERIFY(f.start()); Backend backend(f.path); QLocalSocket observer;
        observer.connectToServer(Telemetry::socketPath(f.path)); QTRY_VERIFY(observer.canReadLine()); messages(observer);
        QTRY_VERIFY(backend.connected()); backend.requestConfigs("etk800"); QTRY_VERIFY(f.game.hasPendingDatagrams());
        const auto command=f.command(); f.configs(command); QTRY_COMPARE(backend.configs().size(),1);
        for(const auto &m:messages(observer)) QVERIFY(m.value("kind")!="configs" && m.value("kind")!="result");
        f.send({{"kind","result"},{"requestId",command.value("requestId")},{"ok",false},{"message","Replay"}});
        QTest::qWait(50); QCOMPARE(backend.actionMessage(),QString("Configurations loaded."));
    }
    void staleGatewayBlocksDriveCommands() {
        Fixture f; QVERIFY(f.start()); Backend backend(f.path); QLocalSocket client;
        client.connectToServer(Telemetry::socketPath(f.path)); QTRY_VERIFY(client.canReadLine());
        QTRY_VERIFY(backend.connected()); QTRY_VERIFY_WITH_TIMEOUT(!backend.connected(),4500); messages(client);
        client.write("{\"action\":\"spawn\",\"requestId\":\"stale\",\"model\":\"etk800\",\"config\":\"base\"}\n");
        QJsonObject reply;
        QTRY_VERIFY(([&]{for(const auto &m:messages(client)) if(m.value("requestId")=="stale") reply=m; return !reply.isEmpty();})());
        QVERIFY(!reply.value("ok").toBool()); QVERIFY(!f.game.hasPendingDatagrams());
    }
    void backendReportsInvalidAndMissingState() {
        QTemporaryDir dir; Backend backend(dir.filePath("state.json")); QVERIFY(!backend.error().isEmpty());
        write(dir.filePath("state.json"),"{}"); backend.refresh(); QVERIFY(!backend.connected());
        write(dir.filePath("state.json"),R"({"connected":true,"updatedAt":1})"); backend.refresh(); QVERIFY(!backend.connected());
        backend.toggleMonitoring(); QVERIFY(!backend.monitoring()); backend.toggleMonitoring(); QVERIFY(backend.monitoring());
    }
    void launchPlansUseNoLaunchAndWorkingDirectory() {
        QTemporaryDir dir; Integration launcher(dir.filePath("integration.ini"),false);
        write(dir.filePath("beamng.log"),"test");
        write(dir.filePath("mods/db.json"),R"({"mods":{"multiplayerbeammp":{"active":true}}})");
        const auto exe=dir.filePath("BeamMP-Launcher.exe"); write(exe,"test");
        QVERIFY(QFile::setPermissions(exe,QFileDevice::ReadOwner|QFileDevice::WriteOwner|QFileDevice::ExeOwner));
        launcher.setUserFolder(dir.path()); launcher.setLauncherPath(exe);
        launcher.processes=[] {return QStringList{"beamng.drive.x64.exe"};};
        int launches=0; QStringList args; QString working, launchedProgram;
        launcher.execute=[&](const QString &program,const QStringList &arguments,const QString &cwd) {++launches; launchedProgram=program; args=arguments; working=cwd; return true;};
        QVERIFY(launcher.startBeamMP()); QCOMPARE(launches,1); QCOMPARE(launchedProgram,exe); QCOMPARE(args,QStringList{"--no-launch"}); QCOMPARE(working,dir.path());
        QVERIFY(launcher.startBeamMP()); QCOMPARE(launches,1);
    }
    void bridgeInstallPreservesUnrelatedFiles() {
        QTemporaryDir dir; Integration launcher(dir.filePath("integration.ini"),false);
        write(dir.filePath("beamng.log"),"test"); launcher.setUserFolder(dir.path());
        QVERIFY(launcher.installBridge());
        const auto path=dir.filePath("lua/ge/extensions/beamdeck.lua");
        QFile file(path); QVERIFY(file.open(QIODevice::ReadOnly)); QVERIFY(file.readAll().contains("0.4.0")); file.close();
        write(path,"User-owned unrelated extension"); QVERIFY(!launcher.installBridge());
        QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(),QByteArray("User-owned unrelated extension"));
    }
    void previewsReadRealLooseAndZipImagesSafely() {
        QTemporaryDir dir; QDir().mkpath(dir.filePath("vehicles/etk800"));
        QImage photo(80,50,QImage::Format_RGB32); photo.fill(Qt::red);
        const auto path=dir.filePath("vehicles/etk800/Custom (M).jpg"); QVERIFY(photo.save(path));
        Previews provider; QSize size;
        const auto url=provider.registerImage({{"path","/vehicles/etk800/Custom (M).jpg"},{"real",path}},{dir.path()});
        QVERIFY(!provider.requestImage(url.section('/',-1),&size,QSize(40,40)).isNull()); QCOMPARE(size,QSize(80,50));
        QByteArray data; QBuffer buffer(&data); buffer.open(QIODevice::WriteOnly); QVERIFY(photo.save(&buffer,"JPG"));
        const auto zip=dir.filePath("car.zip"); auto *writer=archive_write_new(); archive_write_set_format_zip(writer);
        QCOMPARE(archive_write_open_filename(writer,QFile::encodeName(zip).constData()),ARCHIVE_OK);
        auto *entry=archive_entry_new(); archive_entry_set_pathname(entry,"vehicles/etk800/default.jpg"); archive_entry_set_size(entry,data.size()); archive_entry_set_filetype(entry,AE_IFREG); archive_entry_set_perm(entry,0644);
        QCOMPARE(archive_write_header(writer,entry),ARCHIVE_OK); QCOMPARE(archive_write_data(writer,data.data(),data.size()),data.size());
        archive_entry_free(entry); archive_write_close(writer); archive_write_free(writer);
        const auto zipped=provider.registerImage({{"path","/vehicles/etk800/default.jpg"},{"archive",zip}},{dir.path()});
        QVERIFY(!provider.requestImage(zipped.section('/',-1),&size,QSize(40,40)).isNull());
        const auto denied=provider.registerImage({{"path","/vehicles/etk800/../../secret.jpg"},{"real",path}},{dir.path()});
        QVERIFY(provider.requestImage(denied.section('/',-1),&size,QSize(40,40)).isNull());
        const auto outside=provider.registerImage({{"path","/vehicles/etk800/no.jpg"},{"real",path}},{dir.filePath("other")});
        QVERIFY(provider.requestImage(outside.section('/',-1),&size,QSize(40,40)).isNull());
    }
    void themeAndTranslatorReload() {
        QTemporaryDir dir; QDir().mkpath(dir.filePath("theme")); Theme theme(dir.path()); QCOMPARE(theme.accent(),QString("#FFD60A"));
        write(dir.filePath("theme/colors.toml"),"accent = \"#123456\"\n"); QTRY_COMPARE(theme.accent(),QString("#123456"));
        write(dir.filePath("pt.json"),R"({"Save":"Guardar","Open":"Abrir"})");
        write(dir.filePath("pt-BR.json"),R"({"Save":"Salvar"})"); Translator translator(dir.filePath("language"),dir.path());
        write(dir.filePath("language"),"pt-BR\n"); QTRY_COMPARE(translator.language(),QString("pt-BR"));
        QCOMPARE(translator.translate("Main","Save"),QString("Salvar")); QCOMPARE(translator.translate("Main","Open"),QString("Abrir"));
    }
    void realWindowAndGarageInteraction() {
        QTemporaryDir dir; Backend backend(dir.filePath("state.json")); backend.setDemo();
        Theme theme(dir.path()); Translator translator(dir.filePath("language")); QCoreApplication::installTranslator(&translator);
        QQmlApplicationEngine engine; QStringList warnings;
        connect(&engine,&QQmlEngine::warnings,this,[&](const QList<QQmlError> &errors){for(const auto &e:errors) warnings<<e.toString();});
        connect(&translator,&Translator::changed,&engine,&QQmlEngine::retranslate);
        engine.rootContext()->setContextProperty("backend",&backend); engine.rootContext()->setContextProperty("theme",&theme);
        engine.rootContext()->setContextProperty("translator",&translator); engine.rootContext()->setContextProperty("demoMode",true);
        engine.load(QUrl("qrc:/Main.qml")); QVERIFY2(!engine.rootObjects().isEmpty(),qPrintable(warnings.join('\n')));
        auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects().first()); QVERIFY(window); QVERIFY(QTest::qWaitForWindowExposed(window));
        window->requestActivate(); QTest::qWait(100); QTest::keyClick(window,Qt::Key_U); QVERIFY(!window->property("metric").toBool());
        window->setProperty("helpVisible",true); QTest::keyClick(window,Qt::Key_Escape); QVERIFY(!window->property("helpVisible").toBool());
        QTest::qWait(1100); QVERIFY(window->property("uiFps").toInt()>0); QVERIFY(window->property("uiFps").toInt()<=65);
        window->setProperty("page",2); QTest::qWait(80);
        auto *grid=window->findChild<QQuickItem *>("garageGrid"); QVERIFY(grid); QCOMPARE(grid->property("count").toInt(),2);
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,grid->mapToScene(QPointF(60,50)).toPoint());
        QTRY_COMPARE(backend.configs().size(),2); QTRY_COMPARE(grid->property("count").toInt(),2);
        QVERIFY(!window->property("selectedModel").toMap().value("id").toString().isEmpty());
        auto *search=window->findChild<QQuickItem *>("garageSearch"); QVERIFY(search); search->forceActiveFocus();
        QTest::keyClick(window,Qt::Key_U); QCOMPARE(search->property("text").toString(),QString("u")); QVERIFY(!window->property("metric").toBool());
        QTest::keyClick(window,Qt::Key_Escape); QTRY_COMPARE(search->property("text").toString(),QString());
        QTest::keyClick(window,Qt::Key_Escape); QTRY_VERIFY(window->property("selectedModel").toMap().isEmpty());
        write(dir.filePath("language"),"da\n"); QTRY_COMPARE(translator.language(),QString("da"));
        for (const auto dimensions : {QSize(380,600),QSize(540,850),QSize(900,850)}) {
            window->resize(dimensions); QTest::qWait(100); QVERIFY(grid->width()>0);
        }
        window->setProperty("page",3); QTest::qWait(100);
        QList<QQuickItem *> pending{window->contentItem()}; int labels=0;
        while(!pending.isEmpty()) { auto *item=pending.takeLast(); pending.append(item->childItems()); if(item->property("abmPlainText").toBool()) {++labels; QCOMPARE(item->property("textFormat").toInt(),0);} }
        QVERIFY(labels>20);
        const auto screenshot=qEnvironmentVariable("BEAMSIDE_TEST_SCREENSHOT");
        if(!screenshot.isEmpty()) QVERIFY(window->grabWindow().save(screenshot));
        QCoreApplication::removeTranslator(&translator);
        QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n')));
    }
};
QTEST_MAIN(AbmTests)
#include "abm_tests.moc"
