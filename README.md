<img src="assets/icon.svg" width="64" height="64" alt="">

# island-shell

A Dynamic Island style shell for Hyprland, written in C++23 on Qt6 Quick.

The island lives in a fixed-size overlay layer surface with a fixed exclusive
zone. It expands and collapses inside that surface, and only the Wayland input
region follows the pill, so windows are never re-laid out and clicks outside
the pill reach the windows below.

Version 0.1. See [docs/PLAN.md](docs/PLAN.md) for the design and roadmap.

## Installing (Arch Linux)

```sh
git clone https://github.com/pyeom/island-shell.git
cd island-shell/packaging/arch
makepkg -si
```

Then follow [docs/migration.md](docs/migration.md) to start it from Hyprland
and move notifications, launcher and keybinds over.

## Building

Dependencies (Arch Linux):

```sh
sudo pacman -S --needed cmake ninja qt6-base qt6-declarative qt6-wayland \
    qt6-svg qt6-shadertools qt6-tools layer-shell-qt tomlplusplus libpipewire libical
```

Build and test:

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

## Development

`scripts/dev-nested.sh` builds the project and runs it inside a nested Hyprland
window with its own D-Bus session, so it does not conflict with your current bar
or notification daemon. Extra arguments go to `island-shell`:

```sh
scripts/dev-nested.sh          # hover to peek, click to expand
scripts/dev-nested.sh --demo   # cycle through states automatically
```

Inside the nested session: `SUPER+Return` opens a terminal, `SUPER+R` restarts
the shell, `SUPER+SHIFT+E` exits.

To test with a throwaway config instead of `~/.config/island-shell`:

```sh
ISLAND_SHELL_CONFIG=/tmp/test/config.toml \
ISLAND_SHELL_ACTIVITIES=/tmp/test/activities \
scripts/dev-nested.sh
```

`scripts/test-pkg.sh` builds and installs the package in a clean Arch
container (docker or podman) from the committed HEAD.

Translations live in `i18n/`. After changing user-visible strings:
`cmake --build --preset dev --target update_translations`, then translate the
new entries in `i18n/island-shell_es.ts` (Qt Linguist or a text editor).

## Configuration and activities

- Config: copy [`data/config.toml`](data/config.toml) to
  `~/.config/island-shell/config.toml`. Changes apply on save.
- Activities: see [docs/activities.md](docs/activities.md) and
  [`examples/activities/`](examples/activities/).
- Control from scripts or Hyprland binds with `islandctl` (`islandctl --help`).

## Keybinds

Example for a Lua Hyprland config (`hl.bind`):

```lua
hl.bind(mainMod .. " + R", hl.dsp.exec_cmd("islandctl launcher toggle"))
hl.bind("ALT + TAB",       hl.dsp.exec_cmd("islandctl power"))
hl.bind("ALT + p",         hl.dsp.exec_cmd("islandctl expand media"))
```

or `hyprland.conf`:

```
bind = SUPER, R, exec, islandctl launcher toggle
bind = ALT, TAB, exec, islandctl power
bind = ALT, P, exec, islandctl expand media
```

Volume keys need no change: the OSD follows PipeWire, whatever changed the
volume.

## Notifications

island-shell is a notification server. Only one program can own
`org.freedesktop.Notifications`, so stop swaync, mako, dunst or hyprpanel
first; if another one holds the name, the island shows an error and leaves it
alone.

## Calendar

The island reads local calendars (vdir collections or folders of `.ics`
files); it never talks to calendar servers itself. To show Google, iCloud,
Nextcloud or any CalDAV calendar, sync it with
[vdirsyncer](https://vdirsyncer.pimutils.org/):

```ini
# ~/.config/vdirsyncer/config
[general]
status_path = "~/.local/share/vdirsyncer/status/"

[pair personal]
a = "personal_remote"
b = "personal_local"
collections = ["from a"]

[storage personal_remote]
type = "caldav"
url = "https://caldav.example.com/"
username = "me"
password.fetch = ["command", "secret-tool", "lookup", "caldav", "me"]

[storage personal_local]
type = "filesystem"
path = "~/.calendars/"
fileext = ".ics"
```

New events (launcher `+`, or the `+` on the expanded calendar) are written
to the local folder; vdirsyncer uploads them on its next sync.

Then `vdirsyncer discover && vdirsyncer sync` (and a timer for periodic
syncs). `~/.calendars` is in the default `[calendar] paths`. For Google
Calendar use vdirsyncer's `google_calendar` storage.

## Launcher

`islandctl launcher toggle` (bind it to SUPER+R). Type to search apps and
open windows; a query that looks like math (`2*(3+4)`, `sqrt(2)`) shows the
result. Prefixes:

| Prefix | Does |
|---|---|
| `=` | calculator; Enter copies the result |
| `>` | run a shell command; Shift+Enter runs it in a terminal |
| `?` | search the web (`[launcher] search_url`) |
| `:` | emoji; Enter copies it |
| `+` | new calendar event: `+ mañana 15:00 dentista 1h`, `+ vie 9 reunión`, `+ 20/10 feriado` |

## Quick settings

Right click on the island, or `islandctl quick`: network, Bluetooth,
do-not-disturb, output volume and device, microphone level, mute and
device, brightness (laptops) and power profile.

## Fullscreen

Over a fullscreen window (games, videos) the island hides the clock, media
and privacy indicators and stops taking input; notifications and the OSD
still show. `[island] fullscreen = "hide"` keeps only critical notifications;
`"show"` disables this.

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).
