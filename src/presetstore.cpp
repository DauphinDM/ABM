#include "presetstore.h"
#include "protocol.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonDocument>
#include <QDateTime>
#include <QUuid>
#include <QRegularExpression>

namespace {
constexpr qint64 Limit=128*1024;
bool keys(const QJsonObject &o,const QStringList &allowed) { for(auto i=o.begin();i!=o.end();++i) if(!allowed.contains(i.key())) return false; return true; }
bool idValid(const QString &id) { return QRegularExpression("^[a-f0-9]{8}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{12}$").match(id).hasMatch(); }
}
PresetStore::PresetStore(const QString &directory,QObject *parent):QObject(parent),m_directory(directory) { reload(); }
void PresetStore::tell(const QString &message) { m_message=message; emit changed(); }
QString PresetStore::path(const QString &id) const { return idValid(id)?m_directory+'/'+id+".json":QString(); }
bool PresetStore::validSettings(const QJsonObject &settings,QString *reason) {
    bool valid=!settings.isEmpty() && keys(settings,{"power","beam","engine","rpm","clutch","handling","gears","dimensions"});
    for(auto i=settings.begin();i!=settings.end();++i) {
        if(!i.value().isObject()) { valid=false; break; }
        const auto o=i.value().toObject(); const auto k=i.key(); bool ok=false;
        if(k=="power") ok=keys(o,{"enginePower"}) && Protocol::enginePower(o.value("enginePower"));
        if(k=="beam") ok=keys(o,{"damageImmunity"}) && o.value("damageImmunity").isBool();
        if(k=="engine") ok=keys(o,{"engineStrength","engineIndestructible"}) && Protocol::engine(o);
        if(k=="rpm") ok=keys(o,{"maxRPM"}) && Protocol::rpm(o.value("maxRPM"));
        if(k=="clutch") ok=keys(o,{"clutchIndestructible"}) && o.value("clutchIndestructible").isBool();
        if(k=="handling") ok=keys(o,{"tireGrip","slidingGrip","brakePower","steeringResponse"}) && Protocol::handling(o);
        if(k=="gears") ok=keys(o,{"ratios"}) && Protocol::gears(o.value("ratios"));
        if(k=="dimensions") ok=keys(o,{"width","length","height"}) && Protocol::dimensions(o);
        valid &= ok;
    }
    if(!valid && reason) *reason=QObject::tr("Unknown, empty or invalid preset settings. Only fixed vehicle adapters are accepted.");
    return valid;
}
bool PresetStore::validate(const QJsonObject &o,QString *reason) {
    const auto config=o.value("configuration").toObject();
    bool ok=keys(o,{"schemaVersion","id","name","created","modified","configuration","settings","tags","notes","requirements"})
        && o.value("schemaVersion").toInt()==1 && idValid(o.value("id").toString())
        && !o.value("name").toString().trimmed().isEmpty() && o.value("name").toString().size()<=80
        && Protocol::model(config.value("model").toString()) && !config.value("identity").toString().isEmpty()
        && config.value("identity").toString().size()<=160 && keys(config,{"model","identity","label","completeness"})
        && config.value("label").toString().size()<=160 && config.value("completeness").toString()=="workshop-overrides"
        && o.value("requirements").toString()=="ABM-0.11-runtime-1"
        && o.value("tags").toString().size()<=256 && o.value("notes").toString().size()<=4096
        && QJsonDocument(o).toJson(QJsonDocument::Compact).size()<=Limit && validSettings(o.value("settings").toObject());
    if(!ok && reason) *reason=QObject::tr("Preset schema, compatibility descriptor or settings invalid. Future/unknown fields are refused.");
    return ok;
}
QJsonObject PresetStore::get(const QString &id) const {
    const auto p=path(id); if(p.isEmpty() || QFileInfo(p).isSymLink()) return {};
    QFile f(p); if(!f.open(QIODevice::ReadOnly) || f.size()>Limit) return {};
    const auto o=QJsonDocument::fromJson(f.read(Limit+1)).object();
    return validate(o) && o.value("id")==id?o:QJsonObject();
}
void PresetStore::reload() {
    m_entries.clear();
    const auto files=QDir(m_directory).entryList({"*.json"},QDir::Files,QDir::Name);
    for(const auto &file:files.mid(0,256)) {
        const auto id=file.chopped(5); const auto o=get(id);
        m_entries.append(o.isEmpty()?QVariantMap{{"id",id},{"name",file},{"error",tr("Invalid or oversized preset; file preserved.")}}:o.toVariantMap());
    }
    if(files.size()>256) m_message=tr("Preset library limit reached (256); extra files are preserved but not listed.");
    emit changed();
}
bool PresetStore::write(QJsonObject o,bool replace) {
    QString error;
    if(!validate(o,&error)) { tell(error); return false; }
    if(!QDir().mkpath(m_directory)) { tell(tr("Cannot create preset directory.")); return false; }
    const auto p=path(o.value("id").toString());
    if(QFileInfo(p).isSymLink() || (!replace && QFileInfo::exists(p))) { tell(tr("Preset destination exists or is a symbolic link.")); return false; }
    if(!replace && QDir(m_directory).entryList({"*.json"},QDir::Files).size()>=256) { tell(tr("Preset library is full.")); return false; }
    // A small history stores only ABM data; it never replaces the live file on failure.
    if(replace && QFileInfo::exists(p)) {
        const auto history=m_directory+"/history"; QDir().mkpath(history);
        const auto backup=history+'/'+QUuid::createUuid().toString(QUuid::WithoutBraces)+".json";
        if(!QFile::copy(p,backup)) { tell(tr("Could not preserve previous preset; update cancelled.")); return false; }
        auto old=QDir(history).entryList({"*.json"},QDir::Files,QDir::Time);
        for(int i=32;i<old.size();++i) QFile::remove(history+'/'+old[i]);
    }
    QSaveFile f(p); const auto bytes=QJsonDocument(o).toJson(QJsonDocument::Indented);
    if(!f.open(QIODevice::WriteOnly)) { tell(f.errorString()); return false; }
    f.setPermissions(QFile::ReadOwner|QFile::WriteOwner);
    if(f.write(bytes)!=bytes.size() || !f.commit()) { tell(tr("Preset write failed; previous file retained.")); return false; }
    reload(); tell(tr("Preset saved. It contains accepted ABM overrides, not a complete native vehicle clone.")); return true;
}
QString PresetStore::save(const QString &name,const QJsonObject &configuration,const QJsonObject &settings) {
    const auto id=QUuid::createUuid().toString(QUuid::WithoutBraces), now=QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    const QJsonObject o{{"schemaVersion",1},{"id",id},{"name",name.trimmed()},{"created",now},{"modified",now},
        {"configuration",configuration},{"settings",settings},{"requirements","ABM-0.11-runtime-1"},{"tags",""},{"notes",""}};
    return write(o,false)?id:QString();
}
bool PresetStore::renamePreset(const QString &id,const QString &name) { auto o=get(id); if(o.isEmpty()) return false; o.insert("name",name.trimmed()); o.insert("modified",QDateTime::currentDateTimeUtc().toString(Qt::ISODate)); return write(o,true); }
bool PresetStore::duplicate(const QString &id) { auto o=get(id); if(o.isEmpty()) return false; o.insert("id",QUuid::createUuid().toString(QUuid::WithoutBraces)); o.insert("name",(o.value("name").toString()+tr(" copy")).left(80)); return write(o,false); }
bool PresetStore::remove(const QString &id,bool confirmed) { if(!confirmed || get(id).isEmpty()) return false; const bool ok=QFile::remove(path(id)); reload(); tell(ok?tr("Preset deleted."):tr("Preset deletion failed.")); return ok; }
bool PresetStore::annotate(const QString &id,const QString &tags,const QString &notes) { auto o=get(id); if(o.isEmpty()) return false; o.insert("tags",tags); o.insert("notes",notes); return write(o,true); }
QString PresetStore::exportJson(const QString &id) const { const auto o=get(id); return o.isEmpty()?QString():QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Indented)); }
bool PresetStore::importJson(const QString &text) { if(text.toUtf8().size()>Limit) { tell(tr("Import exceeds 128 KiB.")); return false; } auto o=QJsonDocument::fromJson(text.toUtf8()).object(); QString error; if(!validate(o,&error)) { tell(error); return false; } o.insert("id",QUuid::createUuid().toString(QUuid::WithoutBraces)); return write(o,false); }
