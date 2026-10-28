#pragma once

#include <QByteArray>
#include <QImage>
#include <QList>
#include <QString>

#include "desktopentry.h"

namespace island {

// Where a StatusNotifierItem lives. Apps register with a bus name
// ("org.kde.StatusNotifierItem-1234-1", object /StatusNotifierItem), an
// object path on their own connection ("/org/ayatana/NotificationItem/x")
// or, through another watcher, "<bus name>/<object path>".
struct TrayAddress {
    QString service;
    QString path;

    QString key() const { return service + path; }
};
TrayAddress parseTrayAddress(const QString& registered, const QString& sender = {});

// One entry of IconPixmap: ARGB32 in network byte order.
struct TrayPixmap {
    int width = 0;
    int height = 0;
    QByteArray data;
};
// The smallest pixmap at least `size` wide (else the largest), as an image;
// null when there is none or its data is short.
QImage trayPixmapImage(const QList<TrayPixmap>& pixmaps, int size);

// The installed app a tray item belongs to, by its Id or Title ("discord",
// "Discord"): desktop file name, StartupWMClass, then app name, ignoring
// case. Empty when nothing matches.
QString trayDesktopId(const QString& itemId, const QString& title, const QList<DesktopEntry>& entries);

// dbusmenu labels mark mnemonics with "_" ("_Quit", "Save __as"): the text
// without them.
QString trayMenuLabel(const QString& label);

} // namespace island
