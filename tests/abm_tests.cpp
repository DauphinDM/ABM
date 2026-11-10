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
#include <QQuickStyle>
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
        auto initial=QJsonDocument::fromJson(packet()).object(); initial.insert("multiplayer",false);
        game.writeDatagram(QJsonDocument(initial).toJson(QJsonDocument::Compact),QHostAddress::LocalHost,port); return true;
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
    void initTestCase() {
        // Exercise the same customizable controls as the app on every platform.
        QQuickStyle::setStyle("Basic");
    }
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
    void vehicleCapabilitiesAreStrictAndBounded() {
        auto input=QJsonDocument::fromJson(packet()).object();
        input.insert("capabilities",QJsonObject{{"power",true},{"gears",false},{"engine","yes"},{"runLua",true}});
        input.insert("vehicleOwned",false);
        auto out=Telemetry::sanitize(QJsonDocument(input).toJson());
        QCOMPARE(out.value("capabilities").toObject(),(QJsonObject{{"power",true},{"gears",false}}));
        QVERIFY(!out.value("vehicleOwned").toBool());
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
        // Power has no artificial ceiling. Non-finite and non-positive fail.
        QVERIFY(Protocol::enginePower(QJsonValue(1.0)) && Protocol::enginePower(QJsonValue(0.5)) && Protocol::enginePower(QJsonValue(3.0)));
        QVERIFY(Protocol::enginePower(QJsonValue(0.01)) && Protocol::enginePower(QJsonValue(1e100)));
        QVERIFY(!Protocol::enginePower(QJsonValue(0)) && !Protocol::enginePower(QJsonValue(-1)));
        QVERIFY(Protocol::gravity(QJsonValue(-9.81)) && Protocol::gravity(QJsonValue(0.0)) && Protocol::gravity(QJsonValue(-30.0)));
        QVERIFY(!Protocol::gravity(QJsonValue(-30.01)) && !Protocol::gravity(QJsonValue(0.01)) && !Protocol::gravity(QJsonValue(9)));
        QVERIFY(!Protocol::enginePower(QJsonValue("1")) && !Protocol::gravity(QJsonValue(true)));
        QVERIFY(Protocol::handling({{"tireGrip",2.0},{"slidingGrip",0.0},{"brakePower",10.0},{"steeringResponse",.1}}));
        QVERIFY(!Protocol::handling({{"tireGrip",11.0},{"slidingGrip",1.0},{"brakePower",1.0},{"steeringResponse",1.0}}));
        QJsonArray ratios; for (int i=0;i<200;++i) ratios.append(.5);
        QVERIFY(Protocol::gears(ratios)); QVERIFY(!Protocol::gears(QJsonArray{0}));
        QVERIFY(!Protocol::gears(QJsonArray{"1"})); QVERIFY(!Protocol::gears(QJsonArray{}));
        QVERIFY(Protocol::engine({{"engineStrength",.1},{"engineIndestructible",true}}));
        QVERIFY(Protocol::engine({{"engineStrength",100.0},{"engineIndestructible",false}}));
        QVERIFY(!Protocol::engine({{"engineStrength",100.1},{"engineIndestructible",false}}));
        QVERIFY(!Protocol::engine({{"engineStrength",1.0},{"engineIndestructible","yes"}}));
        QVERIFY(Protocol::dimensions({{"width",.5},{"length",2.0},{"height",1.0}}));
        QVERIFY(!Protocol::dimensions({{"width",0.0},{"length",1.0},{"height",1.0}}));
        QVERIFY(!Protocol::dimensions({{"width",1.0},{"length",2.01},{"height",1.0}}));
        QVERIFY(!Protocol::dimensions({{"width","1"},{"length",1.0},{"height",1.0}}));
    }
    void tuningRoundTripAndBusyRequest() {
        Fixture f; QVERIFY(f.start());
        auto state=QJsonDocument::fromJson(packet()).object(); state.insert("multiplayer",false); state.insert("bridgeVersion","0.4.0"); f.send(state);
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
    void sandboxCommandsRequireNewBridgeAndStop() {
        Fixture f; QVERIFY(f.start());
        auto state=QJsonDocument::fromJson(packet()).object(); state.insert("multiplayer",false); state.insert("bridgeVersion","0.4.0"); f.send(state);
        Backend backend(f.path); QTRY_VERIFY(backend.connected()); QTest::qWait(100);
        backend.carAction("dealer"); QTRY_VERIFY(!backend.busy());
        QVERIFY(backend.actionMessage().contains("0.5")); QVERIFY(!f.game.hasPendingDatagrams());
        state.insert("bridgeVersion","0.6.1"); f.send(state);
        QTRY_COMPARE(backend.state().value("bridgeVersion").toString(),QString("0.6.1"));
        backend.applyGears("3,2,1"); QTRY_VERIFY(!backend.busy());
        QVERIFY(backend.actionMessage().contains("0.6.2")); QVERIFY(!f.game.hasPendingDatagrams());
        state.insert("bridgeVersion","0.6.2"); state.insert("telemetry",QJsonObject{{"speed",0}}); f.send(state);
        QTRY_COMPARE(backend.state().value("bridgeVersion").toString(),QString("0.6.2"));
        backend.applyGears("3.5, 2.1 1 .8"); QTRY_VERIFY(f.game.hasPendingDatagrams());
        auto command=f.command(); QCOMPARE(command.value("ratios").toArray().size(),4);
        f.send({{"kind","result"},{"requestId",command.value("requestId")},{"ok",true}}); QTRY_VERIFY(!backend.busy());
        state.insert("telemetry",QJsonObject{{"speed",20}}); f.send(state);
        QTRY_COMPARE(backend.state().value("telemetry").toMap().value("speed").toDouble(),20.0);
        backend.applyGears("3,2,1"); QTRY_VERIFY(!backend.busy()); QVERIFY(!f.game.hasPendingDatagrams());
        backend.applyGears("3, nope"); QVERIFY(!backend.busy()); QVERIFY(backend.actionMessage().contains("numeric"));
    }
    void engineCommandsRequireNewBridge() {
        Fixture f; QVERIFY(f.start());
        auto state=QJsonDocument::fromJson(packet()).object(); state.insert("multiplayer",false); state.insert("bridgeVersion","0.5.0"); f.send(state);
        Backend backend(f.path); QTRY_VERIFY(backend.connected()); QTest::qWait(100);
        backend.applyEngine({{"engineStrength",2.0},{"engineIndestructible",true}});
        QTRY_VERIFY(!backend.busy()); QVERIFY(backend.actionMessage().contains("0.6")); QVERIFY(!f.game.hasPendingDatagrams());
        state.insert("bridgeVersion","0.6.0"); f.send(state);
        QTRY_COMPARE(backend.state().value("bridgeVersion").toString(),QString("0.6.0"));
        backend.applyEngine({{"engineStrength",2.0},{"engineIndestructible",true},{"code","ignored"}});
        QTRY_VERIFY(f.game.hasPendingDatagrams()); const auto command=f.command();
        QCOMPARE(command.value("action").toString(),QString("engine"));
        QCOMPARE(command.value("engineStrength").toDouble(),2.0);
        QVERIFY(command.value("engineIndestructible").toBool()); QVERIFY(!command.contains("code"));
        f.send({{"kind","result"},{"requestId",command.value("requestId")},{"ok",true},{"message","Engine protection ON"}});
        QTRY_VERIFY(!backend.busy()); QCOMPARE(backend.actionMessage(),QString("Engine protection ON"));
        backend.applyEngine({{"engineStrength",0.0},{"engineIndestructible",true}});
        QVERIFY(!backend.busy()); QVERIFY(!f.game.hasPendingDatagrams());
    }
    void dimensionsAndBounceStopRequire061() {
        Fixture f; QVERIFY(f.start()); Backend backend(f.path); QTRY_VERIFY(backend.connected());
        auto state=QJsonDocument::fromJson(packet()).object(); state.insert("multiplayer",false); state.insert("bridgeVersion","0.6.0"); f.send(state);
        QTRY_COMPARE(backend.state().value("bridgeVersion").toString(),QString("0.6.0"));
        backend.carAction("dealerStop"); QTRY_VERIFY(!backend.busy());
        QVERIFY(backend.actionMessage().contains("0.6.1")); QVERIFY(!f.game.hasPendingDatagrams());
        state.insert("bridgeVersion","0.6.1"); state.insert("multiplayer",false); state.insert("telemetry",QJsonObject{{"speed",0}}); f.send(state);
        QTRY_COMPARE(backend.state().value("bridgeVersion").toString(),QString("0.6.1"));
        backend.applyDimensions({{"width",1.5},{"length",.8},{"height",1.2},{"code","ignored"}});
        QTRY_VERIFY(f.game.hasPendingDatagrams()); auto command=f.command();
        QCOMPARE(command.value("action").toString(),QString("dimensions")); QCOMPARE(command.value("width").toDouble(),1.5); QVERIFY(!command.contains("code"));
        f.send({{"kind","result"},{"requestId",command.value("requestId")},{"ok",true}}); QTRY_VERIFY(!backend.busy());
        backend.carAction("dealerStop"); QTRY_VERIFY(f.game.hasPendingDatagrams()); command=f.command();
        QCOMPARE(command.value("action").toString(),QString("dealerStop"));
        f.send({{"kind","result"},{"requestId",command.value("requestId")},{"ok",true}}); QTRY_VERIFY(!backend.busy());
        state.insert("multiplayer",true); f.send(state); QTRY_VERIFY(backend.state().value("multiplayer").toBool());
        backend.applyDimensions({{"width",1.0},{"length",1.0},{"height",1.0}}); QTRY_VERIFY(!backend.busy()); QVERIFY(!f.game.hasPendingDatagrams());
        state.insert("bridgeVersion","0.6.2"); f.send(state);
        QTRY_COMPARE(backend.state().value("bridgeVersion").toString(),QString("0.6.2"));
        backend.applyDimensions({{"width",1.5},{"length",1.0},{"height",1.0}}); QTRY_VERIFY(!backend.busy());
        QVERIFY(backend.actionMessage().contains("0.7")); QVERIFY(!f.game.hasPendingDatagrams());
        state.insert("multiplayer",false); f.send(state); QTRY_VERIFY(!backend.state().value("multiplayer").toBool());
        backend.applyDimensions({{"width",1.5},{"length",1.0},{"height",1.0}}); QTRY_VERIFY(f.game.hasPendingDatagrams()); command=f.command();
        QCOMPARE(command.value("action").toString(),QString("dimensions"));
        f.send({{"kind","result"},{"requestId",command.value("requestId")},{"ok",true}}); QTRY_VERIFY(!backend.busy());
        state.insert("multiplayer",false); state.insert("telemetry",QJsonObject{{"speed",5}}); f.send(state);
        QTRY_COMPARE(backend.state().value("telemetry").toMap().value("speed").toDouble(),5.0);
        backend.applyDimensions({{"width",1.0},{"length",1.0},{"height",1.0}}); QTRY_VERIFY(!backend.busy()); QVERIFY(!f.game.hasPendingDatagrams());
    }
    void livePowerCoalescesAndCancelsOnVehicleReset() {
        Fixture f; QVERIFY(f.start()); Backend backend(f.path); QTRY_VERIFY(backend.connected());
        auto state=QJsonDocument::fromJson(packet()).object();
        state.insert("multiplayer",false); state.insert("bridgeVersion","0.9.0"); state.insert("vehicleId",42); state.insert("vehicleOwned",true);
        state.insert("capabilities",QJsonObject{{"power",true}});
        state.insert("workshop",QJsonObject{{"contract",1},{"generation","car:0"},{"gears",QJsonArray{QJsonObject{{"index",-1},{"ratio",-3.2}},QJsonObject{{"index",1},{"ratio",3.5}}}}});
        f.send(state); QTRY_COMPARE(backend.state().value("bridgeVersion").toString(),QString("0.9.0"));
        backend.setLivePower(1.2); QTRY_VERIFY(f.game.hasPendingDatagrams()); const auto first=f.command();
        QCOMPARE(first.value("action").toString(),QString("power")); QCOMPARE(first.value("targetVehicle").toInt(),42);
        QCOMPARE(first.value("generation").toString(),QString("car:0")); QVERIFY(!first.contains("gravity")); QVERIFY(!first.contains("damageImmunity"));
        backend.setLivePower(2); backend.setLivePower(3); QVERIFY(!f.game.hasPendingDatagrams());
        f.send({{"kind","result"},{"requestId",first.value("requestId")},{"ok",true}});
        QTRY_VERIFY(f.game.hasPendingDatagrams()); const auto latest=f.command();
        QCOMPARE(latest.value("enginePower").toDouble(),3.0); QVERIFY(latest.value("revision").toInt()>first.value("revision").toInt());
        backend.setLivePower(4);
        state.insert("workshop",QJsonObject{{"contract",1},{"generation","car:1"}}); f.send(state);
        QTRY_COMPARE(backend.state().value("workshop").toMap().value("generation").toString(),QString("car:1")); QTRY_VERIFY(!backend.busy());
        f.send({{"kind","result"},{"requestId",latest.value("requestId")},{"ok",true}});
        QTest::qWait(350); QVERIFY(!f.game.hasPendingDatagrams());
        backend.setLivePower(2); QTRY_VERIFY(f.game.hasPendingDatagrams()); const auto discarded=f.command();
        backend.fullReset(); QTRY_VERIFY(f.game.hasPendingDatagrams()); const auto reset=f.command();
        QCOMPARE(reset.value("action").toString(),QString("fullReset"));
        f.send({{"kind","result"},{"requestId",discarded.value("requestId")},{"ok",true}}); QVERIFY(backend.busy());
        f.send({{"kind","result"},{"requestId",reset.value("requestId")},{"ok",true}}); QTRY_VERIFY(!backend.busy());
    }
    void workshopReadbackIsBoundedAndLiteral() {
        auto input=QJsonDocument::fromJson(packet()).object();
        input.insert("workshop",QJsonObject{{"contract",1},{"generation","instance:0"},{"run","bad"},{"enginePower",2},
            {"engines",QJsonArray{QJsonObject{{"name","<b>engine</b>"},{"maxRPM",7200},{"code","bad"}}}},
            {"gears",QJsonArray{QJsonObject{{"index",-1},{"ratio",-3}},QJsonObject{{"index",0},{"ratio",0}},QJsonObject{{"index",1.5},{"ratio",4}}}}});
        const auto workshop=Telemetry::sanitize(QJsonDocument(input).toJson()).value("workshop").toObject();
        QVERIFY(!workshop.contains("run")); QCOMPARE(workshop.value("gears").toArray().size(),2);
        QCOMPARE(workshop.value("engines").toArray()[0].toObject().value("name").toString(),QString("<b>engine</b>"));
        QVERIFY(!workshop.value("engines").toArray()[0].toObject().contains("code"));
    }
    void rpmClutchAndPersistenceUseBoundTargets() {
        QVERIFY(Protocol::rpm(7200)); QVERIFY(!Protocol::rpm(7200.5)); QVERIFY(!Protocol::rpm("7200")); QVERIFY(!Protocol::rpm(30001));
        Fixture f; QVERIFY(f.start()); Backend backend(f.path); QTRY_VERIFY(backend.connected());
        auto state=QJsonDocument::fromJson(packet()).object(); state.insert("multiplayer",false);
        state.insert("bridgeVersion","0.9.0"); state.insert("vehicleId",42);
        state.insert("workshop",QJsonObject{{"contract",1},{"generation","engine:0"}});
        f.send(state); QTRY_COMPARE(backend.state().value("bridgeVersion").toString(),QString("0.9.0"));
        backend.setMaxRPM(6000); QTRY_VERIFY(!backend.busy()); QVERIFY(backend.actionMessage().contains("0.10.0")); QVERIFY(!f.game.hasPendingDatagrams());
        state.insert("bridgeVersion","0.10.0"); f.send(state); QTRY_COMPARE(backend.state().value("bridgeVersion").toString(),QString("0.10.0"));
        backend.setMaxRPM(6000); QTRY_VERIFY(f.game.hasPendingDatagrams()); auto command=f.command();
        QCOMPARE(command.value("action").toString(),QString("rpm")); QCOMPARE(command.value("maxRPM").toInt(),6000);
        QCOMPARE(command.value("generation").toString(),QString("engine:0")); QVERIFY(!command.contains("gravity"));
        f.send({{"kind","result"},{"requestId",command.value("requestId")},{"ok",true}}); QTRY_VERIFY(!backend.busy());
        backend.setClutchProtection(true); QTRY_VERIFY(f.game.hasPendingDatagrams()); command=f.command();
        QCOMPARE(command.value("action").toString(),QString("clutch")); QVERIFY(command.value("clutchIndestructible").toBool());
        f.send({{"kind","result"},{"requestId",command.value("requestId")},{"ok",true}}); QTRY_VERIFY(!backend.busy());
        backend.setResetPersistence(false); QTRY_VERIFY(f.game.hasPendingDatagrams()); command=f.command();
        QCOMPARE(command.value("action").toString(),QString("persistence")); QCOMPARE(command.value("enabled"),QJsonValue(false));
    }
    void preferencesStayInTestDirectory() {
        QTemporaryDir dir; Backend first(dir.filePath("state.json"));
        first.setMetric(false); first.toggleFavorite("etk800"); first.toggleFavorite("etk800/Racely (M)"); first.toggleFavorite("../bad");
        Backend second(dir.filePath("state.json")); QVERIFY(!second.metric()); QCOMPARE(second.favorites().size(),2);
        second.toggleFavorite("etk800"); QCOMPARE(second.favorites(),QStringList{"etk800/Racely (M)"});
    }
    void officialServerRosterDecisions() {
        const QString name="Official BeamMP Server | OGC TWO (02) | US-East | 2 Vehicle | Modded";
        QJsonObject server{{"serverIp","15.204.242.33"},{"serverPort",30814},{"serverRaw",name}};
        QJsonObject row{{"ip","15.204.242.33"},{"port",30814},{"sname",name},{"official",true}};
        auto decide=[&]{return ServerPolicy::evaluate(server,QJsonArray{row});};
        QVERIFY(decide().value("allowed").toBool());
        QVERIFY(!ServerPolicy::evaluate(server,{}).value("allowed").toBool());
        row.insert("sname",name+" event"); server.insert("serverRaw",row.value("sname"));
        QVERIFY(!decide().value("allowed").toBool());
        server.insert("serverRaw",name); row.insert("sname",name); row.insert("port",30815); server.insert("serverPort",30815);
        QVERIFY(decide().value("allowed").toBool()); // Rotated port, approved name.
        row.insert("ip","203.0.113.7"); server.insert("serverIp","203.0.113.7");
        QVERIFY(decide().value("allowed").toBool()); // Rotated host, approved name.
        row.insert("sname","New official event"); server.insert("serverRaw","New official event");
        QVERIFY(!decide().value("allowed").toBool());
        row.insert("official",false); QVERIFY(decide().value("allowed").toBool());
        row.insert("official","false"); QVERIFY(!decide().value("allowed").toBool());
        row.insert("official",false);
        QVERIFY(!ServerPolicy::evaluate(server,QJsonArray{row,row}).value("allowed").toBool());
        row={{"ip","15.204.242.33"},{"port",30816},{"sname","Official BeamMP x Moza - Qualifying Server [US-East]"},{"official",true}};
        server={{"serverIp",row.value("ip")},{"serverPort",row.value("port")},{"serverRaw",row.value("sname")}};
        QVERIFY(!decide().value("allowed").toBool());
        row.insert("official",false); QVERIFY(!decide().value("allowed").toBool());
        row.insert("ip","203.0.113.8"); server.insert("serverIp",row.value("ip"));
        row.insert("port",31000); server.insert("serverPort",31000);
        QVERIFY(!decide().value("allowed").toBool()); // Excluded event name follows rotations too.
        // A former official address does not constrain a different community server.
        row.insert("ip","15.204.242.33"); server.insert("serverIp",row.value("ip"));
        row.insert("port",30816); server.insert("serverPort",30816);
        row.insert("sname","Community on a reused address"); server.insert("serverRaw",row.value("sname"));
        QVERIFY(decide().value("allowed").toBool());
        row.insert("sname",name.toLower()); server.insert("serverRaw",row.value("sname")); row.insert("official",true);
        QVERIFY(!decide().value("allowed").toBool()); // Exact, case-sensitive matching.
        row.insert("sname",name+" "); server.insert("serverRaw",row.value("sname"));
        QVERIFY(!decide().value("allowed").toBool());
        QFile roster(":/bridge/official-servers.json"); QVERIFY(roster.open(QIODevice::ReadOnly));
        const auto data=QJsonDocument::fromJson(roster.readAll()).object(); int count=0;
        const auto groups=data.value("groups").toObject(); for (const auto &group:groups) count+=group.toArray().size();
        QCOMPARE(count,108); QCOMPARE(data.value("excluded").toArray().size(),4);
    }
    void multiplayerCannotSelfAuthorizeViaTelemetry() {
        Fixture f; QVERIFY(f.start()); Backend backend(f.path); QTRY_VERIFY(backend.connected());
        auto state=QJsonDocument::fromJson(packet()).object();
        state.insert("bridgeVersion","0.6.2"); state.insert("sandboxAllowed",true); f.send(state);
        QTRY_COMPARE(backend.state().value("bridgeVersion").toString(),QString("0.6.2"));
        backend.applyTuning({{"enginePower",2.0},{"gravity",-9.81},{"damageImmunity",false}}); QTRY_VERIFY(!backend.busy());
        QVERIFY(backend.actionMessage().contains("0.7")); QVERIFY(!f.game.hasPendingDatagrams());
        state.insert("bridgeVersion","0.7.0"); f.send(state);
        QTRY_COMPARE(backend.state().value("bridgeVersion").toString(),QString("0.7.0"));
        backend.applyTuning({{"enginePower",2.0},{"gravity",-9.81},{"damageImmunity",false}}); QTRY_VERIFY(!backend.busy());
        QVERIFY(backend.actionMessage().contains("0.7.1")); QVERIFY(!f.game.hasPendingDatagrams());
        state.insert("bridgeVersion","0.7.1"); f.send(state);
        QTRY_COMPARE(backend.state().value("bridgeVersion").toString(),QString("0.7.1"));
        QVERIFY(!backend.state().value("sandboxAllowed").toBool());
        backend.applyTuning({{"enginePower",2.0},{"gravity",-9.81},{"damageImmunity",false}}); QTRY_VERIFY(!backend.busy());
        QVERIFY(backend.actionMessage().contains("unverified",Qt::CaseInsensitive));
        while(f.game.hasPendingDatagrams()) QCOMPARE(f.command().value("action").toString(),QString("serverPolicy"));
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
            {"items",QJsonArray{QJsonObject{{"id","etk800"},{"name","800"},{"facets",QJsonObject{{"type",QJsonArray{"Car","Car",42}},{"source",QJsonArray{"Custom"}},{"unknown",QJsonArray{"secret"}}}}},QJsonObject{{"id","../bad"}}}}});
        QTRY_COMPARE(backend.catalog().size(),1); QVERIFY(!backend.busy());
        const auto facets=backend.catalog()[0].toMap().value("facets").toMap();
        QCOMPARE(facets.value("type").toList(),QVariantList{"Car"}); QVERIFY(!facets.contains("unknown"));
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
        QFile file(path); QVERIFY(file.open(QIODevice::ReadOnly)); QVERIFY(file.readAll().contains("0.10.0")); file.close();
        QVERIFY(QFileInfo::exists(dir.filePath("lua/vehicle/extensions/abmTuning.lua")));
        QVERIFY(QFileInfo::exists(dir.filePath("lua/ge/extensions/abm-official-servers.json")));
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
        auto *grid=window->findChild<QQuickItem *>("garageGrid"); QVERIFY(grid); QCOMPARE(grid->property("count").toInt(),5);
        window->setProperty("modelFacets",QVariantMap{{"type","Truck"}}); QTRY_COMPARE(grid->property("count").toInt(),3);
        window->setProperty("modelFilter","mods"); QTRY_COMPARE(grid->property("count").toInt(),1);
        window->setProperty("modelFacets",QVariantMap{}); window->setProperty("modelFilter","custom"); QTRY_COMPARE(grid->property("count").toInt(),1);
        window->setProperty("modelFilter","all"); QTRY_COMPARE(grid->property("count").toInt(),5);
        QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,grid->mapToScene(QPointF(60,50)).toPoint());
        QTRY_COMPARE(backend.configs().size(),2); QTRY_COMPARE(grid->property("count").toInt(),2);
        QVERIFY(!window->property("selectedModel").toMap().value("id").toString().isEmpty());
        auto *search=window->findChild<QQuickItem *>("garageSearch"); QVERIFY(search); search->forceActiveFocus();
        QTest::keyClick(window,Qt::Key_U); QCOMPARE(search->property("text").toString(),QString("u")); QVERIFY(!window->property("metric").toBool());
        QTest::keyClick(window,Qt::Key_Escape); QTRY_COMPARE(search->property("text").toString(),QString());
        QTest::keyClick(window,Qt::Key_Escape); QTRY_VERIFY(window->property("selectedModel").toMap().isEmpty());
        window->setProperty("page",1);
        auto *rpm=window->findChild<QQuickItem *>("rpmInput"); auto *rpmApply=window->findChild<QQuickItem *>("rpmApplyButton");
        QVERIFY(rpm && rpmApply && rpm->isEnabled()); QVERIFY(!rpmApply->isEnabled());
        rpm->setProperty("text","6000"); QTRY_VERIFY(rpmApply->isEnabled());
        rpm->setProperty("text","9000"); QTRY_VERIFY(!rpmApply->isEnabled());
        QVERIFY(window->findChild<QQuickItem *>("clutchToggle")->isEnabled());
        auto *section=window->findChild<QQuickItem *>("carSection"); QVERIFY(section); QCOMPARE(section->property("count").toInt(),4);
        auto *engineToggle=window->findChild<QQuickItem *>("engineToggle"); auto *strength=window->findChild<QQuickItem *>("strengthSlider");
        QVERIFY(engineToggle && strength); QVERIFY(strength->isEnabled()); engineToggle->setProperty("checked",true); QTRY_VERIFY(!strength->isEnabled());
        section->setProperty("currentIndex",1);
        auto *ratios=window->findChild<QQuickItem *>("ratiosInput"); auto *add=window->findChild<QQuickItem *>("addGearButton");
        auto *apply=window->findChild<QQuickItem *>("gearsApplyButton"); QVERIFY(ratios && add && apply);
        QVERIFY(add->isEnabled()); QVERIFY(!apply->isEnabled()); // Demo is moving.
        QCOMPARE(ratios->property("text").toString(),QString("3.5, 2.1, 1.4, 1, 0.8"));
        QVERIFY(QMetaObject::invokeMethod(ratios,"addGear"));
        QVERIFY(ratios->property("text").toString().endsWith("0.5714")); // Median of the last three valid retention factors.
        ratios->setProperty("text","bad, ratios"); QTRY_VERIFY(!add->isEnabled()); QVERIFY(!apply->isEnabled());
        section->setProperty("currentIndex",2); auto *repair=window->findChild<QQuickItem *>("repairButton"); QVERIFY(repair && !repair->isEnabled());
        backend.setDemo(true); QTRY_VERIFY(!engineToggle->isEnabled());
        auto *stop=window->findChild<QQuickItem *>("dealerStopButton"); QVERIFY(stop && stop->isEnabled());
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
