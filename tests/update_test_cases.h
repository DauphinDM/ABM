#pragma once
#include "appupdates.h"
#include <QtTest>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>

namespace UpdateTests {
class Reply : public QNetworkReply {
    QByteArray body;
public:
    Reply(const QNetworkRequest &r,QObject *parent) : QNetworkReply(parent) { setUrl(r.url()); setRequest(r); open(QIODevice::ReadOnly); }
    void complete(QByteArray data,int status=200,QUrl redirect={}) {
        if (isFinished()) return;
        body=data; setAttribute(QNetworkRequest::HttpStatusCodeAttribute,status);
        if (!redirect.isEmpty()) setAttribute(QNetworkRequest::RedirectionTargetAttribute,redirect);
        setFinished(true); emit readyRead(); emit finished();
    }
    void abort() override { if(isFinished()) return; setError(OperationCanceledError,"Cancelled"); setFinished(true); emit finished(); }
    qint64 bytesAvailable() const override { return body.size()+QNetworkReply::bytesAvailable(); }
    qint64 readData(char *data,qint64 size) override { auto n=qMin(size,qint64(body.size())); if(!n)return -1; memcpy(data,body.constData(),size_t(n)); body.remove(0,n); return n; }
};
class Network : public QNetworkAccessManager {
public:
    QPointer<Reply> reply;
    QList<QNetworkRequest> requests;
    QNetworkReply *createRequest(Operation op,const QNetworkRequest &r,QIODevice *) override {
        Q_ASSERT(op==GetOperation); requests.append(r); reply=new Reply(r,this); return reply;
    }
};
inline QJsonObject release(QString repo="ABM",QString tag="v1.2.0",QString os="linux",QByteArray payload="fixture update") {
    const auto version=tag.mid(1).section('-',0,0);
    const auto name=os=="android" ? "ABM-Mobile-"+version+"-Android-arm64.apk" : os=="windows" ? "ABM-"+version+"-Windows-x64.zip" : "ABM-"+version+"-x86_64.AppImage";
    const auto base="https://github.com/DauphinDM/"+repo+"/releases/";
    return {{"tag_name",tag},{"html_url",base+"tag/"+tag},{"draft",false},{"prerelease",false},
        {"assets",QJsonArray{QJsonObject{{"name",name},{"browser_download_url",base+"download/"+tag+"/"+name},{"size",payload.size()},
             {"digest","sha256:"+QString::fromLatin1(QCryptographicHash::hash(payload,QCryptographicHash::Sha256).toHex())}}}}};
}
inline void selection() {
    for(const auto &os:{QString("linux"),QString("windows"),QString("android")}) {
        auto r=release("ABM", "v1.2.0",os);
        auto selected=AppUpdates::selectRelease({r},"ABM","1.1.9",false,os);
        QCOMPARE(selected.version,QString("1.2.0")); QVERIFY(!selected.asset.isEmpty());
        QVERIFY(AppUpdates::selectRelease({r},"ABM","1.2.0",true,os).version.isEmpty());
        r["draft"]=true; QVERIFY(AppUpdates::selectRelease({r},"ABM","0.1.0",true,os).version.isEmpty());
        r["draft"]=false; r["prerelease"]=true;
        QVERIFY(AppUpdates::selectRelease({r},"ABM","0.1.0",false,os).version.isEmpty());
        QVERIFY(!AppUpdates::selectRelease({r},"ABM","0.1.0",true,os).version.isEmpty());
    }
    auto bad=release(); bad["html_url"]="https://github.com/attacker/ABM/releases/tag/v1.2.0";
    QVERIFY(AppUpdates::selectRelease({bad},"ABM","0.1.0",true,"linux").version.isEmpty());
    for(const auto &key:{QString("digest"),QString("browser_download_url"),QString("name"),QString("size")}) {
        bad=release(); auto asset=bad["assets"].toArray().first().toObject();
        asset[key]=key=="size" ? QJsonValue(900000000) : QJsonValue("../../evil"); bad["assets"]=QJsonArray{asset};
        const auto selected=AppUpdates::selectRelease({bad},"ABM","0.1.0",true,"linux");
        QCOMPARE(selected.version,QString("1.2.0")); QVERIFY(selected.asset.isEmpty());
    }
    for(const auto &url:{"http://github.com/a","https://github.com.evil/a","https://user@github.com/a","https://github.com:443/a","https://example.com/a","file:///tmp/a"}) QVERIFY(!AppUpdates::trustedDownload(QUrl(url)));
    QVERIFY(AppUpdates::trustedDownload(QUrl("https://release-assets.githubusercontent.com/a?token=example")));
    QVERIFY(AppUpdates::selectRelease({release("ABM-Mobile")},"ABM","0.1.0",true,"linux").version.isEmpty());
}
inline void transfers() {
    QTemporaryDir dir; QVERIFY(dir.isValid()); Network network;
    AppUpdates updates(false,"0.1.0",dir.path(),nullptr,&network);
    QVERIFY(!updates.automatic()); QVERIFY(network.requests.isEmpty());
    updates.check(); QVERIFY(updates.busy());
    QVERIFY(network.requests.last().url().toString().startsWith("https://api.github.com/repos/DauphinDM/ABM/releases?"));
#ifdef Q_OS_WIN
    const QString os="windows";
#elif defined(Q_OS_ANDROID)
    const QString os="android";
#else
    const QString os="linux";
#endif
    auto response=release("ABM","v1.2.0",os);
    const auto feed=QJsonDocument(QJsonArray{response}).toJson();
    network.reply->complete(feed); QVERIFY(updates.available()); QVERIFY(!updates.busy()); QVERIFY(updates.canDownload());
    updates.download(); QVERIFY(updates.busy()); network.reply->complete("fixture update");
    QVERIFY(updates.downloaded()); QVERIFY(!updates.busy()); QCOMPARE(updates.progress(),1.0);
    const auto name=response["assets"].toArray().first().toObject()["name"].toString();
    QFile file(dir.filePath(name)); QVERIFY(file.open(QIODevice::ReadOnly)); QCOMPARE(file.readAll(),QByteArray("fixture update")); file.close();
    updates.download(); network.reply->complete("wrong bytes!!!"); QVERIFY(!updates.downloaded()); QVERIFY(updates.message().contains("verification",Qt::CaseInsensitive));
    updates.download(); network.reply->complete({},302,QUrl("https://attacker.invalid/update")); QVERIFY(!updates.busy()); QVERIFY(!updates.downloaded());
    updates.download(); network.reply->complete({},302,QUrl("https://release-assets.githubusercontent.com/fixture"));
    QVERIFY(updates.busy()); network.reply->complete("fixture update"); QVERIFY(updates.downloaded());
    updates.download(); updates.cancel(); QVERIFY(!updates.busy()); QVERIFY(!updates.downloaded());
    updates.check(); network.reply->complete(QByteArray(1024*1024+1,'x')); QVERIFY(!updates.available()); QVERIFY(!updates.busy());
    updates.check(); network.reply->complete("[]",403); QVERIFY(!updates.available()); QVERIFY(!updates.busy());
    updates.setAutomatic(true); QVERIFY(updates.busy()); network.reply->complete("[]");
    updates.setPreviews(true); updates.setAutoDownload(true);
    AppUpdates restored(false,"0.1.0",dir.path(),nullptr,&network);
    QVERIFY(restored.automatic()); QVERIFY(restored.previews()); QVERIFY(restored.autoDownload());
    QCOMPARE(network.requests.last().attribute(QNetworkRequest::RedirectPolicyAttribute).toInt(),int(QNetworkRequest::ManualRedirectPolicy));
}
inline void panel() {
    QTemporaryDir dir; QVERIFY(dir.isValid()); Network network;
    AppUpdates updates(false,"0.1.0",dir.path(),nullptr,&network);
    QQmlEngine engine;
    QStringList warnings;
    QObject::connect(&engine,&QQmlEngine::warnings,&engine,[&](const QList<QQmlError> &errors) { for(const auto &e:errors) warnings.append(e.toString()); });
    engine.rootContext()->setContextProperty("testUpdater",&updates);
    engine.rootContext()->setContextProperty("theme",QVariantMap{{"accent",QColor("#ffd60a")},{"radius",6}});
    QQuickWindow window;
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nimport QtQuick.Controls\nimport \"qrc:/\"\nUpdatesPane { width: 380; updater: testUpdater }",QUrl("qrc:/UpdatesTest.qml"));
    QScopedPointer<QObject> object(component.create()); QVERIFY2(object,qPrintable(component.errorString()));
    auto *item=qobject_cast<QQuickItem*>(object.data()); QVERIFY(item);
    window.resize(400,680); item->setParentItem(window.contentItem()); window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto *button=object->findChild<QQuickItem*>("checkAppUpdates"); QVERIFY(button); QVERIFY(button->isEnabled()); QVERIFY(network.requests.isEmpty());
    QTest::mouseClick(&window,Qt::LeftButton,Qt::NoModifier,button->mapToScene(QPointF(button->width()/2,button->height()/2)).toPoint());
    QTRY_COMPARE(network.requests.size(),1); QVERIFY(updates.busy()); QTRY_VERIFY(!button->isEnabled());
    network.reply->complete("[]"); QTRY_VERIFY(button->isEnabled()); QVERIFY(!updates.automatic());
    if(qEnvironmentVariableIsSet("ABM_UPDATES_SCREENSHOT")) QVERIFY(window.grabWindow().save(qEnvironmentVariable("ABM_UPDATES_SCREENSHOT")));
    QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n')));
}
}
