#include "appupdates.h"
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QVersionNumber>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QDesktopServices>
#include <QFileInfo>
#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <QCoreApplication>
#endif

namespace {
constexpr qint64 MaxPackage=512LL*1024*1024;
bool safeUrl(const QUrl &u) { return u.isValid() && u.scheme()=="https" && u.userInfo().isEmpty() && u.port(-1)==-1 && u.fragment().isEmpty(); }
QString platform() {
#ifdef Q_OS_ANDROID
    return "android";
#elif defined(Q_OS_WIN)
    return "windows";
#else
    return "linux";
#endif
}
bool packageName(const QString &name, const QString &os) {
    if (!QRegularExpression("^[A-Za-z0-9][A-Za-z0-9._-]{0,159}$").match(name).hasMatch()) return false;
    if (os=="android") return name.startsWith("ABM-Mobile-") && name.endsWith("-Android-arm64.apk");
    if (os=="windows") return name.startsWith("ABM-") && name.endsWith("-Windows-x64.zip");
    return name.startsWith("ABM-") && name.endsWith("-x86_64.AppImage");
}
}
AppUpdates::AppUpdates(bool mobile, QString current, QString directory, QObject *parent, QNetworkAccessManager *network)
    : QObject(parent), m_repo(mobile ? "ABM-Mobile" : "ABM"), m_current(current), m_directory(directory), m_platform(platform()), m_network(network ? network : new QNetworkAccessManager(this)) {
    if (m_directory.isEmpty()) {
        auto root=qEnvironmentVariable(mobile ? "ABM_MOBILE_CONFIG_DIR" : "BEAMSIDE_CONFIG_DIR");
        if (root.isEmpty()) root=QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        m_directory=QDir(root).filePath("updates");
    }
    QSettings s(QDir(m_directory).filePath("preferences.ini"),QSettings::IniFormat);
    m_automatic=s.value("automatic",false).toBool(); m_previews=s.value("previews",false).toBool(); m_autoDownload=s.value("autoDownload",false).toBool();
    m_message=tr("Checks contact GitHub. Enable automatic checks or choose Check now.");
    m_daily.setInterval(24*60*60*1000);
    connect(&m_daily,&QTimer::timeout,this,[this] { if (m_automatic) check(); });
}
void AppUpdates::savePreferences() {
    if (!QDir().mkpath(m_directory)) { m_message=tr("Cannot save update preferences."); emit changed(); return; }
    QSettings s(QDir(m_directory).filePath("preferences.ini"),QSettings::IniFormat);
    s.setValue("automatic",m_automatic); s.setValue("previews",m_previews); s.setValue("autoDownload",m_autoDownload); s.sync();
    if (s.status()!=QSettings::NoError) m_message=tr("Cannot save update preferences.");
    emit changed();
}
AppUpdates::~AppUpdates() { if (m_reply) { m_reply->disconnect(this); m_reply->abort(); m_reply->deleteLater(); } }
void AppUpdates::setAutomatic(bool v) { if (v==m_automatic) return; m_automatic=v; savePreferences(); if (v) check(); }
void AppUpdates::setPreviews(bool v) { if (busy() || v==m_previews) return; m_previews=v; m_release={}; m_download.clear(); savePreferences(); }
void AppUpdates::setAutoDownload(bool v) { m_autoDownload=v; savePreferences(); }
void AppUpdates::start() { m_daily.start(); if (m_automatic) QTimer::singleShot(10000,this,[this] { if(m_automatic) check(); }); }
bool AppUpdates::trustedDownload(const QUrl &u) {
    return safeUrl(u) && (u.host()=="github.com" || u.host()=="release-assets.githubusercontent.com" || u.host()=="objects.githubusercontent.com");
}
AppUpdates::Release AppUpdates::selectRelease(const QJsonArray &feed, const QString &repo, const QString &current, bool previews, const QString &os) {
    Release best; auto version=QVersionNumber::fromString(current);
    if (repo!="ABM" && repo!="ABM-Mobile") return best;
    const QString base="https://github.com/DauphinDM/"+repo+"/releases/";
    for (int i=0;i<qMin(feed.size(),100);++i) {
        const auto r=feed.at(i).toObject(); const auto tag=r.value("tag_name").toString();
        const auto match=QRegularExpression("^v([0-9]+\\.[0-9]+\\.[0-9]+)(?:-(?:preview|beta|rc)\\.[0-9]+)?$").match(tag);
        if (!match.hasMatch() || r.value("draft").toBool() || (!previews && (r.value("prerelease").toBool() || tag.contains('-')))) continue;
        const auto v=QVersionNumber::fromString(match.captured(1)); if (v.segmentCount()!=3 || v<=version) continue;
        const QUrl page(r.value("html_url").toString());
        if (page.toString()!=base+"tag/"+tag || !safeUrl(page)) continue;
        Release candidate; candidate.version=match.captured(1); candidate.page=page;
        for (const auto &asset : r.value("assets").toArray()) {
            const auto a=asset.toObject(); const auto name=a.value("name").toString(); const QUrl url(a.value("browser_download_url").toString());
            const auto digest=a.value("digest").toString(); const auto size=a.value("size").toDouble();
            if (!packageName(name,os) || !name.contains("-"+candidate.version+"-") || url.toString()!=base+"download/"+tag+"/"+name || !safeUrl(url) || size<1 || size>MaxPackage || size!=qint64(size)) continue;
            if (!QRegularExpression("^sha256:[a-f0-9]{64}$").match(digest).hasMatch()) continue;
            candidate.name=name; candidate.asset=url; candidate.sha256=digest.mid(7); candidate.size=qint64(size); break;
        }
        best=candidate; version=v;
    }
    return best;
}
void AppUpdates::check() {
    if (busy()) return;
    m_release={}; m_download.clear(); m_progress=0; m_feed.clear(); m_message=tr("Checking GitHub releases…");
    request(QUrl("https://api.github.com/repos/DauphinDM/"+m_repo+"/releases?per_page=20"),false);
}
void AppUpdates::download() {
    if (busy() || !available() || m_release.asset.isEmpty()) return;
    if (!QDir().mkpath(m_directory)) { fail(tr("Cannot create the update folder.")); return; }
    m_download.clear(); m_hash.reset(); m_received=0; m_progress=0;
    m_file=std::make_unique<QSaveFile>(QDir(m_directory).filePath(m_release.name));
    if (!m_file->open(QIODevice::WriteOnly)) { fail(tr("Cannot save the update.")); return; }
    m_file->setPermissions(QFileDevice::ReadOwner|QFileDevice::WriteOwner);
    m_message=tr("Downloading and checking SHA-256…"); request(m_release.asset,true);
}
void AppUpdates::fail(const QString &text) {
    auto reply=m_reply; m_reply=nullptr;
    if (reply) { reply->disconnect(this); reply->abort(); reply->deleteLater(); }
    m_file.reset(); m_message=text; emit changed();
}
void AppUpdates::cancel() { fail(tr("Transfer cancelled. No installation was performed.")); }
void AppUpdates::request(const QUrl &url, bool asset, int redirects) {
    QNetworkRequest req(url); req.setTransferTimeout(30000); req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::ManualRedirectPolicy);
    req.setRawHeader("User-Agent","ABM-release-updater"); req.setRawHeader("Accept",asset ? "application/octet-stream" : "application/vnd.github+json");
    auto *reply=m_network->get(req); m_reply=reply; reply->setReadBufferSize(128*1024); emit changed();
    QTimer::singleShot(asset ? 600000 : 30000,reply,[reply] { if(!reply->isFinished()) reply->abort(); });
    connect(reply,&QIODevice::readyRead,this,[this,reply,asset] {
        if (m_reply!=reply) return;
        const int status=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status!=200) { reply->readAll(); return; }
        const auto bytes=reply->readAll();
        if (!asset) {
            if (m_feed.size()+bytes.size()>1024*1024) { fail(tr("Release response is too large.")); return; }
            m_feed+=bytes;
        } else {
            m_received+=bytes.size();
            if (m_received>m_release.size || !m_file || m_file->write(bytes)!=bytes.size()) { fail(tr("Update size or disk write failed.")); return; }
            m_hash.addData(bytes); m_progress=double(m_received)/m_release.size; emit changed();
        }
    });
    connect(reply,&QNetworkReply::finished,this,[this,reply,asset,redirects] {
        if (m_reply!=reply) return;
        const auto redirect=reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl(); const int status=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (!redirect.isEmpty() && asset && redirects<3 && (status==301 || status==302 || status==303 || status==307 || status==308)) {
            const auto next=reply->url().resolved(redirect);
            if (!trustedDownload(next)) { fail(tr("Update redirected outside allowed HTTPS hosts.")); return; }
            m_reply=nullptr; reply->deleteLater(); request(next,true,redirects+1); return;
        }
        if (reply->error()!=QNetworkReply::NoError || status!=200) { fail(tr("GitHub update request failed. Check the connection or try later (rate limits may apply).")); return; }
        m_reply=nullptr; reply->deleteLater();
        if (!asset) {
            QJsonParseError error; const auto doc=QJsonDocument::fromJson(m_feed,&error);
            if (error.error!=QJsonParseError::NoError || !doc.isArray()) { fail(tr("Invalid release response.")); return; }
            m_release=selectRelease(doc.array(),m_repo,m_current,m_previews,m_platform);
            m_message=available() ? tr("Update %1 is available.").arg(version()) : tr("No newer published release in this channel. Development commits and CI artifacts are not updates.");
            if (available() && !canDownload()) m_message+=tr(" No compatible SHA-256 verified package is published; open the release page for instructions.");
            emit changed(); if (available() && m_autoDownload) download();
        } else {
            if (m_received!=m_release.size || QString::fromLatin1(m_hash.result().toHex())!=m_release.sha256 || !m_file->commit()) { fail(tr("Update verification failed. The download was not accepted.")); return; }
            m_download=QDir(m_directory).filePath(m_release.name); m_file.reset(); m_message=tr("Verified download ready. Installation requires your action; game control is not interrupted automatically."); emit changed();
        }
    });
}
void AppUpdates::openRelease() {
    if (available() && !QDesktopServices::openUrl(m_release.page)) { m_message=tr("Could not open the release page."); emit changed(); }
}
QString AppUpdates::installHelp() const {
#ifdef Q_OS_ANDROID
    return tr("Android asks permission to install. Updates must use the same app ID and signing certificate. Never uninstall automatically to bypass a signature mismatch.");
#elif defined(Q_OS_WIN)
    return tr("Open the verified ZIP, close ABM, and extract it into a new folder. ZIP builds have no silent installer. Keep all runtime files together.");
#else
    return tr("For an Arch installation, update through pacman/your package manager. For portable builds, make the downloaded AppImage executable and launch it after closing ABM. Package-managed files are never overwritten.");
#endif
}
void AppUpdates::openDownload() {
    if (!downloaded() || busy()) return;
    QFile file(m_download);
    if (!file.open(QIODevice::ReadOnly)) { fail(tr("Downloaded file is no longer available.")); return; }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file) || QString::fromLatin1(hash.result().toHex())!=m_release.sha256) { m_download.clear(); fail(tr("Downloaded file changed. Download it again.")); return; }
    file.close();
#ifdef Q_OS_ANDROID
    const auto context=QNativeInterface::QAndroidApplication::context();
    const auto result=QJniObject::callStaticObjectMethod("io/github/dauphindm/abmmobile/UpdateProvider","offerInstall","(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",context.object(),QJniObject::fromString(m_download).object(),QJniObject::fromString(m_release.sha256).object());
    m_message=result.toString(); emit changed();
#else
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(m_download))) { m_message=tr("Could not open the download. Find it in %1.").arg(m_directory); emit changed(); }
#endif
}
