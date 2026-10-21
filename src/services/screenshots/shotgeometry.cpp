#include "shotgeometry.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

#include <algorithm>

namespace island::shots {

namespace {

QJsonArray array(QByteArrayView json) {
    return QJsonDocument::fromJson(json.toByteArray()).array();
}

std::optional<QString> box(const QJsonObject& client) {
    const auto at = client.value(u"at").toArray();
    const auto size = client.value(u"size").toArray();
    if (at.size() != 2 || size.size() != 2)
        return std::nullopt;
    const int w = size[0].toInt(), h = size[1].toInt();
    if (w <= 0 || h <= 0)
        return std::nullopt;
    return QStringLiteral("%1,%2 %3x%4").arg(at[0].toInt()).arg(at[1].toInt()).arg(w).arg(h);
}

} // namespace

QString windowBoxes(QByteArrayView clients, QByteArrayView monitors) {
    QSet<int> shown;
    for (const auto& m : array(monitors)) {
        const auto mon = m.toObject();
        shown.insert(mon.value(u"activeWorkspace").toObject().value(u"id").toInt());
        const int special = mon.value(u"specialWorkspace").toObject().value(u"id").toInt();
        if (special != 0)
            shown.insert(special);
    }

    QList<QJsonObject> windows;
    for (const auto& c : array(clients)) {
        const auto client = c.toObject();
        if (!client.value(u"mapped").toBool() || client.value(u"hidden").toBool())
            continue;
        if (!shown.contains(client.value(u"workspace").toObject().value(u"id").toInt()))
            continue;
        windows << client;
    }
    std::stable_sort(windows.begin(), windows.end(), [](const QJsonObject& a, const QJsonObject& b) {
        const bool fa = a.value(u"floating").toBool(), fb = b.value(u"floating").toBool();
        if (fa != fb)
            return fa;
        return a.value(u"focusHistoryID").toInt() < b.value(u"focusHistoryID").toInt();
    });

    QString out;
    for (const auto& w : windows)
        if (const auto b = box(w))
            out += *b + u'\n';
    return out;
}

std::optional<QString> activeWindowBox(QByteArrayView activeWindow) {
    const auto obj = QJsonDocument::fromJson(activeWindow.toByteArray()).object();
    if (obj.value(u"address").toString().isEmpty())
        return std::nullopt;
    return box(obj);
}

} // namespace island::shots
