#pragma once

#include <QByteArrayView>
#include <QString>

namespace island {

// True when `environ` (a /proc/<pid>/environ blob, NUL-separated) belongs to
// a compositor that is itself a Wayland client, i.e. a nested Hyprland. A
// compositor driving real outputs has no WAYLAND_DISPLAY of its own.
bool environIsNested(QByteArrayView environ);

// Looks up the Hyprland instance this process runs under
// ($HYPRLAND_INSTANCE_SIGNATURE → hyprland.lock → pid) and checks it.
bool runningUnderNestedHyprland();

} // namespace island
