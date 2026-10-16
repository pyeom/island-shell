# Switching to island-shell

island-shell replaces a bar (waybar), a notification daemon (swaync, mako,
dunst), a launcher (rofi, wofi) and a power menu. This page lists what to
change in a Hyprland config. Nothing is changed for you.

Try it first in the nested session (`scripts/dev-nested.sh`) or alongside
your current setup: if another daemon owns notifications, island-shell shows
an error in the island and leaves it alone.

## 1. Autostart

Start `island-shell-run` (restarts the shell if it crashes) and stop starting
the bar.

Lua config (`hl.on("hyprland.start", ...)`):

```lua
hl.on("hyprland.start", function()
    -- hl.exec_cmd("waybar")
    -- hl.exec_cmd("hyprpanel")
    hl.exec_cmd("island-shell-run")
    -- ...
end)
```

`hyprland.conf`:

```
# exec-once = waybar
exec-once = island-shell-run
```

With uwsm or another setup that runs `graphical-session.target`, use the
user unit instead:

```sh
systemctl --user enable --now island-shell.service
```

## 2. Notifications

Only one program can own `org.freedesktop.Notifications`. island-shell takes
it at startup, so:

- Remove any `exec` of swaync, mako, dunst or hyprpanel from the autostart.
- D-Bus-activated daemons (swaync ships
  `/usr/share/dbus-1/services/org.erikreider.swaync.service`) are only started
  when nobody owns the name, so they stay idle once island-shell runs. To
  rule it out entirely, uninstall them or mask the file:

  ```sh
  mkdir -p ~/.local/share/dbus-1/services
  ln -s /dev/null ~/.local/share/dbus-1/services/org.erikreider.swaync.service
  ```

- If something already owns the name when island-shell starts (check with
  `busctl --user status org.freedesktop.Notifications`), the island shows
  "notifications: ... is owned by ...". Stop that program and run
  `pkill island-shell` (island-shell-run restarts it).

## 3. Keybinds

Lua:

```lua
hl.bind(mainMod .. " + R", hl.dsp.exec_cmd("islandctl launcher toggle"))
hl.bind("ALT + TAB",       hl.dsp.exec_cmd("islandctl power"))
hl.bind("ALT + p",         hl.dsp.exec_cmd("islandctl expand media"))
-- Screenshots (saved to ~/Pictures/Screenshots and copied):
hl.bind("Print",        hl.dsp.exec_cmd("islandctl screenshot region"))
hl.bind("CTRL + Print", hl.dsp.exec_cmd("islandctl screenshot window"))
hl.bind("ALT + Print",  hl.dsp.exec_cmd("islandctl screenshot output"))
-- Wallpaper picker (or "islandctl wallpaper random" for a random one):
hl.bind("ALT + w", hl.dsp.exec_cmd("islandctl wallpaper"))
-- Optional:
hl.bind(mainMod .. " + N", hl.dsp.exec_cmd("islandctl dnd toggle"))
hl.bind(mainMod .. " + S", hl.dsp.exec_cmd("islandctl quick"))
```

`hyprland.conf`:

```
bind = SUPER, R, exec, islandctl launcher toggle
bind = ALT, TAB, exec, islandctl power
bind = ALT, P, exec, islandctl expand media
bind = , Print, exec, islandctl screenshot region
bind = CTRL, Print, exec, islandctl screenshot window
bind = ALT, Print, exec, islandctl screenshot output
bind = ALT, W, exec, islandctl wallpaper
bind = SUPER, N, exec, islandctl dnd toggle
bind = SUPER, S, exec, islandctl quick
```

Volume keys (`pactl`, `wpctl`) need no change: the OSD follows PipeWire.

Binds that refreshed the old bar or notification daemon (for example
`~/.config/waybar/refresh.sh`, `~/.config/swaync/refresh.sh`) can go. To
reload island-shell's config, theme and activities: `islandctl reload`.

## 4. Blur (optional)

With `[theme] style = "blur"`:

```lua
hl.layer_rule({ name = "island-blur", match = { namespace = "island-shell" }, blur = true, ignore_alpha = 0.01 })
```

```
layerrule = blur on, match:namespace island-shell
layerrule = ignore_alpha 0.01, match:namespace island-shell
```

## 5. Config

```sh
mkdir -p ~/.config/island-shell
cp /usr/share/island-shell/config.toml ~/.config/island-shell/
```

Every key is optional. With pywal, set `[theme] source = "pywal"`.

## Going back

Undo the autostart and bind changes, then:

```sh
pkill island-shell-run; pkill island-shell
waybar &
```

Notification history is in `~/.local/state/island-shell/`; the config in
`~/.config/island-shell/`. Both can be deleted.
