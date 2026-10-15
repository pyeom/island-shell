#include "hyprlandparse.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace island::hypr {

std::optional<Event> parseEventLine(QByteArrayView line) {
    const auto sep = line.indexOf(QByteArrayView(">>"));
    if (sep <= 0)
        return std::nullopt;
    return Event{QString::fromUtf8(line.first(sep)), QString::fromUtf8(line.sliced(sep + 2))};
}

bool affectsWorkspaces(const QString& eventName) {
    static const QSet<QString> names = {
        u"workspace"_s,        u"workspacev2"_s,        u"focusedmon"_s,
        u"focusedmonv2"_s,     u"createworkspace"_s,    u"createworkspacev2"_s,
        u"destroyworkspace"_s, u"destroyworkspacev2"_s, u"moveworkspace"_s,
        u"moveworkspacev2"_s,  u"renameworkspace"_s,    u"monitoradded"_s,
        u"monitoraddedv2"_s,   u"monitorremoved"_s,     u"monitorremovedv2"_s,
        u"openwindow"_s,       u"closewindow"_s,        u"movewindow"_s,
        u"movewindowv2"_s,     u"fullscreen"_s,
    };
    return names.contains(eventName);
}

namespace {
QJsonArray parseArray(QByteArrayView json) {
    return QJsonDocument::fromJson(json.toByteArray()).array();
}

void sortById(QVariantList& list) {
    std::ranges::sort(list, {}, [](const QVariant& v) { return v.toMap().value(u"id"_s).toInt(); });
}
} // namespace

QVariantList parseMonitors(QByteArrayView json, QString* focused) {
    QVariantList out;
    for (const auto& value : parseArray(json)) {
        const auto m = value.toObject();
        const auto name = m[u"name"].toString();
        const bool isFocused = m[u"focused"].toBool();
        if (isFocused && focused)
            *focused = name;
        out.append(QVariantMap{
            {u"name"_s, name},
            {u"id"_s, m[u"id"].toInt()},
            {u"focused"_s, isFocused},
            {u"activeWorkspace"_s, m[u"activeWorkspace"].toObject()[u"id"].toInt()},
        });
    }
    sortById(out);
    return out;
}

QVariantList parseWorkspaces(QByteArrayView json) {
    QVariantList out;
    for (const auto& value : parseArray(json)) {
        const auto w = value.toObject();
        const int id = w[u"id"].toInt();
        if (id <= 0)
            continue;
        out.append(QVariantMap{
            {u"id"_s, id},
            {u"name"_s, w[u"name"].toString()},
            {u"monitor"_s, w[u"monitor"].toString()},
            {u"windows"_s, w[u"windows"].toInt()},
            {u"hasFullscreen"_s, w[u"hasfullscreen"].toBool()},
        });
    }
    sortById(out);
    return out;
}

} // namespace island::hypr
