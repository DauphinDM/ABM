#pragma once
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <cmath>

namespace Protocol {
// Tuning-range checks shared by the service and the test suite. The vehicle
// bridge re-validates, so these are the outer fence only.
inline bool enginePower(const QJsonValue &value) {
    const auto v = value.toDouble();
    return value.isDouble() && std::isfinite(v) && v > 0;
}
inline bool rpm(const QJsonValue &value) {
    const auto n=value.toDouble();
    return value.isDouble() && std::isfinite(n) && n>=100 && n<=30000 && n==std::floor(n);
}
inline bool handling(const QJsonObject &data) {
    for (const auto key : {"tireGrip", "slidingGrip", "brakePower", "steeringResponse"}) {
        const auto value = data.value(key);
        const auto n = value.toDouble(-1);
        if (!value.isDouble() || !std::isfinite(n) || n < (QString(key) == "steeringResponse" ? .1 : 0)
            || n > (QString(key) == "steeringResponse" ? 3 : 10)) return false;
    }
    return true;
}
inline bool engine(const QJsonObject &data) {
    const auto strength = data.value("engineStrength");
    return strength.isDouble() && std::isfinite(strength.toDouble()) && strength.toDouble() >= .1
        && strength.toDouble() <= 100 && data.value("engineIndestructible").isBool();
}
inline bool dimensions(const QJsonObject &data) {
    for (const auto key : {"width", "length", "height"}) {
        const auto value = data.value(key);
        if (!value.isDouble() || !std::isfinite(value.toDouble()) || value.toDouble() < .5 || value.toDouble() > 2) return false;
    }
    return true;
}
inline bool gears(const QJsonValue &value) {
    if (!value.isArray() || value.toArray().isEmpty()) return false;
    if (QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact).size() > 24000) return false;
    for (const auto ratio : value.toArray())
        if (!ratio.isDouble() || !std::isfinite(ratio.toDouble()) || ratio.toDouble() < .01 || ratio.toDouble() > 100) return false;
    return true;
}
inline bool gravity(const QJsonValue &value) {
    const auto v = value.toDouble();
    return value.isDouble() && std::isfinite(v) && v >= -30.0 && v <= 0.0;
}
inline bool model(const QString &value) {
    return QRegularExpression("^[A-Za-z0-9_-]{1,80}$").match(value).hasMatch();
}
inline bool config(const QString &value) {
    if (value.isEmpty() || value.size() > 160 || value == "." || value == "..") return false;
    for (const auto c : value)
        if (c.unicode() < 32 || c == '/' || c == '\\' || c.unicode() == 127) return false;
    return true;
}
inline bool identity(const QString &value) {
    const auto slash = value.indexOf('/');
    return slash < 0 ? model(value) : model(value.left(slash)) && config(value.mid(slash + 1));
}
inline QJsonObject preview(const QJsonValue &value, const QString &modelKey) {
    const auto input = value.toObject();
    const auto path = input.value("path").toString();
    if (!path.startsWith("/vehicles/" + modelKey + "/") || path.size() > 512
        || path.contains("..") || path.contains('\\')
        || (!path.endsWith(".jpg", Qt::CaseInsensitive) && !path.endsWith(".png", Qt::CaseInsensitive))) return {};
    QJsonObject out{{"path", path}};
    for (const auto key : {"archive", "real"}) {
        const auto source = input.value(key).toString();
        if (!source.contains(QChar(0)) && source.size() <= 2048) out.insert(key, source);
    }
    return out;
}
}
