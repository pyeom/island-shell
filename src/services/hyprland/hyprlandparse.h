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

// `j/workspaces` → [{ id, name, monitor, windows, hasFullscreen }], regular workspaces only
// (special workspaces have negative ids), sorted by id. Hyprland's
// "hasfullscreen" is also true for a maximized window; with `clients`
// (`j/clients`), hasFullscreen is true only for a real fullscreen one.
QVariantList parseWorkspaces(QByteArrayView json, std::optional<QByteArrayView> clients = std::nullopt);

// `j/clients` → [{ address, class, title, workspace, monitor }]: mapped,
// visible windows only.
QVariantList parseClients(QByteArrayView json);

} // namespace island::hypr
