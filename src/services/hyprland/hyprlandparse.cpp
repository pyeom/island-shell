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

QVariantList parseWorkspaces(QByteArrayView json, std::optional<QByteArrayView> clients) {
    // Client "fullscreen" is a bit mask: 1 maximized, 2 fullscreen.
    QSet<int> fullscreen;
    if (clients) {
        for (const auto& value : parseArray(*clients)) {
            const auto c = value.toObject();
            if (c[u"fullscreen"].toInt() & 2)
                fullscreen.insert(c[u"workspace"].toObject()[u"id"].toInt());
        }
    }
    QVariantList out;
    for (const auto& value : parseArray(json)) {
        const auto w = value.toObject();
        const int id = w[u"id"].toInt();
        if (id <= 0)
            continue;
        const bool hasFullscreen = w[u"hasfullscreen"].toBool() && (!clients || fullscreen.contains(id));
        out.append(QVariantMap{
            {u"id"_s, id},
            {u"name"_s, w[u"name"].toString()},
            {u"monitor"_s, w[u"monitor"].toString()},
            {u"windows"_s, w[u"windows"].toInt()},
            {u"hasFullscreen"_s, hasFullscreen},
        });
    }
    sortById(out);
    return out;
}

QVariantList parseClients(QByteArrayView json) {
    QVariantList out;
    for (const auto& value : parseArray(json)) {
        const auto c = value.toObject();
        if (!c[u"mapped"].toBool(true) || c[u"hidden"].toBool(false))
            continue;
        const auto cls = c[u"class"].toString();
        if (cls.isEmpty() && c[u"title"].toString().isEmpty())
            continue;
        out.append(QVariantMap{
            {u"address"_s, c[u"address"].toString()},
            {u"class"_s, cls},
            {u"title"_s, c[u"title"].toString()},
            {u"workspace"_s, c[u"workspace"].toObject()[u"name"].toString()},
            {u"monitor"_s, c[u"monitor"].toInt()},
        });
    }
    return out;
}

QString openWindowClass(QStringView data) {
    const auto parts = data.split(u',');
    return parts.size() >= 3 ? parts[2].trimmed().toString() : QString();
}

QString luaString(QStringView text) {
    QString level;
    while (text.contains(u']' + level + u']'))
        level += u'=';
    // A newline right after the opening bracket would be dropped.
    const auto lead = text.startsWith(u'\n') ? u"\n"_s : QString();
    return u'[' + level + u'[' + lead + text.toString() + u']' + level + u']';
}

Dispatch execDispatch(const QString& command) {
    return {u"exec "_s + command, u"hl.dsp.exec_cmd("_s + luaString(command) + u')'};
}

Dispatch workspaceDispatch(int id) {
    return {u"workspace "_s + QString::number(id), u"hl.dsp.focus({ workspace = %1 })"_s.arg(id)};
}

Dispatch focusWindowDispatch(const QString& address) {
    return {u"focuswindow address:"_s + address,
            u"hl.dsp.focus({ window = "_s + luaString(u"address:"_s + address) + u" })"_s};
}

Dispatch exitDispatch() {
    return {u"exit"_s, u"hl.dsp.exit()"_s};
}

bool isLuaSyntaxError(QByteArrayView reply) {
    return reply.contains("hl.dispatch(") || reply.contains("dispatch in lua");
}

} // namespace island::hypr
