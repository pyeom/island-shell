#pragma once

#include <QByteArrayView>
#include <QString>
#include <QVariantList>

#include <optional>

namespace island::hypr {

// A line from .socket2.sock: "name>>data".
struct Event {
    QString name;
    QString data;
};

std::optional<Event> parseEventLine(QByteArrayView line);

// Events after which monitors/workspaces must be re-read.
bool affectsWorkspaces(const QString& eventName);

// `j/monitors` → [{ name, id, focused, activeWorkspace }], sorted by id.
// Sets `focused` to the focused monitor's name.
QVariantList parseMonitors(QByteArrayView json, QString* focused);

// `j/workspaces` → [{ id, name, monitor, windows }], regular workspaces only
// (special workspaces have negative ids), sorted by id.
QVariantList parseWorkspaces(QByteArrayView json);

} // namespace island::hypr
