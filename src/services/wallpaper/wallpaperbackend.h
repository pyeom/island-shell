#pragma once

#include <QString>
#include <QStringList>
#include <QStringView>

#include <optional>

namespace island::wallpaper {

// What sets the wallpaper ([wallpaper] backend).
enum class Backend {
    None,      // nothing installed
    Awww,      // awww img
    Swww,      // swww img
    Hyprpaper, // hyprctl hyprpaper
    Swaybg,    // restarts swaybg
    Waypaper,  // waypaper --wallpaper, which drives one of the above and
               // remembers the choice for `waypaper --restore`
    Command,   // [wallpaper] apply
};

// "awww", "swww", "hyprpaper", "swaybg", "waypaper", "command"; not "auto".
std::optional<Backend> backendFromName(QStringView name);
QString backendName(Backend backend);

// Executable the backend needs; empty for Command and None.
QString program(Backend backend);
// Process that shows the wallpaper while the backend is in use; empty when
// it has none of its own (waypaper, a command).
QString daemonProcess(Backend backend);

// Shell command that applies %f (replaced by the quoted file later). `args`
// go to the backend's own command; `custom` is [wallpaper] apply.
QString applyCommand(Backend backend, const QString& args, const QString& custom);

// Program and arguments that print what the backend shows, for parseQuery();
// empty when it cannot be asked (swaybg, waypaper, a command).
QStringList queryCommand(Backend backend);

// Images on screen, one per output in order:
//   awww/swww query:              ": DP-2: 1920x1080, scale: 1, currently displaying: image: /a.jpg"
//   hyprctl hyprpaper listactive: "DP-2 = /a.jpg"
// Outputs that show a plain color are skipped.
QStringList parseQuery(QStringView text);

// waypaper's config.ini: the wallpaper it last set and the backend it uses.
struct WaypaperState {
    QString wallpaper;
    std::optional<Backend> backend;
};
WaypaperState parseWaypaperIni(QStringView text);

// The image of a running swaybg, from its command line (-i/--image).
QString swaybgImage(const QStringList& argv);

// The entry of `files` that the first matching `shown` image refers to,
// comparing canonical paths (symlinks, "~/"); empty when none is in `files`.
QString match(const QStringList& files, const QStringList& shown);

} // namespace island::wallpaper
