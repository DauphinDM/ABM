#include "usbtunnel.h"
#include "mobilehost.h"
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QGuiApplication>
#include <QClipboard>
#include <QScopedValueRollback>
#include <QRegularExpression>

UsbTunnel::UsbTunnel(MobileHost *host, QObject *parent) : QObject(parent),m_host(host) {
    for (auto *socket : {&m_phone,&m_local}) {
        socket->setReadBufferSize(256*1024);
        connect(socket,&QTcpSocket::disconnected,this,&UsbTunnel::resetBridge);
        connect(socket,&QTcpSocket::errorOccurred,this,[this] { resetBridge(); });
        connect(socket,&QTcpSocket::connected,this,[socket] { socket->setSocketOption(QAbstractSocket::LowDelayOption,1); });
    }
    connect(&m_phone,&QTcpSocket::connected,this,[this] { m_local.connectToHost(QHostAddress::LocalHost,m_host->listenPort()); });
    connect(&m_local,&QTcpSocket::connected,this,[this] { pump(m_phone,m_local); pump(m_local,m_phone); });
    connect(&m_phone,&QTcpSocket::readyRead,this,[this] { pump(m_phone,m_local); });
    connect(&m_local,&QTcpSocket::readyRead,this,[this] { pump(m_local,m_phone); });
    connect(&m_phone,&QTcpSocket::bytesWritten,this,[this] { pump(m_local,m_phone); });
    connect(&m_local,&QTcpSocket::bytesWritten,this,[this] { pump(m_phone,m_local); });
    m_retry.setInterval(1000);
    m_startup.setSingleShot(true); m_startup.setInterval(10000);
    connect(&m_startup,&QTimer::timeout,this,[this] { stop(); m_message=tr("USB helper timed out. Check device authorization and retry."); emit changed(); });
    connect(&m_retry,&QTimer::timeout,this,[this] {
        if (m_running && m_phone.state()==QAbstractSocket::UnconnectedState) m_phone.connectToHost(QHostAddress::LocalHost,44448);
    });
    connect(host,&MobileHost::changed,this,[this] { if (m_running && !m_host->running()) stop(); emit changed(); });
    connect(&m_proxy,&QProcess::readyReadStandardOutput,this,[this] {
        m_output+=m_proxy.readAllStandardOutput();
        if (m_output.size()>16384) { stop(); m_message=tr("USB helper output exceeded its limit."); emit changed(); }
    });
    connect(&m_proxy,&QProcess::readyReadStandardError,this,[this] { m_proxy.readAllStandardError(); });
    connect(&m_proxy,&QProcess::errorOccurred,this,[this](QProcess::ProcessError error) {
        if (error==QProcess::FailedToStart) { stop(); m_message=tr("USB helper could not start. Install iproxy (iPhone) or adb (Android)."); emit changed(); }
    });
    connect(&m_proxy,&QProcess::started,this,[this] {
        if (m_mode=="iphone") { m_startup.stop(); m_running=true; m_retry.start(); m_message=tr("USB relay enabled. Unlock and trust the PC on iPhone, then open USB mode in ABM Mobile. No firewall opening is needed."); emit changed(); }
    });
    connect(&m_proxy,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int code,QProcess::ExitStatus status) {
        if (m_mode=="android" && code==0 && status==QProcess::NormalExit) {
            m_output+=m_proxy.readAllStandardOutput();
            if (m_stage=="serial") {
                m_serial=QString::fromUtf8(m_output).trimmed();
                if (m_serial.isEmpty() || m_serial.size()>128 || m_serial.contains(QRegularExpression("[\\s\\x00-\\x1f]")) || m_serial=="unknown") {
                    stop(); m_message=tr("Authorize exactly one Android USB device and retry."); emit changed(); return;
                }
                m_stage="list"; m_output.clear(); m_proxy.start(m_tool,{"-s",m_serial,"reverse","--list"});
            } else if (m_stage=="list") {
                for (const auto &line:m_output.split('\n')) {
                    const auto fields=line.simplified().split(' ');
                    if (fields.size()>=3 && fields[1]=="tcp:44445") { stop(); m_message=tr("Android already has a reverse mapping on port 44445. ABM will not replace it."); emit changed(); return; }
                }
                m_stage="add"; m_output.clear(); m_proxy.start(m_tool,{"-s",m_serial,"reverse","--no-rebind","tcp:44445","tcp:44445"});
            } else if (m_stage=="add") {
                m_startup.stop(); m_ownedReverse=true; m_stage="active"; m_running=true;
                m_message=tr("Android USB reverse tunnel ready. Paste the link on the phone. USB debugging authorization is required."); emit changed();
            }
        } else if (!m_mode.isEmpty()) {
            stop(); m_message=tr("USB helper stopped or failed. Check cable, device trust, drivers and helper availability."); emit changed();
        }
    });
}
UsbTunnel::~UsbTunnel() { disconnect(&m_proxy,nullptr,this,nullptr); stop(); }
void UsbTunnel::resetBridge() {
    if (m_resetting) return;
    QScopedValueRollback<bool> guard(m_resetting,true); m_phone.abort(); m_local.abort();
}
void UsbTunnel::pump(QTcpSocket &from, QTcpSocket &to) {
    if (to.state()!=QAbstractSocket::ConnectedState || !from.bytesAvailable()) return;
    if (to.bytesToWrite()>256*1024) { resetBridge(); return; }
    to.write(from.read(qMin<qint64>(16384,from.bytesAvailable())));
    if (from.bytesAvailable()) QTimer::singleShot(0,this,[this,&from,&to] { pump(from,to); });
}
bool UsbTunnel::start(const QString &mode) {
    stop();
    if (mode!="iphone" && mode!="android") return false;
    const auto tool=QStandardPaths::findExecutable(mode=="iphone"?"iproxy":"adb");
    if (tool.isEmpty()) { m_message=tr("Install the optional iproxy/libusbmuxd helper for iPhone, or adb for Android. ABM does not install drivers or enable USB debugging automatically."); emit changed(); return false; }
    if (!m_host->start("127.0.0.1",44445)) { m_message=m_host->message(); emit changed(); return false; }
    m_mode=mode; m_tool=tool; m_stage=mode=="android"?"serial":"proxy"; m_output.clear(); m_message=tr("Starting explicit USB transport…");
    m_startup.start(); m_proxy.start(tool,mode=="iphone" ? QStringList{"-l","-s","127.0.0.1","44448:44446"} : QStringList{"-d","get-serialno"});
    emit changed(); return true;
}
void UsbTunnel::stop() {
    const bool owned=!m_mode.isEmpty(); m_mode.clear(); m_running=false; m_retry.stop(); m_startup.stop(); resetBridge();
    if (m_proxy.state()!=QProcess::NotRunning) { m_proxy.terminate(); if (!m_proxy.waitForFinished(1000)) m_proxy.kill(); }
    if (m_ownedReverse && !m_serial.isEmpty()) {
        // Only remove our exact mapping on the same physical device. Never use
        // --remove-all or follow a newly attached/default Android device.
        QProcess cleanup; cleanup.start(m_tool,{"-s",m_serial,"reverse","--list"});
        if (cleanup.waitForFinished(2000) && cleanup.exitCode()==0) {
            for (const auto &line:cleanup.readAllStandardOutput().split('\n')) {
                const auto fields=line.simplified().split(' ');
                if (fields.size()==3 && fields[1]=="tcp:44445" && fields[2]=="tcp:44445") {
                    cleanup.start(m_tool,{"-s",m_serial,"reverse","--remove","tcp:44445"}); cleanup.waitForFinished(2000); break;
                }
            }
        }
    }
    m_ownedReverse=false; m_serial.clear(); m_stage.clear(); m_output.clear();
    if (owned && m_host->running()) m_host->stop();
    m_message=tr("USB transport off. PC listener closed; remembered credentials are unchanged."); emit changed();
}
QString UsbTunnel::pairingLink() const {
    if (!m_running || m_host->pairingLink().isEmpty()) return {};
    auto value=QJsonDocument::fromJson(QByteArray::fromBase64(m_host->pairingLink().mid(11).toLatin1(),QByteArray::Base64UrlEncoding)).object();
    value.insert("host","127.0.0.1"); value.insert("port",m_mode=="iphone"?44447:44445); value.insert("transport",m_mode=="iphone"?"usb-iphone":"usb-android");
    return "abm://pair/"+QString::fromLatin1(QJsonDocument(value).toJson(QJsonDocument::Compact).toBase64(QByteArray::Base64UrlEncoding|QByteArray::OmitTrailingEquals));
}
void UsbTunnel::copyPairingLink() { if (!pairingLink().isEmpty() && QGuiApplication::clipboard()) QGuiApplication::clipboard()->setText(pairingLink()); }
