# Changelog

The QML API (`import IslandShell`) is unstable until 1.0; breaking changes are
listed here.

## Unreleased

### Added
- Notification server (`org.freedesktop.Notifications`): popups in the island,
  actions, inline reply, images, per-app rules, do-not-disturb, history saved
  across restarts and shown in the expanded idle view.
- Volume OSD from PipeWire and brightness OSD from the backlight.
- Media activity (MPRIS) with art, progress, seeking and player switching.
- Live activities over D-Bus (`org.islandshell.LiveActivities`) and
  `islandctl push/update/end/live`.
- Launcher with fuzzy search, usage ranking and desktop actions.
- Power menu through logind; shut down and restart need press-and-hold.
- `Activity.modal` and `Activity.dismiss()`; `IconImage`, `IconButton`,
  `ProgressRing`, `Icons`, `Notifications`, `Audio`, `Brightness`, `Mpris`,
  `LiveActivities`, `Apps`, `Power` in the public module.
- Activity framework: `Activity` with compact, minimal and expanded
  presentations; per-monitor arbiter with primary and secondary bubble.
- User activities loaded from `~/.config/island-shell/activities/` with hot
  reload and per-file error isolation.
- `config.toml` with hot reload; theme with optional pywal colors and blur style.
- D-Bus interface `org.islandshell.Control` and the `islandctl` client.
- Hyprland IPC service; built-in idle activity with workspaces and clock.
- Fixed-size layer surface per monitor with a fixed exclusive zone; the island
  animates inside it with springs and only the input region follows it.
