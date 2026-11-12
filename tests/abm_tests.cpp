#include <QDateTime>
#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkDatagram>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlComponent>
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
#include "playerprofiles.h"
#include "presetstore.h"
#include "mobilehost.h"
#include "companionfirewall.h"
#include "usbtunnel.h"
#include <QStringConverter>
#include <QProcess>
#include <QStandardPaths>

namespace {
class CablePeer : public QTcpServer {
public:
    QPointer<QSslSocket> phone;
protected:
    void incomingConnection(qintptr descriptor) override {
        phone=new QSslSocket(this); phone->setPeerVerifyMode(QSslSocket::VerifyNone);
        if (!phone->setSocketDescriptor(descriptor)) { phone->deleteLater(); phone.clear(); return; }
        // The real phone originates TLS, even though the PC opens the USB pipe.
        phone->startClientEncryption();
    }
};
class ProfileReply : public QNetworkReply {
public:
    ProfileReply(const QNetworkRequest &request, QObject *parent) : QNetworkReply(parent) {
        setRequest(request); setUrl(request.url()); setOperation(QNetworkAccessManager::GetOperation); open(QIODevice::ReadOnly);
    }
    void complete(const QByteArray &body, int status=200) {
        if (isFinished()) return;
        m_body=body; setAttribute(QNetworkRequest::HttpStatusCodeAttribute,status);
        setFinished(true); emit readyRead(); emit finished();
    }
    void abort() override {
        if (isFinished()) return;
        setError(OperationCanceledError,"Canceled"); setFinished(true); emit finished();
    }
    qint64 bytesAvailable() const override { return m_body.size()+QNetworkReply::bytesAvailable(); }
protected:
    qint64 readData(char *data,qint64 maximum) override {
        const auto size=qMin(maximum,qint64(m_body.size()));
        if (!size) return -1;
        memcpy(data,m_body.constData(),size_t(size)); m_body.remove(0,size); return size;
    }
private:
    QByteArray m_body;
};
class ProfileNetwork : public QNetworkAccessManager {
public:
    QPointer<ProfileReply> reply;
    QNetworkRequest request;
protected:
    QNetworkReply *createRequest(Operation operation,const QNetworkRequest &r,QIODevice *) override {
        Q_ASSERT(operation==GetOperation); request=r; reply=new ProfileReply(r,this); return reply;
    }
};
class BrowserCapture : public QObject {
    Q_OBJECT
public:
    QUrl url;
public slots:
    void open(const QUrl &value) { url=value; }
};
struct BrowserGuard {
    BrowserCapture capture;
    BrowserGuard() { QDesktopServices::setUrlHandler("https",&capture,"open"); }
    ~BrowserGuard() { QDesktopServices::unsetUrlHandler("https"); }
};
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
    static void writeZip(const QString &path, const QByteArray &member, const QByteArray &bytes) {
        auto *writer = archive_write_new(); archive_write_set_format_zip(writer);
        QCOMPARE(archive_write_open_filename(writer,QFile::encodeName(path).constData()),ARCHIVE_OK);
        auto *entry = archive_entry_new(); archive_entry_set_pathname(entry,member.constData());
        archive_entry_set_size(entry,bytes.size()); archive_entry_set_filetype(entry,AE_IFREG); archive_entry_set_perm(entry,0644);
        QCOMPARE(archive_write_header(writer,entry),ARCHIVE_OK);
        QCOMPARE(archive_write_data(writer,bytes.constData(),bytes.size()),bytes.size());
        archive_entry_free(entry); archive_write_close(writer); archive_write_free(writer);
    }
private slots:
    void initTestCase() {
        // Exercise the same customizable controls as the app on every platform.
        QQuickStyle::setStyle("MotorsportGlass");
        QQuickStyle::setFallbackStyle("Basic");
    }
    void rejectsInvalidPackets() {
        for (const auto &p : {QByteArray("[]"),QByteArray("null"),QByteArray("{}"),QByteArray("{bad"),QByteArray(40000,'x'),QByteArray(R"({"schema":2,"source":"beamdeck"})")})
            QVERIFY(Telemetry::sanitize(p).isEmpty());
    }
    void companionFirewallRulesStayOnSelectedPrivateSubnet() {
        using Firewall=CompanionFirewall;
        const auto rule=Firewall::ruleFor("192.168.12.42",23,"enp10s0","/opt/ABM's folder/abm",44445);
        QVERIFY(rule.valid()); QCOMPARE(rule.subnet,QString("192.168.12.0/23"));
        QCOMPARE(Firewall::ufwArguments(rule),(QStringList{"allow","in","on","enp10s0","proto","tcp","from","192.168.12.0/23","to","192.168.12.42","port","44445","comment","ABM mobile companion LAN"}));
        for (const auto &address : {"0.0.0.0","127.0.0.1","8.8.8.8","169.254.1.2","::1","::ffff:192.168.1.2","192.168.001.2"})
            QVERIFY(!Firewall::ruleFor(address,24,"eth0","/app",44445).valid());
        for (int prefix : {-1,0,8,15,31,32})
            QVERIFY(!Firewall::ruleFor("10.1.2.3",prefix,"eth0","/app",44445).valid());
        QVERIFY(Firewall::ruleFor("172.20.1.4",16,"eth0","/app",44445).valid());
        QVERIFY(!Firewall::ruleFor("192.168.1.2",16,"eth0","/app",0).valid());
        QVERIFY(!Firewall::ruleFor("192.168.1.2",16,"eth0","/app",65536).valid());
        auto injected=rule; injected.interfaceName="eth0; touch /tmp/no";
        QVERIFY(Firewall::ufwArguments(injected).isEmpty());
        injected.interfaceName="--force"; QVERIFY(Firewall::ufwArguments(injected).isEmpty());
        const auto script=Firewall::windowsScript(rule);
        QVERIFY(script.contains("-Profile Private")); QVERIFY(script.contains("-Protocol TCP -LocalPort 44445"));
        QVERIFY(script.contains("-LocalAddress '192.168.12.42' -RemoteAddress '192.168.12.0/23'"));
        QVERIFY(script.contains("ABM''s folder")); QVERIFY(!script.contains("-Profile Any"));
        QVERIFY(!script.contains("Set-NetFirewallProfile")); QVERIFY(!script.contains("Remove-NetFirewallRule"));
        QStringDecoder decoder(QStringConverter::Utf16LE);
        QCOMPARE(QString(decoder(QByteArray::fromBase64(Firewall::encodedScript(script).toLatin1()))),script);
#ifdef Q_OS_WIN
        // Parse, but NEVER execute, the administrative script on Windows CI.
        QProcess parser;
        const auto check="$tokens=$null; $errors=$null; [void][System.Management.Automation.Language.Parser]::ParseInput("
            "[Text.Encoding]::Unicode.GetString([Convert]::FromBase64String('"+Firewall::encodedScript(script)+"')),[ref]$tokens,[ref]$errors); if($errors.Count){exit 1}";
        parser.start("powershell.exe",{"-NoProfile","-NonInteractive","-EncodedCommand",Firewall::encodedScript(check)});
        QVERIFY(parser.waitForFinished(10000)); QCOMPARE(parser.exitCode(),0);
#endif
        QTemporaryDir directory; Backend backend(directory.filePath("missing-state.json"));
        MobileHost host(&backend,directory.filePath("mobile")); CompanionFirewall firewall(&host);
        QVERIFY(!firewall.supported()); QVERIFY(!firewall.busy()); QVERIFY(firewall.scope().isEmpty());
        firewall.requestAccess("stale scope"); QVERIFY(!firewall.busy());
        QVERIFY(firewall.message().contains("changed"));
    }
    void connectFirewallRequiresExplicitConfirmation() {
        QTemporaryDir directory; Theme theme(directory.path());
        QQmlEngine engine; engine.rootContext()->setContextProperty("theme",&theme);
        QList<QQmlError> warnings;
        connect(&engine,&QQmlEngine::warnings,this,[&](const auto &values) { warnings.append(values); });
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQuick
            import QtQuick.Controls
            import QtQuick.Layouts
            import "qrc:/"
            ApplicationWindow {
                width: 560; height: 840; visible: true
                property alias requests: fixture.requests
                property alias confirmed: fixture.confirmed
                property alias pane: connectPane
                QtObject {
                    id: fixture
                    property bool supported: true
                    property bool busy: false
                    property string message: "Manual firewall setup"
                    property string scope: "192.168.12.0/24 to 192.168.12.42:44445"
                    property int requests: 0
                    property string confirmed: ""
                    function requestAccess(value) { confirmed=value; requests++ }
                }
                ConnectPane { id: connectPane; anchors.fill: parent; firewall: fixture }
            })",QUrl("qrc:/FirewallFixture.qml"));
        QScopedPointer<QObject> object(component.create()); QVERIFY2(object,qPrintable(component.errorString()));
        auto *window=qobject_cast<QQuickWindow *>(object.data()); QVERIFY(window); QVERIFY(QTest::qWaitForWindowExposed(window));
        auto find=[&](const QString &name) {
            std::function<QQuickItem *(QQuickItem *)> walk=[&](QQuickItem *item)->QQuickItem * {
                if (item->objectName()==name) return item;
                for (auto *child:item->childItems()) if (auto *found=walk(child)) return found;
                return nullptr;
            };
            return walk(window->contentItem());
        };
        auto *button=find("mobileFirewallSetup"); QVERIFY(button); QVERIFY(button->isEnabled());
        QVERIFY(QMetaObject::invokeMethod(button,"clicked")); QTest::qWait(20);
        QObject *dialog=nullptr;
        for (auto *candidate:window->findChildren<QObject *>())
            if (candidate->objectName()=="mobileFirewallConfirm") dialog=candidate;
        QVERIFY(dialog); QVERIFY(dialog->property("visible").toBool()); QCOMPARE(window->property("requests").toInt(),0);
        QVERIFY(QMetaObject::invokeMethod(dialog,"reject")); QCOMPARE(window->property("requests").toInt(),0);
        QVERIFY(QMetaObject::invokeMethod(button,"clicked")); QVERIFY(QMetaObject::invokeMethod(dialog,"accept"));
        QCOMPARE(window->property("requests").toInt(),1);
        QCOMPARE(window->property("confirmed").toString(),QString("192.168.12.0/24 to 192.168.12.42:44445"));
        window->resize(320,440); QVERIFY(QMetaObject::invokeMethod(button,"clicked")); QTest::qWait(50);
        QVERIFY(dialog->property("height").toReal()<=window->height()-24);
        if (qEnvironmentVariableIsSet("BEAMSIDE_FIREWALL_SCREENSHOT"))
            QVERIFY(window->grabWindow().save(qEnvironmentVariable("BEAMSIDE_FIREWALL_SCREENSHOT")));
        QVERIFY(QMetaObject::invokeMethod(dialog,"reject"));
        auto *pane=qvariant_cast<QObject *>(window->property("pane")); QVERIFY(pane); pane->setProperty("demo",true);
        QTRY_VERIFY(!button->isEnabled()); QVERIFY(warnings.isEmpty());
    }
    void mobileHostRequiresTlsApprovalTargetAndRevocation() {
        if (QStandardPaths::findExecutable("openssl").isEmpty() || !QSslSocket::supportsSsl()) QSKIP("Local OpenSSL certificate generation is unavailable");
        Fixture f; QVERIFY(f.start()); Backend backend(f.path);
        auto state = QJsonDocument::fromJson(packet()).object();
        state.insert("multiplayer",false); state.insert("vehicleId",42); state.insert("vehicleOwned",true);
        state.insert("bridgeVersion","0.11.0");
        state.insert("vehicle","Test car"); state.insert("map","test_map");
        state.insert("gameFolder","/private/game/path"); f.send(state);
        QTimer heartbeat; heartbeat.setInterval(100);
        connect(&heartbeat,&QTimer::timeout,this,[&] { f.send(state); }); heartbeat.start();
        QTRY_VERIFY(backend.connected()); QTRY_COMPARE(backend.state().value("vehicleId").toInt(),42);
        MobileHost host(&backend,f.dir.filePath("mobile")); QVERIFY(!host.running());
        QVERIFY(!host.start("0.0.0.0",0)); QVERIFY(!host.start("8.8.8.8",0));
        QVERIFY(host.start("127.0.0.1",0)); QVERIFY(host.running());
        const auto link = host.pairingLink(); QVERIFY(link.startsWith("abm://pair/"));
        const auto invitation = QJsonDocument::fromJson(QByteArray::fromBase64(link.mid(11).toLatin1(),QByteArray::Base64UrlEncoding)).object();
        QCOMPARE(invitation.value("secret").toString().size(),64);
        QSslSocket phone; phone.setPeerVerifyMode(QSslSocket::VerifyNone);
        phone.connectToHostEncrypted("127.0.0.1",quint16(invitation.value("port").toInt())); QTRY_VERIFY(phone.isEncrypted());
        QCOMPARE(QString::fromLatin1(phone.peerCertificate().digest(QCryptographicHash::Sha256).toHex()),invitation.value("sha256").toString());
        auto send = [&](QJsonObject message) { phone.write(QJsonDocument(message).toJson(QJsonDocument::Compact)+'\n'); };
        QString receiptMessage;
        QHash<int,QString> receipts;
        auto received = [&](int seq) {
            while (phone.canReadLine()) {
                const auto message=QJsonDocument::fromJson(phone.readLine()).object();
                if (message.value("kind")=="receipt") receipts.insert(message.value("seq").toInt(),message.value("message").toString());
            }
            receiptMessage=receipts.value(seq); return receipts.contains(seq);
        };
        send({{"kind","pair"},{"v",1},{"secret",invitation.value("secret")},{"device","Test phone"}});
        QTRY_COMPARE(host.pendingDevice(),QString("Test phone")); QVERIFY(host.connectedDevice().isEmpty());
        QVERIFY(!f.game.hasPendingDatagrams()); host.approve();
        QCOMPARE(host.connectedDevice(),QString("Test phone")); QVERIFY(host.pairingLink().isEmpty());
        const auto snapshot = host.snapshot(); QVERIFY(!snapshot.value("state").toObject().contains("gameFolder"));
        QVERIFY(!snapshot.contains("secret"));
        send({{"kind","command"},{"seq",1},{"action","runLua"},{"target",snapshot.value("target")}});
        QTRY_VERIFY(received(1)); QVERIFY(receiptMessage.contains("Unsupported")); QVERIFY(!f.game.hasPendingDatagrams());
        send({{"kind","command"},{"seq",2},{"action","repair"},{"target",QJsonObject{{"vehicle",99}}}});
        QTRY_VERIFY(received(2)); QVERIFY(receiptMessage.contains("changed")); QVERIFY(!f.game.hasPendingDatagrams());
        state.insert("telemetry",QJsonObject{{"speed",12}}); f.send(state);
        QTRY_COMPARE(backend.state().value("telemetry").toMap().value("speed").toDouble(),12);
        send({{"kind","command"},{"seq",3},{"action","repair"},{"target",snapshot.value("target")}});
        QTRY_VERIFY(received(3)); QVERIFY(receiptMessage.contains("stopped")); QVERIFY(!f.game.hasPendingDatagrams());
        state.insert("telemetry",QJsonObject{{"speed",0}}); state.insert("vehicleOwned",false); f.send(state);
        QTRY_VERIFY(!backend.state().value("vehicleOwned").toBool());
        send({{"kind","command"},{"seq",4},{"action","repair"},{"target",snapshot.value("target")}});
        QTRY_VERIFY(received(4)); QVERIFY(receiptMessage.contains("owned")); QVERIFY(!f.game.hasPendingDatagrams());
        state.insert("vehicleOwned",true); state.insert("joining",true); f.send(state);
        QTRY_VERIFY(backend.state().value("joining").toBool());
        send({{"kind","command"},{"seq",5},{"action","repair"},{"target",snapshot.value("target")}});
        QTRY_VERIFY(received(5)); QVERIFY(receiptMessage.contains("permission")); QVERIFY(!f.game.hasPendingDatagrams());
        state.insert("joining",false); f.send(state); QTRY_VERIFY(backend.state().value("sandboxAllowed").toBool());
        send({{"kind","command"},{"seq",6},{"action","repair"},{"target",snapshot.value("target")}});
        QTRY_VERIFY(f.game.hasPendingDatagrams());
        const auto command=f.command(); QCOMPARE(command.value("action").toString(),QString("repair"));
        host.stop(); QVERIFY(!host.running()); QVERIFY(host.connectedDevice().isEmpty()); QTRY_COMPARE(phone.state(),QAbstractSocket::UnconnectedState);
        QVERIFY(host.start("127.0.0.1",0)); QVERIFY(host.pairingLink()!=link);
        const auto next = QJsonDocument::fromJson(QByteArray::fromBase64(host.pairingLink().mid(11).toLatin1(),QByteArray::Base64UrlEncoding)).object();
        QSslSocket stranger; stranger.setPeerVerifyMode(QSslSocket::VerifyNone);
        stranger.connectToHostEncrypted("127.0.0.1",quint16(next.value("port").toInt())); QTRY_VERIFY(stranger.isEncrypted());
        stranger.write(QJsonDocument(QJsonObject{{"kind","pair"},{"v",1},{"secret",invitation.value("secret")},{"device","Expired"}}).toJson(QJsonDocument::Compact)+'\n');
        QTRY_COMPARE(stranger.state(),QAbstractSocket::UnconnectedState); QVERIFY(host.pendingDevice().isEmpty());
        QSslSocket replay; replay.setPeerVerifyMode(QSslSocket::VerifyNone);
        replay.connectToHostEncrypted("127.0.0.1",quint16(next.value("port").toInt())); QTRY_VERIFY(replay.isEncrypted());
        replay.write(QJsonDocument(QJsonObject{{"kind","pair"},{"v",1},{"secret",next.value("secret")},{"device","New phone"}}).toJson(QJsonDocument::Compact)+'\n');
        QTRY_COMPARE(host.pendingDevice(),QString("New phone")); host.approve();
        const auto invalid=QJsonDocument(QJsonObject{{"kind","command"},{"seq",1},{"action","runLua"},{"target",host.snapshot().value("target")}}).toJson(QJsonDocument::Compact)+'\n';
        replay.write(invalid); QTest::qWait(80); replay.write(invalid);
        QTRY_COMPARE(replay.state(),QAbstractSocket::UnconnectedState); QTRY_VERIFY(host.connectedDevice().isEmpty());
    }
    void mobileCompanionRemembersRevokesStreamsPhotosAndSpawnsIntoEmptyMap() {
        if (QStandardPaths::findExecutable("openssl").isEmpty() || !QSslSocket::supportsSsl()) QSKIP("Local TLS identity generation is unavailable");
        Fixture f; QVERIFY(f.start()); Backend backend(f.path); Previews previews; backend.setPreviews(&previews);
        const auto root=f.dir.filePath("game"); QDir().mkpath(root+"/vehicles/etk800");
        QImage image(640,400,QImage::Format_RGB32); image.fill(Qt::yellow); QVERIFY(image.save(root+"/vehicles/etk800/default.jpg"));
        backend.launcher()->setGameFolder(root);
        auto state=QJsonDocument::fromJson(packet()).object(); state.insert("multiplayer",false);
        state.insert("bridgeVersion","0.11.0"); state.insert("map","empty_map");
        QTimer heartbeat; heartbeat.setInterval(100); connect(&heartbeat,&QTimer::timeout,this,[&] { f.send(state); }); heartbeat.start(); f.send(state);
        QTRY_VERIFY(backend.connected()); QTRY_COMPARE(backend.state().value("map").toString(),QString("empty_map"));
        QVERIFY(!backend.state().contains("vehicleId"));
        backend.requestCatalog(); QTRY_VERIFY(f.game.hasPendingDatagrams()); const auto catalog=f.command();
        f.send({{"kind","catalog"},{"requestId",catalog.value("requestId")},{"chunk",0},{"complete",true},
            {"items",QJsonArray{QJsonObject{{"id","etk800"},{"name","Fixture car"},{"preview",QJsonObject{{"path","/vehicles/etk800/default.jpg"},{"real",root+"/vehicles/etk800/default.jpg"}}}}}}});
        QTRY_COMPARE(backend.catalog().size(),1); QTRY_VERIFY(!backend.busy());
        backend.requestConfigs("etk800"); QTRY_VERIFY(f.game.hasPendingDatagrams()); f.configs(f.command()); QTRY_COMPARE(backend.configs().size(),1); QTRY_VERIFY(!backend.busy());
        const auto directory=f.dir.filePath("mobile"); MobileHost host(&backend,directory); QVERIFY(host.start("127.0.0.1",0));
        auto invite=[&] { return QJsonDocument::fromJson(QByteArray::fromBase64(host.pairingLink().mid(11).toLatin1(),QByteArray::Base64UrlEncoding)).object(); };
        auto send=[](QSslSocket &socket,QJsonObject value) { socket.write(QJsonDocument(value).toJson(QJsonDocument::Compact)+'\n'); };
        QSslSocket phone; phone.setPeerVerifyMode(QSslSocket::VerifyNone); phone.connectToHostEncrypted("127.0.0.1",quint16(invite().value("port").toInt())); QTRY_VERIFY(phone.isEncrypted());
        QList<QJsonObject> inbox;
        auto collect=[&] { while (phone.canReadLine()) inbox.append(QJsonDocument::fromJson(phone.readLine()).object()); };
        auto find=[&](const QString &kind,int seq=0) { collect(); for (const auto &item:inbox) if (item.value("kind")==kind && (!seq || item.value("seq").toInt()==seq)) return item; return QJsonObject{}; };
        send(phone,{{"kind","pair"},{"v",1},{"secret",invite().value("secret")},{"device","Remembered fixture"}});
        QTRY_VERIFY(!host.pendingDevice().isEmpty()); host.approveRemembered(true);
        QTRY_VERIFY(!find("trusted").isEmpty()); const auto credential=find("trusted"); QCOMPARE(host.rememberedCount(),1);
        QFile saved(directory+"/devices.json"); QVERIFY(saved.open(QIODevice::ReadOnly)); const auto stored=saved.readAll(); saved.close();
        QVERIFY(!stored.contains(credential.value("token").toString().toLatin1())); QVERIFY(stored.contains("hash"));
        const auto snapshot=host.snapshot(); const auto photo=snapshot.value("catalog").toArray().first().toObject().value("photo").toString(); QCOMPARE(photo.size(),64);
        QVERIFY(!QJsonDocument(snapshot).toJson().contains(root.toUtf8()));
        send(phone,{{"kind","thumbnail"},{"id",photo}}); QTRY_VERIFY(!find("thumbnail").isEmpty());
        const auto bytes=QByteArray::fromBase64(find("thumbnail").value("jpeg").toString().toLatin1()); QVERIFY(!bytes.isEmpty()); QVERIFY(bytes.size()<=65536);
        const auto decoded=QImage::fromData(bytes,"JPEG"); QVERIFY(!decoded.isNull()); QVERIFY(decoded.width()<=320 && decoded.height()<=200);
        QVERIFY(backend.mobileThumbnail(QString(64,'0')).isEmpty()); QVERIFY(backend.mobileThumbnail("../private").isEmpty());
        QTest::qWait(350); collect(); bool full=false,delta=false;
        for (const auto &item:inbox) if (item.value("kind")=="snapshot") { full|=item.contains("catalog"); delta|=!item.contains("catalog"); }
        QVERIFY(full); QVERIFY(delta);
        send(phone,{{"kind","ping"},{"stamp",42}}); QTRY_COMPARE(find("pong").value("stamp").toInt(),42);
        send(phone,{{"kind","command"},{"seq",1},{"action","spawn"},{"args",QJsonObject{{"config","etk800/Racely (M)"}}},{"target",snapshot.value("target")}});
        QTRY_VERIFY(f.game.hasPendingDatagrams()); const auto spawn=f.command(); QCOMPARE(spawn.value("action").toString(),QString("spawn")); QCOMPARE(spawn.value("config").toString(),QString("Racely (M)"));
        f.send({{"kind","result"},{"requestId",spawn.value("requestId")},{"ok",true}}); QTRY_VERIFY(!backend.busy());
        state.insert("multiplayer",true); state.insert("server","Unverified fixture"); state.insert("serverIp","203.0.113.8"); state.insert("serverPort",30814); f.send(state);
        QTRY_VERIFY(backend.state().value("multiplayer").toBool()); QTRY_VERIFY(!backend.state().value("sandboxAllowed").toBool());
        send(phone,{{"kind","command"},{"seq",2},{"action","spawn"},{"args",QJsonObject{{"config","etk800/Racely (M)"}}},{"target",host.snapshot().value("target")}});
        QTRY_VERIFY(!find("receipt",2).isEmpty()); QVERIFY(find("receipt",2).value("message").toString().contains("permission"));
        // The receiver refreshes its fail-closed MP policy lease, never a spawn.
        while (f.game.hasPendingDatagrams()) QCOMPARE(f.command().value("action").toString(),QString("serverPolicy"));
        host.stop(); QTRY_COMPARE(phone.state(),QAbstractSocket::UnconnectedState);
        MobileHost restored(&backend,directory); QCOMPARE(restored.rememberedCount(),1); QVERIFY(restored.start("127.0.0.1",0));
        const auto next=QJsonDocument::fromJson(QByteArray::fromBase64(restored.pairingLink().mid(11).toLatin1(),QByteArray::Base64UrlEncoding)).object();
        QSslSocket resumed; resumed.setPeerVerifyMode(QSslSocket::VerifyNone); resumed.connectToHostEncrypted("127.0.0.1",quint16(next.value("port").toInt())); QTRY_VERIFY(resumed.isEncrypted());
        send(resumed,{{"kind","resume"},{"v",1},{"id",credential.value("id")},{"token",credential.value("token")}});
        QTRY_COMPARE(restored.connectedDevice(),QString("Remembered fixture")); QVERIFY(restored.pendingDevice().isEmpty());
        restored.forgetDevices(); QCOMPARE(restored.rememberedCount(),0); QTRY_COMPARE(resumed.state(),QAbstractSocket::UnconnectedState);
        QVERIFY(restored.start("127.0.0.1",0));
        const auto revoked=QJsonDocument::fromJson(QByteArray::fromBase64(restored.pairingLink().mid(11).toLatin1(),QByteArray::Base64UrlEncoding)).object();
        QSslSocket rejected; rejected.setPeerVerifyMode(QSslSocket::VerifyNone); rejected.connectToHostEncrypted("127.0.0.1",quint16(revoked.value("port").toInt())); QTRY_VERIFY(rejected.isEncrypted());
        send(rejected,{{"kind","resume"},{"v",1},{"id",credential.value("id")},{"token",credential.value("token")}});
        QTRY_COMPARE(rejected.state(),QAbstractSocket::UnconnectedState); QVERIFY(restored.connectedDevice().isEmpty());
    }
    void androidUsbMappingDoesNotReplaceUnrelatedForwards() {
#ifdef Q_OS_UNIX
        if (QStandardPaths::findExecutable("openssl").isEmpty() || !QSslSocket::supportsSsl()) QSKIP("Local TLS identity generation is unavailable");
        QTemporaryDir directory; const auto path=qgetenv("PATH");
        struct PathGuard { QByteArray value; ~PathGuard() { qputenv("PATH",value); } } guard{path};
        const auto helper=directory.filePath("adb"), log=directory.filePath("calls"), mapping=directory.filePath("mapping");
        const auto script=QString("#!/bin/sh\necho \"$*\" >> '%1'\ncase \"$*\" in\n '-d get-serialno') echo fixture-serial;;\n '-s fixture-serial reverse --list') cat '%2';;\n '-s fixture-serial reverse --no-rebind tcp:44445 tcp:44445') echo 'fixture-serial tcp:44445 tcp:44445' > '%2';;\n '-s fixture-serial reverse --remove tcp:44445') : > '%2';;\n *) exit 2;;\nesac\n").arg(log,mapping);
        write(helper,script.toUtf8()); QVERIFY(QFile::setPermissions(helper,QFileDevice::ReadOwner|QFileDevice::WriteOwner|QFileDevice::ExeOwner)); write(mapping,{});
        qputenv("PATH",directory.path().toUtf8()+":"+path);
        Backend backend(directory.filePath("missing-state.json")); MobileHost host(&backend,directory.filePath("mobile")); UsbTunnel usb(&host);
        QVERIFY(usb.start("android")); QTRY_VERIFY(usb.running()); QCOMPARE(host.listenAddress(),QHostAddress(QHostAddress::LocalHost));
        QVERIFY(usb.pairingLink().startsWith("abm://pair/")); usb.stop(); QVERIFY(!host.running());
        QFile removed(mapping); QVERIFY(removed.open(QIODevice::ReadOnly)); QVERIFY(removed.readAll().isEmpty()); removed.close();
        write(mapping,"other tcp:44445 tcp:9999\n"); QVERIFY(usb.start("android")); QTRY_VERIFY(usb.message().contains("will not replace")); QVERIFY(!usb.running());
        QFile untouched(mapping); QVERIFY(untouched.open(QIODevice::ReadOnly)); QCOMPARE(untouched.readAll(),QByteArray("other tcp:44445 tcp:9999\n"));
        QFile calls(log); QVERIFY(calls.open(QIODevice::ReadOnly)); const auto commands=calls.readAll(); QVERIFY(!commands.contains("--remove-all")); QCOMPARE(commands.count("--no-rebind"),1); QCOMPARE(commands.count("--remove tcp:44445"),1);
#else
        QSKIP("Fake POSIX helper test; Windows uses the same argument-building state machine");
#endif
    }
    void iphoneUsbByteBridgePreservesPinnedTlsAndPcApproval() {
#ifdef Q_OS_UNIX
        if (QStandardPaths::findExecutable("openssl").isEmpty() || !QSslSocket::supportsSsl()) QSKIP("Local TLS identity generation is unavailable");
        CablePeer cable; if (!cable.listen(QHostAddress::LocalHost,44448)) QSKIP("Test USB loopback port is in use");
        QTemporaryDir directory; const auto path=qgetenv("PATH");
        struct PathGuard { QByteArray value; ~PathGuard() { qputenv("PATH",value); } } guard{path};
        const auto tool=directory.filePath("iproxy"); write(tool,"#!/bin/sh\ntrap 'exit 0' TERM INT\nwhile :; do sleep 1; done\n");
        QVERIFY(QFile::setPermissions(tool,QFileDevice::ReadOwner|QFileDevice::WriteOwner|QFileDevice::ExeOwner)); qputenv("PATH",directory.path().toUtf8()+":"+path);
        Backend backend(directory.filePath("missing-state.json")); MobileHost host(&backend,directory.filePath("mobile")); UsbTunnel usb(&host);
        QVERIFY(usb.start("iphone")); QTRY_VERIFY(usb.running());
        const auto invitation=QJsonDocument::fromJson(QByteArray::fromBase64(usb.pairingLink().mid(11).toLatin1(),QByteArray::Base64UrlEncoding)).object();
        QCOMPARE(invitation.value("transport").toString(),QString("usb-iphone")); QCOMPARE(invitation.value("host").toString(),QString("127.0.0.1")); QCOMPARE(invitation.value("port").toInt(),44447);
        QTRY_VERIFY(cable.phone && cable.phone->isEncrypted());
        QCOMPARE(QString::fromLatin1(cable.phone->peerCertificate().digest(QCryptographicHash::Sha256).toHex()),invitation.value("sha256").toString());
        cable.phone->write(QJsonDocument(QJsonObject{{"kind","pair"},{"v",1},{"secret",invitation.value("secret")},{"device","Cable fixture"}}).toJson(QJsonDocument::Compact)+'\n');
        QTRY_COMPARE(host.pendingDevice(),QString("Cable fixture")); QVERIFY(host.connectedDevice().isEmpty()); host.approveRemembered(true);
        QTRY_COMPARE(host.connectedDevice(),QString("Cable fixture")); usb.stop(); QVERIFY(!host.running()); QVERIFY(!usb.running());
        QTRY_VERIFY(!cable.phone || cable.phone->state()==QAbstractSocket::UnconnectedState);
#else
        QSKIP("Fake POSIX iproxy transport; native iOS loopback relay is tested on macOS");
#endif
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
    void playerProfilesUseFixedPublicOrigins() {
        const auto url=PlayerProfiles::profileUrl("Test Driver");
        QCOMPARE(url.scheme(),QString("https")); QCOMPARE(url.host(),QString("forum.beammp.com"));
        QCOMPARE(url.path(QUrl::FullyEncoded),QString("/u/Test%20Driver/summary"));
        QVERIFY(!PlayerProfiles::profileUrl("bad/name").isValid());
        QVERIFY(!PlayerProfiles::profileUrl("bad\\name").isValid());
        QVERIFY(!PlayerProfiles::profileUrl(QString(65,'a')).isValid());
        const QByteArray body=R"({"user":{"id":42,"username":"Test Driver"}})";
        QCOMPARE(PlayerProfiles::accountId(body,"Test Driver"),QString("42"));
        QVERIFY(PlayerProfiles::accountId(body,"Other").isEmpty());
        QVERIFY(PlayerProfiles::accountId(R"({"user":{"id":-1,"username":"Test Driver"}})","Test Driver").isEmpty());
        QVERIFY(PlayerProfiles::accountId(QByteArray(300000,'x'),"Test Driver").isEmpty());
    }
    void playerCopiesUseCurrentSessionRoster() {
        PlayerProfiles profiles;
        profiles.setPlayers("session-a", {QVariantMap{{"rowKey","id:0"},{"rawName","Test Guest"},
            {"sessionPlayerId",0},{"guest",true},{"vehicles",2},{"role","USER"}}});
        profiles.copyName("id:0");
        QCOMPARE(QGuiApplication::clipboard()->text(),QString("Test Guest"));
        profiles.copyDetails("id:0");
        const auto details = QGuiApplication::clipboard()->text();
        QVERIFY(details.contains("Session ID: 0"));
        QVERIFY(details.contains("Vehicles: 2"));
        QVERIFY(!details.contains("Ping:"));
        profiles.copySessionId("id:0");
        QCOMPARE(QGuiApplication::clipboard()->text(),QString("0"));
        profiles.setPlayers("session-b", {});
        profiles.copyName("id:0");
        QCOMPARE(QGuiApplication::clipboard()->text(),QString("0"));
        profiles.copyDetails("id:0");
        QCOMPARE(QGuiApplication::clipboard()->text(),QString("0"));
    }
    void playerProfileActionsCompleteOrCancelWithoutExternalRequests() {
        ProfileNetwork network; PlayerProfiles profiles(nullptr,&network); BrowserGuard browser;
        QVariantMap player{{"rowKey","id:0"},{"rawName","TestDriver"},{"sessionPlayerId",0}};
        profiles.setPlayers("server-a",{player}); profiles.openProfile("TestDriver");
        QCOMPARE(browser.capture.url,PlayerProfiles::profileUrl("TestDriver"));
        const QByteArray body=R"({"user":{"username":"TestDriver","id":73}})";
        QGuiApplication::clipboard()->setText("before"); profiles.copyAccount("id:0");
        QVERIFY(profiles.resolving()); QCOMPARE(network.request.url(),PlayerProfiles::profileUrl("TestDriver",true));
        QCOMPARE(network.request.transferTimeout(),10000);
        QCOMPARE(network.request.attribute(QNetworkRequest::RedirectPolicyAttribute).toInt(),int(QNetworkRequest::ManualRedirectPolicy));
        network.reply->complete(body); QVERIFY(!profiles.resolving());
        QCOMPARE(QGuiApplication::clipboard()->text(),QString("73"));
        profiles.copyAccount("id:0"); QGuiApplication::clipboard()->setText("new clipboard"); network.reply->complete(body);
        QCOMPARE(QGuiApplication::clipboard()->text(),QString("new clipboard")); QVERIFY(!profiles.resolving());
        profiles.copyAccount("id:0"); profiles.setPlayers("server-b",{player});
        QVERIFY(!profiles.resolving()); network.reply->complete(body);
        QCOMPARE(QGuiApplication::clipboard()->text(),QString("new clipboard"));
        profiles.copyAccount("id:0"); player.insert("rawName","Replacement"); profiles.setPlayers("server-b",{player});
        QVERIFY(!profiles.resolving()); network.reply->complete(body);
        QCOMPARE(QGuiApplication::clipboard()->text(),QString("new clipboard"));
        player.insert("rawName","TestDriver"); profiles.setPlayers("server-b",{player});
        for (const auto &response : {QByteArray(R"({"user":{"username":"Other","id":73}})"),QByteArray("invalid"),QByteArray(262145,'x')}) {
            profiles.copyAccount("id:0"); network.reply->complete(response);
            QCOMPARE(QGuiApplication::clipboard()->text(),QString("new clipboard")); QVERIFY(!profiles.resolving());
        }
        profiles.copyAccount("id:0"); network.reply->complete(body,302);
        QCOMPARE(QGuiApplication::clipboard()->text(),QString("new clipboard"));
        profiles.copyAccount("id:0"); profiles.copySessionId("id:0"); network.reply->complete(body);
        QCOMPARE(QGuiApplication::clipboard()->text(),QString("0"));
        player.insert("guest",true); profiles.setPlayers("server-b",{player}); browser.capture.url=QUrl();
        profiles.openProfile("TestDriver"); QVERIFY(browser.capture.url.isEmpty());
        profiles.copyAccount("id:0"); QVERIFY(!profiles.resolving()); QCOMPARE(QGuiApplication::clipboard()->text(),QString("0"));
    }
    void playerTelemetryKeepsStableIdentityAndProvenance() {
        auto input=QJsonDocument::fromJson(packet()).object();
        input.insert("players",QJsonArray{
            QJsonObject{{"rawName","Local"},{"sessionPlayerId",7},{"ping",24},{"pingSource","local"},{"pingAgeMs",1200},{"vehicles",1},{"guest",false},{"role","USER"}},
            QJsonObject{{"rawName","Guest"},{"ping",-1},{"pingSource","invented"},{"role","ADMIN"}},
            QJsonObject{{"rawName",QString(100,'x')},{"sessionPlayerId",-4},{"ping",90000},{"vehicles",2000}}
        });
        const auto out=Telemetry::sanitize(QJsonDocument(input).toJson(QJsonDocument::Compact));
        const auto players=out.value("players").toArray();
        QCOMPARE(players.size(),3);
        const auto one=players[0].toObject();
        QCOMPARE(one.value("rowKey").toString(),QString("id:7"));
        QCOMPARE(one.value("pingSource").toString(),QString("local"));
        QCOMPARE(one.value("role").toString(),QString("USER"));
        const auto two=players[1].toObject();
        QVERIFY(two.value("rowKey").toString().startsWith("name:"));
        QVERIFY(!two.contains("ping"));
        QCOMPARE(two.value("pingSource").toString(),QString("unavailable"));
        QVERIFY(!two.value("guest").toBool()); // Missing session ID does not prove guest status.
        const auto three=players[2].toObject();
        QCOMPARE(three.value("name").toString().size(),64);
        QVERIFY(!three.contains("sessionPlayerId"));
        QVERIFY(!three.contains("ping"));
        QVERIFY(!three.contains("vehicles"));
    }
    void presetStoreValidatesAndPreservesFiles() {
        QTemporaryDir dir; PresetStore store(dir.filePath("presets"));
        const QJsonObject config{{"model","etk800"},{"identity","abc123"},{"label","Test car"},{"completeness","workshop-overrides"}};
        const QJsonObject settings{{"power",QJsonObject{{"enginePower",1.2}}},
            {"gears",QJsonObject{{"ratios",QJsonArray{2.5,1.8,1.2,.9}}}}};
        QVERIFY(!PresetStore::validSettings(QJsonObject{{"runLua",QJsonObject{{"code","x"}}}}));
        const auto id=store.save("Track",config,settings);
        QVERIFY(!id.isEmpty());
        const auto stored=store.get(id); QCOMPARE(stored.value("name").toString(),QString("Track"));
        QCOMPARE(stored.value("requirements").toString(),QString("ABM-0.11-runtime-1"));
        QVERIFY(store.renamePreset(id,"Renamed")); QCOMPARE(store.get(id).value("name").toString(),QString("Renamed"));
        QVERIFY(store.annotate(id,"track,test","notes"));
        const auto exported=store.exportJson(id); QVERIFY(exported.contains("Renamed"));
        QVERIFY(!store.remove(id,false)); QVERIFY(store.remove(id,true)); QVERIFY(store.get(id).isEmpty());
        QVERIFY(store.importJson(exported)); QCOMPARE(store.entries().size(),1);
        QJsonObject bad=stored; bad.insert("unknown",true); QString reason;
        QVERIFY(!PresetStore::validate(bad,&reason)); QVERIFY(!reason.isEmpty());
    }
    void randomTuneIsDeterministicAndBounded() {
        Backend backend(QDir::temp().filePath("abm-random-test/state.json"));
        const auto a=backend.randomTune("fixed"), b=backend.randomTune("fixed"), c=backend.randomTune("other");
        QCOMPARE(a,b); QVERIFY(a!=c);
        QVERIFY(a.value("enginePower").toDouble()>=.8 && a.value("enginePower").toDouble()<=1.5);
        QVERIFY(a.value("steeringResponse").toDouble()>=.8 && a.value("steeringResponse").toDouble()<=1.2);
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
        const QJsonObject snapshot{{"connected",true},{"updatedAt",QDateTime::currentMSecsSinceEpoch()},
            {"sessionId","snapshot"},{"multiplayer",true},{"serverIp","2001:db8::7"},{"serverPort",30814},
            {"players",QJsonArray{QJsonObject{{"rowKey","id:0"},{"rawName","SnapshotDriver"}}}}};
        write(dir.filePath("state.json"),QJsonDocument(snapshot).toJson()); backend.refresh(); QVERIFY(backend.connected());
        backend.profiles()->copyName("id:0"); QCOMPARE(QGuiApplication::clipboard()->text(),QString("SnapshotDriver"));
        QVERIFY(backend.copyServerAddress()); QCOMPARE(QGuiApplication::clipboard()->text(),QString("[2001:db8::7]:30814"));
        write(dir.filePath("state.json"),"invalid"); backend.refresh();
        backend.profiles()->copyName("id:0"); QCOMPARE(QGuiApplication::clipboard()->text(),QString("[2001:db8::7]:30814"));
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
        QFile file(path); QVERIFY(file.open(QIODevice::ReadOnly)); QVERIFY(file.readAll().contains("0.11.0")); file.close();
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
    void previewsDecodeEachCandidateAndRespectMountedPriority() {
        QTemporaryDir dir;
        const auto game = dir.filePath("game"), user = dir.filePath("user");
        const auto key = QString("vehicles/etk800/Custom.png");
        QDir().mkpath(game + "/vehicles/etk800"); QDir().mkpath(user + "/vehicles/etk800");
        QImage red(80,50,QImage::Format_RGB32); red.fill(Qt::red);
        QImage blue(80,50,QImage::Format_RGB32); blue.fill(Qt::blue);
        QVERIFY(red.save(game + '/' + key)); QVERIFY(blue.save(user + '/' + key));
        QByteArray data; QBuffer buffer(&data); buffer.open(QIODevice::WriteOnly); QVERIFY(red.save(&buffer,"PNG"));
        const auto zip = dir.filePath("mounted.zip");
        writeZip(zip,key.toUtf8(),data);
        Previews provider; QSize size;
        const auto mounted = provider.registerImage({{"path",'/' + key},{"archive",zip}}, {game,user,dir.path()});
        auto image = provider.requestImage(mounted.section('/',-1),&size,QSize(40,40));
        QVERIFY(!image.isNull()); QCOMPARE(image.pixelColor(0,0),QColor(Qt::red));

        // A corrupt mounted image must not mask a decodable loose candidate.
        writeZip(zip,key.toUtf8(),"not a PNG");
        const auto fallback = provider.registerImage({{"path",'/' + key},{"archive",zip}}, {game,user,dir.path()});
        QVERIFY(fallback != mounted);
        image = provider.requestImage(fallback.section('/',-1),&size,QSize(40,40));
        QVERIFY(!image.isNull()); QCOMPARE(image.pixelColor(0,0),QColor(Qt::blue));

        // Nonempty corrupt loose data must also advance to the next same-path image.
        write(user + '/' + key,"corrupt loose image");
        const auto loose = provider.registerImage({{"path",'/' + key}}, {game,user});
        image = provider.requestImage(loose.section('/',-1),&size,QSize(40,40));
        QVERIFY(!image.isNull()); QCOMPARE(image.pixelColor(0,0),QColor(Qt::red));
        QCOMPARE(size,QSize(80,50));
    }
    void previewsCacheResultsAndInvalidateChangedSources() {
        QTemporaryDir dir; const auto path = dir.filePath("vehicles/etk800/Custom.png");
        write(path,"corrupt"); Previews provider; QSize size(99,99);
        const QJsonObject descriptor{{"path","/vehicles/etk800/Custom.png"}};
        const auto bad = provider.registerImage(descriptor,{dir.path()});
        QVERIFY(!bad.isEmpty());
        QVERIFY(provider.requestImage(bad.section('/',-1),&size,QSize(40,40)).isNull());
        QCOMPARE(size,QSize());
        QImage photo(80,50,QImage::Format_RGB32); photo.fill(Qt::red); QVERIFY(photo.save(path));
        // Failed decodes stay cached for their revision; registration sees repairs.
        QVERIFY(provider.requestImage(bad.section('/',-1),&size,QSize(40,40)).isNull());
        const auto repaired = provider.registerImage(descriptor,{dir.path()}); QVERIFY(repaired != bad);
        auto image = provider.requestImage(repaired.section('/',-1),&size,QSize(40,40));
        QVERIFY(!image.isNull()); QCOMPARE(size,QSize(80,50));
        QCOMPARE(image.size(),QSize(40,25));
        QVERIFY(QFile::remove(path));
        // Successful decodes are reused without reopening the source.
        QVERIFY(!provider.requestImage(repaired.section('/',-1),&size,QSize(40,40)).isNull());
        QCOMPARE(size,QSize(80,50));
        QVERIFY(provider.registerImage(descriptor,{dir.path()}).isEmpty());
        QVERIFY(provider.requestImage(repaired.section('/',-1),&size,QSize(20,20)).isNull());
        QCOMPARE(size,QSize());

        QImage replacement(90,60,QImage::Format_RGB32); replacement.fill(Qt::blue); QVERIFY(replacement.save(path));
        const auto changed = provider.registerImage(descriptor,{dir.path()}); QVERIFY(changed != repaired);
        image = provider.requestImage(changed.section('/',-1),&size,QSize(4000,4000));
        QVERIFY(!image.isNull()); QCOMPARE(image.pixelColor(0,0),QColor(Qt::blue));
        QVERIFY(image.width() <= 960 && image.height() <= 640); QCOMPARE(size,QSize(90,60));
    }
    void previewsRejectOversizeAndSourceEscapes() {
        QTemporaryDir dir, outside;
        const auto game = dir.filePath("game"), user = dir.filePath("user");
        const auto key = QString("vehicles/etk800/Custom.png");
        QDir().mkpath(game + "/vehicles/etk800"); QDir().mkpath(user + "/vehicles/etk800");
        QImage photo(80,50,QImage::Format_RGB32); photo.fill(Qt::red); QVERIFY(photo.save(game + '/' + key));
        QImage wide(9000,1,QImage::Format_RGB32); wide.fill(Qt::blue); QVERIFY(wide.save(user + '/' + key));
        Previews provider; QSize size;
        const QJsonObject descriptor{{"path",'/' + key}};
        auto url = provider.registerImage(descriptor,{game,user});
        QVERIFY(!provider.requestImage(url.section('/',-1),&size,QSize(40,40)).isNull());
        QCOMPARE(size,QSize(80,50)); // Oversized dimensions fall through, not just corrupt headers.
        write(user + '/' + key,QByteArray(8*1024*1024+1,'x'));
        url = provider.registerImage(descriptor,{game,user});
        QVERIFY(!provider.requestImage(url.section('/',-1),&size,QSize(40,40)).isNull());
        QCOMPARE(size,QSize(80,50));
#ifdef Q_OS_UNIX
        const auto secret = outside.filePath("private.png"); QVERIFY(photo.save(secret));
        const auto link = user + '/' + key; QVERIFY(QFile::remove(link)); QVERIFY(QFile::link(secret,link));
        QVERIFY(provider.registerImage(descriptor,{user}).isEmpty());
        QVERIFY(QFile::remove(link)); QVERIFY(photo.save(link));
        url = provider.registerImage(descriptor,{user});
        QVERIFY(QFile::remove(link)); QVERIFY(QFile::link(secret,link));
        QVERIFY(provider.requestImage(url.section('/',-1),&size,QSize(40,40)).isNull());
        QCOMPARE(size,QSize());
#endif
    }
    void variantPreviewFallbackSurvivesCardReuse() {
        QTemporaryDir dir; Theme theme(dir.path());
        QImage photo(80,50,QImage::Format_RGB32); photo.fill(Qt::red);
        const auto model = dir.filePath("model.png"), exact = dir.filePath("exact.png");
        QVERIFY(photo.save(model)); photo.fill(Qt::blue); QVERIFY(photo.save(exact));
        const auto modelUrl = QUrl::fromLocalFile(model).toString(), exactUrl = QUrl::fromLocalFile(exact).toString();
        const auto missingUrl = QUrl::fromLocalFile(dir.filePath("missing.png")).toString();
        QQmlEngine engine; engine.rootContext()->setContextProperty("theme",&theme);
        QQmlComponent component(&engine,QUrl("qrc:/VariantCard.qml"));
        QVERIFY2(component.isReady(),qPrintable(component.errorString()));
        QScopedPointer<QObject> card(component.createWithInitialProperties({{"modelData",QVariantMap{
            {"id","first"},{"thumbnail",missingUrl},{"modelThumbnail",modelUrl}}}}));
        QVERIFY(card);
        auto *image = card->findChild<QObject *>("variantPhoto"); QVERIFY(image);
        QTRY_COMPARE(card->property("previewKind").toString(),QString("model"));
        QCOMPARE(image->property("source").toUrl().toString(),modelUrl);
        QVERIFY(card->property("exactPreviewFailed").toBool());
        // Reusing the delegate must restore the exact-image binding, not retain
        // the preceding vehicle's fallback or its provenance label.
        card->setProperty("modelData",QVariantMap{{"id","second"},{"thumbnail",exactUrl},{"modelThumbnail",modelUrl}});
        QTRY_COMPARE(card->property("previewKind").toString(),QString("exact"));
        QCOMPARE(image->property("source").toUrl().toString(),exactUrl);
        QVERIFY(!card->property("exactPreviewFailed").toBool());
        card->setProperty("modelData",QVariantMap{{"id","empty"}});
        QTRY_COMPARE(card->property("previewKind").toString(),QString("unavailable"));
        QVERIFY(image->property("source").toUrl().isEmpty());
        QVERIFY(!card->property("fallbackPreview").toBool());
        card->setProperty("modelData",QVariantMap{{"id","broken"},{"thumbnail",missingUrl},{"modelThumbnail",missingUrl}});
        QTRY_VERIFY(card->property("exactPreviewFailed").toBool());
        QCOMPARE(card->property("previewKind").toString(),QString("unavailable"));
        card->setProperty("modelData",QVariantMap{{"id","model-only"},{"modelThumbnail",modelUrl}});
        QTRY_COMPARE(card->property("previewKind").toString(),QString("model"));
    }
    void themeAndTranslatorReload() {
        QTemporaryDir dir; QDir().mkpath(dir.filePath("theme")); Theme theme(dir.path()); QCOMPARE(theme.accent(),QString("#FFD60A"));
        write(dir.filePath("theme/colors.toml"),"accent = \"#123456\"\n"); QTRY_COMPARE(theme.accent(),QString("#123456"));
        write(dir.filePath("pt.json"),R"({"Save":"Guardar","Open":"Abrir"})");
        write(dir.filePath("pt-BR.json"),R"({"Save":"Salvar"})"); Translator translator(dir.filePath("language"),dir.path());
        write(dir.filePath("language"),"pt-BR\n"); QTRY_COMPARE(translator.language(),QString("pt-BR"));
        QCOMPARE(translator.translate("Main","Save"),QString("Salvar")); QCOMPARE(translator.translate("Main","Open"),QString("Abrir"));
    }
    void glassControlInteraction() {
        QTemporaryDir dir; Theme theme(dir.path());
        QQmlApplicationEngine engine; QStringList warnings;
        connect(&engine,&QQmlEngine::warnings,this,[&](const QList<QQmlError> &errors){for(const auto &e:errors) warnings<<e.toString();});
        engine.rootContext()->setContextProperty("theme",&theme);
        engine.loadData(R"(
            import QtQuick
            import QtQuick.Controls
            ApplicationWindow {
                id: root
                width: 380; height: 500; visible: true
                property int clicks: 0
                property int acceptedCount: 0
                property int rejectedCount: 0
                property int menuCount: 0
                Column {
                    Button { objectName: "testButton"; text: "Apply"; onClicked: root.clicks++ }
                    TextField { objectName: "testField"; width: 200; placeholderText: "Search" }
                    TextArea { objectName: "testArea"; width: 200; height: 80; placeholderText: "Notes" }
                    ComboBox { objectName: "testCombo"; model: ["Engine", "Handling", "World"] }
                    CheckBox { objectName: "testCheck"; text: "Favorites only" }
                }
                Menu {
                    objectName: "testMenu"
                    function focusFirst() { first.forceActiveFocus() }
                    MenuItem { id: first; text: "Refresh"; onTriggered: root.menuCount++ }
                }
                Dialog {
                    objectName: "testDialog"; width: 300; title: "Confirm"
                    standardButtons: Dialog.Ok | Dialog.Cancel
                    function focusOkay() { standardButton(Dialog.Ok).forceActiveFocus() }
                    onAccepted: root.acceptedCount++
                    onRejected: root.rejectedCount++
                }
            }
        )");
        QVERIFY2(!engine.rootObjects().isEmpty(),qPrintable(warnings.join('\n')));
        auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects().first()); QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowExposed(window)); window->requestActivate();
        auto *button=window->findChild<QQuickItem *>("testButton"); QVERIFY(button);
        button->forceActiveFocus(); QTest::keyClick(window,Qt::Key_Space);
        QCOMPARE(window->property("clicks").toInt(),1);
        button->setEnabled(false); QTest::keyClick(window,Qt::Key_Space);
        QCOMPARE(window->property("clicks").toInt(),1);
        auto *field=window->findChild<QQuickItem *>("testField"); QVERIFY(field);
        const auto typeText=[&](const QByteArray &text) { for (char c:text) QTest::keyClick(window,c); };
        field->forceActiveFocus(); typeText("ratio");
        QCOMPARE(field->property("text").toString(),QString("ratio"));
        QTest::keyClick(window,Qt::Key_A,Qt::ControlModifier); typeText("2.5");
        QCOMPARE(field->property("text").toString(),QString("2.5"));
        auto *area=window->findChild<QQuickItem *>("testArea"); QVERIFY(area);
        QVERIFY(area->findChild<QQuickItem *>("glassFieldSurface"));
        area->forceActiveFocus(); typeText("first"); QTest::keyClick(window,Qt::Key_Return); typeText("second");
        QCOMPARE(area->property("text").toString(),QString("first\nsecond"));
        auto *combo=window->findChild<QQuickItem *>("testCombo"); QVERIFY(combo);
        combo->forceActiveFocus(); QTest::keyClick(window,Qt::Key_Down);
        QCOMPARE(combo->property("currentIndex").toInt(),1);
        combo->setProperty("editable",true);
        auto *comboEditor=qvariant_cast<QQuickItem *>(combo->property("contentItem")); QVERIFY(comboEditor);
        for (auto *editor : {field,area,comboEditor}) {
            editor->setProperty("text","context copy"); editor->forceActiveFocus();
            QVERIFY(QMetaObject::invokeMethod(editor,"selectAll"));
            auto *editMenu=editor->findChild<QObject *>("fieldEditMenu"); QVERIFY(editMenu);
            QGuiApplication::clipboard()->setText("before context menu");
            QTest::mouseClick(window,Qt::RightButton,Qt::NoModifier,editor->mapToScene(QPointF(10,10)).toPoint());
            QTRY_VERIFY(editMenu->property("visible").toBool());
            auto *copy=editMenu->findChild<QQuickItem *>("fieldCopy"); QVERIFY(copy && copy->isEnabled());
            QTest::mouseClick(window,Qt::LeftButton,Qt::NoModifier,copy->mapToScene(QPointF(copy->width()/2,copy->height()/2)).toPoint());
            QTRY_COMPARE(QGuiApplication::clipboard()->text(),QString("context copy"));
            QTRY_VERIFY(!editMenu->property("visible").toBool());
            editor->forceActiveFocus(); QTest::keyClick(window,Qt::Key_F10,Qt::ShiftModifier);
            QTRY_VERIFY(editMenu->property("visible").toBool());
            QTest::keyClick(window,Qt::Key_Escape); QTRY_VERIFY(!editMenu->property("visible").toBool());
        }
        auto *check=window->findChild<QQuickItem *>("testCheck"); QVERIFY(check);
        check->forceActiveFocus(); QTest::keyClick(window,Qt::Key_Space);
        QVERIFY(check->property("checked").toBool());
        check->setEnabled(false); QTest::keyClick(window,Qt::Key_Space);
        QVERIFY(check->property("checked").toBool());
        auto *menu=window->findChild<QObject *>("testMenu"); QVERIFY(menu);
        auto *menuBackground=qvariant_cast<QObject *>(menu->property("background")); QVERIFY(menuBackground);
        QCOMPARE(menuBackground->property("color").value<QColor>(),QColor("#1c222c"));
        QVERIFY(QMetaObject::invokeMethod(menu,"open"));
        QTRY_VERIFY(menu->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(menu,"focusFirst"));
        QTest::keyClick(window,Qt::Key_Return);
        QTRY_COMPARE(window->property("menuCount").toInt(),1);
        QTRY_VERIFY(!menu->property("visible").toBool());
        auto *dialog=window->findChild<QObject *>("testDialog"); QVERIFY(dialog);
        QVERIFY(QMetaObject::invokeMethod(dialog,"open"));
        QTRY_VERIFY(dialog->property("visible").toBool());
        auto *footer=qvariant_cast<QObject *>(dialog->property("footer")); QVERIFY(footer);
        QVERIFY(footer->findChild<QQuickItem *>("glassButtonSurface"));
        QTest::keyClick(window,Qt::Key_Escape);
        QTRY_COMPARE(window->property("rejectedCount").toInt(),1);
        QVERIFY(QMetaObject::invokeMethod(dialog,"open"));
        QTRY_VERIFY(dialog->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(dialog,"focusOkay"));
        QTest::keyClick(window,Qt::Key_Space);
        QTRY_COMPARE(window->property("acceptedCount").toInt(),1);
        QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n')));
    }
    void responsiveGarageKeepsColumnsAndFiltersUsable() {
        QVERIFY2(!QFontDatabase::families().isEmpty(),"UI layout tests require an installed font; Linux CI installs noto-fonts.");
        QTemporaryDir dir; Backend backend(dir.filePath("state.json")); backend.setDemo();
        Theme theme(dir.path()); Translator translator(dir.filePath("language"));
        QQmlApplicationEngine engine; QStringList warnings;
        connect(&engine,&QQmlEngine::warnings,this,[&](const QList<QQmlError> &errors){for(const auto &e:errors) warnings<<e.toString();});
        engine.rootContext()->setContextProperty("backend",&backend); engine.rootContext()->setContextProperty("theme",&theme);
        engine.rootContext()->setContextProperty("translator",&translator); engine.rootContext()->setContextProperty("demoMode",true);
        engine.load(QUrl("qrc:/Main.qml")); QVERIFY2(!engine.rootObjects().isEmpty(),qPrintable(warnings.join('\n')));
        auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects().first()); QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowExposed(window)); window->requestActivate(); window->setProperty("page",2);
        auto *grid=window->findChild<QQuickItem *>("garageGrid"); QVERIFY(grid);
        auto *filters=window->findChild<QObject *>("garageFiltersDialog"); QVERIFY(filters);
        for (const auto dimensions : {QSize(320,440),QSize(380,600),QSize(560,840),QSize(1040,860)}) {
            window->resize(dimensions); window->setProperty("selectedModel",QVariantMap{});
            QTRY_COMPARE(window->size(),dimensions);
            const int expectedColumns=dimensions.width() < 400 ? 1 : dimensions.width() < 760 ? 2 : 4;
            QTRY_COMPARE(grid->property("columns").toInt(),expectedColumns);
            QTest::qWait(50);
            QCOMPARE(grid->property("count").toInt(),5);
            const int columns=grid->property("columns").toInt();
            QCOMPARE(columns,expectedColumns);
            QVERIFY(grid->height()>=80);
            QVERIFY(qAbs(grid->property("cellWidth").toDouble()*columns-grid->width())<0.1);
            QQuickItem *first=nullptr, *second=nullptr;
            QVERIFY(QMetaObject::invokeMethod(grid,"itemAtIndex",Q_RETURN_ARG(QQuickItem *,first),Q_ARG(int,0)));
            QVERIFY(QMetaObject::invokeMethod(grid,"itemAtIndex",Q_RETURN_ARG(QQuickItem *,second),Q_ARG(int,1)));
            QVERIFY(first && second);
            QVERIFY(first->width()<=grid->property("cellWidth").toDouble());
            if (columns>1) { QCOMPARE(first->y(),second->y()); QVERIFY(second->x()>first->x()); }
            else QVERIFY(second->y()>first->y());
            // Modal filters must never consume the gallery's layout space.
            const auto galleryHeight=grid->height(); window->setProperty("extraFilters",true);
            QTRY_VERIFY(filters->property("visible").toBool()); QCOMPARE(grid->height(),galleryHeight);
            QVERIFY(filters->property("height").toDouble()<=window->height());
            QTest::keyClick(window,Qt::Key_Escape); QTRY_VERIFY(!filters->property("visible").toBool());
            QVERIFY(!window->property("extraFilters").toBool());
            backend.requestConfigs("etk800");
            window->setProperty("selectedModel",QVariantMap{{"id","etk800"},{"name","800 Series"}});
            QTest::qWait(80); QCOMPARE(grid->property("count").toInt(),2); QVERIFY(grid->height()>=60);
            // Browsing and resizing never acquires authority to act in a demo.
            QVERIFY(!window->property("canDriveAction").toBool());
        }
        QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n')));
    }
    void sessionButtonsSurviveChangingTelemetry() {
        Fixture f; QVERIFY(f.start()); Backend backend(f.path); QTRY_VERIFY(backend.connected());
        BrowserGuard browser;
        Theme theme(f.dir.filePath("theme")); Translator translator(f.dir.filePath("language"));
        auto data=QJsonDocument::fromJson(packet()).object();
        data.insert("server","Session test"); data.insert("serverIp","203.0.113.7"); data.insert("serverPort",30814);
        data.insert("players",QJsonArray{QJsonObject{{"rawName","TestDriver"},{"sessionPlayerId",0},{"local",true},{"ping",25}}});
        data.insert("activeVehicles",QJsonArray{QJsonObject{{"id",42},{"name","Test vehicle"},{"owned",true}}});
        data.insert("telemetry",QJsonObject{{"speed",10}});
        QTimer updates; updates.setInterval(50);
        connect(&updates,&QTimer::timeout,this,[&]{
            auto rows=data.value("players").toArray(); auto p=rows[0].toObject();
            p.insert("pingAgeMs",p.value("pingAgeMs").toInt()+50); rows[0]=p; data.insert("players",rows); f.send(data);
        });
        updates.start(); f.send(data); QTRY_COMPARE(backend.players().size(),1);
        QQmlApplicationEngine engine; QStringList warnings;
        connect(&engine,&QQmlEngine::warnings,this,[&](const QList<QQmlError> &errors){for(const auto &error:errors) warnings<<error.toString();});
        engine.rootContext()->setContextProperty("backend",&backend); engine.rootContext()->setContextProperty("theme",&theme);
        engine.rootContext()->setContextProperty("translator",&translator); engine.rootContext()->setContextProperty("demoMode",false);
        engine.load(QUrl("qrc:/Main.qml")); QVERIFY2(!engine.rootObjects().isEmpty(),qPrintable(warnings.join('\n')));
        auto *window=qobject_cast<QQuickWindow *>(engine.rootObjects().first()); QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowExposed(window)); window->requestActivate(); window->setProperty("page",3); QTest::qWait(100);
        auto find=[&](const QString &name)->QQuickItem * {
            QList<QQuickItem *> pending{window->contentItem()};
            while(!pending.isEmpty()) { auto *item=pending.takeLast(); if(item->objectName()==name) return item; pending.append(item->childItems()); }
            return nullptr;
        };
        auto click=[&](QQuickItem *button) {
            QVERIFY(button); QVERIFY(button->isEnabled());
            for(auto *ancestor=button->parentItem();ancestor;ancestor=ancestor->parentItem()) {
                if(ancestor->metaObject()->indexOfProperty("contentY")<0) continue;
                const auto y=button->mapToItem(ancestor,QPointF()).y();
                ancestor->setProperty("contentY",qBound(0.,ancestor->property("contentY").toDouble()+y-ancestor->height()/2,
                    qMax(0.,ancestor->property("contentHeight").toDouble()-ancestor->height()))); break;
            }
            QTest::qWait(60);
            const auto center=button->mapToScene(QPointF(button->width()/2,button->height()/2)).toPoint();
            QTest::mousePress(window,Qt::LeftButton,Qt::NoModifier,center);
            QVERIFY(button->property("pressed").toBool()); QTest::qWait(180);
            QVERIFY(button->property("pressed").toBool());
            QTest::mouseRelease(window,Qt::LeftButton,Qt::NoModifier,center); QTest::qWait(30);
        };
        for(const auto &name:{"copyPlayerName","copyPlayerDetails","copyPlayerSessionId"}) {
            QGuiApplication::clipboard()->setText("unchanged"); click(find(name));
            QVERIFY2(QGuiApplication::clipboard()->text()!="unchanged",name);
        }
        click(find("openPlayerProfile")); QCOMPARE(browser.capture.url,PlayerProfiles::profileUrl("TestDriver"));
        {
            ProfileNetwork network; PlayerProfiles profiles(nullptr,&network);
            profiles.setPlayers("test-server",backend.players());
            auto *row=find("playerRow_id:0"); QVERIFY(row);
            row->setProperty("profiles",QVariant::fromValue(static_cast<QObject *>(&profiles)));
            click(find("copyPlayerForumId")); QVERIFY(profiles.resolving()); QVERIFY(network.reply);
            network.reply->complete(R"({"user":{"username":"TestDriver","id":73}})");
            QCOMPARE(QGuiApplication::clipboard()->text(),QString("73"));
            QVERIFY(row->property("actionMessage").toString().contains("73"));
            const auto screenshot=qEnvironmentVariable("BEAMSIDE_SESSION_SCREENSHOT");
            if (!screenshot.isEmpty()) QVERIFY(window->grabWindow().save(screenshot));
            row->setProperty("profiles",QVariant::fromValue(static_cast<QObject *>(backend.profiles())));
        }
        window->resize(320,440); QTest::qWait(100);
        click(find("copyPlayerName")); QCOMPARE(QGuiApplication::clipboard()->text(),QString("TestDriver"));
        window->resize(560,840); QTest::qWait(100);
        click(find("copyServerAddress")); QCOMPARE(QGuiApplication::clipboard()->text(),QString("203.0.113.7:30814"));
        click(find("sessionVehicles")); auto *owned=window->findChild<QObject *>("ownedDialog"); QVERIFY(owned && owned->property("visible").toBool());
        QTest::keyClick(window,Qt::Key_Escape); QTest::qWait(100);
        click(find("sessionDiagnostics")); auto *diagnostics=window->findChild<QObject *>("diagnosticsDialog"); QVERIFY(diagnostics && diagnostics->property("visible").toBool());
        QTest::keyClick(window,Qt::Key_Escape); QTest::qWait(100);
        click(find("sessionResetTrip")); auto *trip=window->findChild<QObject *>("tripDialog"); QVERIFY(trip && trip->property("visible").toBool());
        QVERIFY(backend.state().value("distance").toDouble()>0);
        data.insert("telemetry",QJsonObject{{"speed",0}}); f.send(data);
        QTRY_COMPARE(backend.state().value("telemetry").toMap().value("speed").toInt(),0);
        QVERIFY(QMetaObject::invokeMethod(trip,"accept")); QTRY_VERIFY(!backend.busy());
        QTRY_COMPARE(backend.actionMessage(),QString("Trip reset."));
        QCOMPARE(backend.state().value("distance").toDouble(),0.);
        // A verified multiplayer policy is deliberately not fabricated for
        // this UI test. Exercise selection in SP, then leave MP policy to the
        // strict bridge mocks and receiver policy tests.
        data.insert("multiplayer",false); data.insert("map","west_coast_usa"); f.send(data);
        QTRY_VERIFY(window->property("canDriveAction").toBool());
        click(find("sessionVehicles")); click(find("selectOwnedVehicle_42"));
        auto *confirmation=window->findChild<QObject *>("vehicleConfirmDialog"); QVERIFY(confirmation);
        QVERIFY(confirmation->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(confirmation,"accept"));
        QJsonObject command;
        QTRY_VERIFY(f.game.hasPendingDatagrams());
        while(f.game.hasPendingDatagrams()) { const auto next=f.command(); if(next.value("action")=="switch") command=next; }
        QCOMPARE(command.value("vehicleId").toInt(),42);
        f.send({{"kind","result"},{"requestId",command.value("requestId")},{"ok",true},{"message","Selection requested"}});
        QTRY_VERIFY(!backend.busy()); QCOMPARE(backend.actionMessage(),QString("Selection requested"));
        QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n')));
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
        QVERIFY(rpmApply->findChild<QQuickItem *>("glassButtonSurface"));
        QVERIFY(rpm->findChild<QQuickItem *>("glassFieldSurface"));
        rpm->setProperty("text","6000"); QTRY_VERIFY(rpmApply->isEnabled());
        QVERIFY(!rpm->property("invalidInput").toBool());
        rpm->setProperty("text","9000"); QTRY_VERIFY(!rpmApply->isEnabled());
        QVERIFY(rpm->property("invalidInput").toBool());
        QVERIFY(window->findChild<QQuickItem *>("clutchToggle")->isEnabled());
        auto *presetPanel = window->findChild<QQuickItem *>("presetPanel"); QVERIFY(presetPanel);
        QCOMPARE(presetPanel->property("backend").value<QObject *>(), static_cast<QObject *>(&backend));
        auto *section=window->findChild<QQuickItem *>("carSection"); QVERIFY(section); QCOMPARE(section->property("count").toInt(),5);
        auto *engineToggle=window->findChild<QQuickItem *>("engineToggle"); auto *strength=window->findChild<QQuickItem *>("strengthSlider");
        QVERIFY(engineToggle && strength); QVERIFY(strength->isEnabled());
        QVERIFY(strength->findChild<QQuickItem *>("glassSliderTrack"));
        QVERIFY(strength->findChild<QQuickItem *>("glassSliderHandle"));
        const auto oldStrength = strength->property("value").toDouble();
        strength->forceActiveFocus(); QTest::keyClick(window,Qt::Key_Right);
        QVERIFY(strength->property("value").toDouble() > oldStrength);
        engineToggle->setProperty("checked",true); QTRY_VERIFY(!strength->isEnabled());
        const auto disabledStrength = strength->property("value").toDouble();
        QTest::keyClick(window,Qt::Key_Right);
        QCOMPARE(strength->property("value").toDouble(),disabledStrength);
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
        const auto player = backend.players().first().toMap();
        // Repeater delegates are visual children, not necessarily QObject descendants.
        const auto rowName = "playerRow_" + player.value("rowKey").toString();
        QList<QQuickItem *> rowPending{window->contentItem()};
        QQuickItem *playerRow = nullptr;
        while (!rowPending.isEmpty()) {
            auto *item = rowPending.takeLast();
            if (item->objectName() == rowName) { playerRow = item; break; }
            rowPending.append(item->childItems());
        }
        QVERIFY(playerRow);
        QCOMPARE(playerRow->property("player").toMap().value("rawName"), player.value("rawName"));
        auto *copyName = playerRow->findChild<QQuickItem *>("copyPlayerName");
        auto *copyDetails = playerRow->findChild<QQuickItem *>("copyPlayerDetails");
        auto *copySession = playerRow->findChild<QQuickItem *>("copyPlayerSessionId");
        QVERIFY(copyName && copyName->isEnabled() && copyDetails && copyDetails->isEnabled() && copySession && copySession->isEnabled());
        QVERIFY(QMetaObject::invokeMethod(copyName,"clicked"));
        QCOMPARE(QGuiApplication::clipboard()->text(),player.value("rawName").toString());
        QVERIFY(QMetaObject::invokeMethod(copyDetails,"clicked"));
        QVERIFY(QGuiApplication::clipboard()->text().contains(player.value("rawName").toString()));
        QVERIFY(QGuiApplication::clipboard()->text().contains("Session ID:"));
        QVERIFY(QMetaObject::invokeMethod(copySession,"clicked"));
        QCOMPARE(QGuiApplication::clipboard()->text(),player.value("sessionPlayerId").toString());
        // A telemetry refresh between press and release must not replace the
        // delegate and swallow the activation.
        QPointer<QQuickItem> persistentCopy = copyName;
        QGuiApplication::clipboard()->setText("before refresh");
        for (auto *ancestor = copyName->parentItem(); ancestor; ancestor = ancestor->parentItem()) {
            if (ancestor->metaObject()->indexOfProperty("contentY") >= 0) {
                const auto y = copyName->mapToItem(ancestor,QPointF()).y();
                ancestor->setProperty("contentY",ancestor->property("contentY").toDouble() + y - ancestor->height()/2);
                break;
            }
        }
        QTest::qWait(50);
        const auto buttonCenter = copyName->mapToScene(QPointF(copyName->width()/2,copyName->height()/2)).toPoint();
        QTest::mousePress(window,Qt::LeftButton,Qt::NoModifier,buttonCenter);
        QVERIFY(persistentCopy);
        QVERIFY(persistentCopy->property("pressed").toBool());
        for (int refresh = 0; refresh < 12; ++refresh) {
            QVERIFY(QMetaObject::invokeMethod(&backend,"playersChanged"));
            QTest::qWait(16);
            QVERIFY(persistentCopy);
            QVERIFY(persistentCopy->property("pressed").toBool());
        }
        QTest::mouseRelease(window,Qt::LeftButton,Qt::NoModifier,buttonCenter);
        QCOMPARE(QGuiApplication::clipboard()->text(),player.value("rawName").toString());
        auto zeroId = player;
        zeroId.insert("sessionPlayerId",0);
        playerRow->setProperty("modelData",zeroId);
        QVERIFY(copySession->isEnabled());
        playerRow->setProperty("modelData",player);
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
