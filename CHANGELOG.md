# Changelog

The QML API (`import IslandShell`) is unstable until 1.0; breaking changes are
listed here.

## Unreleased

### Added
- Hide over fullscreen windows (`[island] fullscreen`), with an empty input
  region so the island never blocks the fullscreen window;
  `Activity.ambient`.
- Audio output and input device picker; microphone level and mute in quick
  settings; microphone mute OSD.
- Launcher modes: open windows, calculator (`=`, or automatic for math),
  commands (`>`), web search (`?`, `[launcher] search_url`), emoji (`:`).
- `LauncherSearch` in the public module.
- System gauges (CPU, GPU, VRAM, RAM, SSD) in the expanded view and a
  sustained-heat alert (`[system]`).
- Weather for `[weather] city` (Open-Meteo) with hourly forecast and a rain
  alert.
- Night light toggle (wlsunset) in quick settings.
- `islandctl screenshot` with a result activity (open, copy, edit, delete).
- Wallpaper picker (`islandctl wallpaper`), random wallpaper.
- `[wallpaper] backend` (awww, swww, hyprpaper, swaybg, waypaper or a
  command), `backend_args` and `pywal`, also in the settings window, which
  now has on/off switches. `Wallpapers.backend` and `Wallpapers.refresh()`.
- Calendar events from the launcher (`+ mañana 15:00 dentista`) and a form.
- Settings window (`islandctl settings`) that edits config.toml in place,
  keeping comments.
- Wi-Fi network list and connection, Bluetooth device list, discovery and
  pairing, laptop and peripheral batteries with low-battery alerts.
- `RoundedImage` in the public module: an Image clipped to rounded corners.

### Fixed
- Privacy indicator no longer flickers and re-alerts while a voice chat
  toggles capture.
- Fuzzy search ranks literal matches first (`fire` → 🔥, not 🇮🇪).
- Bluetooth devices added after startup were never seen (the
  InterfacesAdded slot did not connect).
- Album art, screenshot and wallpaper thumbnails and notification images
  had square corners inside their rounded frames.
- An app's desktop actions in the launcher could squeeze its name to a few
  letters; only the actions that fit are shown now.
- Quick settings tiles overlapped the icon and the title with fonts that
  have tall line heights (Noto Sans).
- The settings window cut off "Screenshots & wallpaper" in its sidebar, and
  showed "&&" in that page's title.
- Only one modal is open at a time. Opening the wallpaper picker over quick
  settings and closing it uncovered quick settings again, so the next
  `islandctl quick` closed it instead of opening it.
- A maximized window counted as fullscreen and hid the island; only real
  fullscreen windows do now.
- The wallpaper picker only marked the current wallpaper when pywal had set
  it. It now asks the backend, so it follows awww, swww, hyprpaper, swaybg
  and changes made outside the island.

### Changed
- Default `[island] surface_height` is 560 so the richer expanded view fits.

### Notes
- The empty input region uses a Qt QPA header: rebuild after Qt minor
  upgrades.
- `docs/api-review.md`: what changes before the API is frozen as 1.0.

## 0.1.0 — 2026-10-15

First release.

### Safety
- Power actions are always dry runs under a nested Hyprland (detected from
  the compositor's own environment), whatever `ISLAND_SHELL_DRY_RUN` says;
  `scripts/dev-nested.sh` also defaults it to 1. Before this, a nested test
  session could power off the host through logind.

### Release
- Spanish translation, picked from the system locale.
- `island-shell-run` restarts the shell after crashes; optional systemd user
  unit for sessions with `graphical-session.target`.
- Arch package (`packaging/arch/PKGBUILD`, `island-shell-git`) and
  `scripts/test-pkg.sh` to build and install it in a clean container.
- `docs/migration.md`: replacing a bar, notification daemon, launcher and
  power menu.

### Fixed
- Startup could stall for ~10 s (and freeze the compositor) when Hyprland IPC
  requests were in flight while a window was being created. Hyprland IPC now
  runs on its own thread.

### Added
- Quick settings (right click or `islandctl quick`): network, Bluetooth,
  do-not-disturb, volume and brightness sliders, power profiles.
- Privacy indicators for microphone, camera and screen sharing.
- Calendar: month view and upcoming events in the expanded idle view, and an
  "event starting soon" activity; reads vdir/.ics folders via libical.
- `Network`, `Bluetooth`, `PowerProfiles`, `Privacy`, `Calendar` and
  `IslandSlider` in the public module; `Audio.setVolume/setMuted`,
  `Brightness.setLevel`.
- The clock stays visible while media plays.
- Shortcuts row (search, quick settings, DND, power) in the expanded idle view.
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
