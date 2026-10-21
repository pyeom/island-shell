#pragma once

#include <QByteArrayView>
#include <QString>

#include <optional>

namespace island::shots {

// Window rectangles for `slurp -r` ("x,y wxh" per line), from `j/clients`
// and `j/monitors`: mapped windows on a workspace shown on some monitor
// (active or special). Floating windows first, then most recently focused,
// so the box under the pointer is the window on top.
QString windowBoxes(QByteArrayView clients, QByteArrayView monitors);

// The focused window's rectangle from `j/activewindow`; nothing when no
// window has focus.
std::optional<QString> activeWindowBox(QByteArrayView activeWindow);

} // namespace island::shots
