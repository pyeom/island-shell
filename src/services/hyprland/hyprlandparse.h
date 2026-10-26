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

// The window class in an "openwindow" event: "ADDRESS,WORKSPACE,CLASS,TITLE".
QString openWindowClass(QStringView data);

// A dispatcher in both config syntaxes: hyprland.conf ("exec foot") and the
// Lua config ("hl.dsp.exec_cmd(...)"), which rejects the former.
struct Dispatch {
    QString legacy;
    QString lua;
};
Dispatch execDispatch(const QString& command);
Dispatch workspaceDispatch(int id);
Dispatch focusWindowDispatch(const QString& address);
Dispatch exitDispatch();

// `text` as a Lua long string literal ([[...]], [=[...]=], ...), so quotes,
// backslashes and "$(...)" pass through unchanged.
QString luaString(QStringView text);

// Setting a boolean option ("decoration:blur:enabled") at runtime:
//   eval hl.config({ decoration = { blur = { enabled = false } } })
//   keyword decoration:blur:enabled false
QByteArray luaSetOption(QStringView option, bool value);
QByteArray confSetOption(QStringView option, bool value);
// A boolean option's value from `j/getoption` ({"bool": true} or, in older
// versions, {"int": 1}); nullopt when the reply has neither.
std::optional<bool> parseBoolOption(const QByteArray& json);

// The reply to a hyprland.conf-style dispatch sent to a Lua config.
bool isLuaSyntaxError(QByteArrayView reply);

} // namespace island::hypr
