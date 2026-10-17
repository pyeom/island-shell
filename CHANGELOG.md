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
- `[launcher] search_engine` (DuckDuckGo, Google, Bing, Brave, Startpage,
  Ecosia, Kagi or a custom `search_url`), a selector in the settings window.
- Emoji from the launcher are typed into the focused window (with `wtype`)
  as well as copied.
- `Hyprland.exec()`, `focusWorkspace()` and `focusWindow()`, which work with
  both hyprland.conf and hyprland.lua.
- Live activities for browser downloads (`.part`, `.crdownload` in the
  downloads folder) and for the progress apps report over the Unity
  LauncherEntry signal (`[live]`); `island-dl URL` downloads with aria2c or
  curl showing its progress.
- An island expanded on a live activity collapses when the activity ends,
  instead of showing the idle view.
- `[priorities]`: change which activity takes the pill (e.g. live
  activities above the microphone indicator), and `[privacy]` microphone /
  camera / screen switches; both in the settings window.
- `[keybinds]`: island actions bound in Hyprland while the island runs,
  re-added after Hyprland reloads its config, and a Keybindings page in the
  settings window that records combinations. `Keybinds` in the public
  module.

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
- With a Lua Hyprland config (hyprland.lua) nothing launched: apps, web
  search, wallpapers, focusing windows and workspace dots used
  hyprland.conf dispatch syntax, which Lua configs reject. island-shell now
  falls back to `hl.dsp.*` when Hyprland asks for it.
- The launcher ranks apps by how often you open their windows, from any
  source, and with an empty query by the plain count (similar counts used
  to tie and fall back to alphabetical order).
- Only one modal is open at a time. Opening the wallpaper picker over quick
  settings and closing it uncovered quick settings again, so the next
  `islandctl quick` closed it instead of opening it.
- A maximized window counted as fullscreen and hid the island; only real
  fullscreen windows do now.
- The wallpaper picker only marked the current wallpaper when pywal had set
  it. It now asks the backend, so it follows awww, swww, hyprpaper, swaybg
  and changes made outside the island.

- Idle CPU use: 2–5% down to ~0. Something on the desktop opening and
  closing /dev/video0 every 3 s made the camera indicator scan every file
  descriptor of every process each time; probes (opened and closed again
  while nobody used the camera) no longer trigger a scan, and the scan uses
  plain readlinkat. The night light check reads the known wlsunset's
  /proc entry instead of all of /proc every 3 s.
- `SystemStats` samples every 2 s only while gauges are shown
  (`addViewer()`/`removeViewer()`), else temperatures every 10 s for the
  heat alert, and only notifies when a value changed.
- `Clock.minute` (and `Clock.formatHasSeconds`): the built-in clocks,
  calendar and event lists no longer re-evaluate every second.
- The media bars step 8 times a second instead of animating continuously,
  which kept the island drawing at the monitor's refresh rate while music
  played (1.7% → 0.3% CPU).

### Changed
- Default `[island] surface_height` is 560 so the richer expanded view fits.
- Tapping the island while an app uses the microphone, camera or screen
  opens the usual expanded view, with a line saying who uses what; the
  short list is only shown for the alert when a device turns on.
- `>command` in the launcher runs terminal programs (yay, htop, scripts) in
  a terminal on Enter; they used to run hidden and exit at once. Graphical
  apps still start on their own; Shift+Enter swaps the two.
- The expanded view always shows the month. Without calendars, the first
  new event creates a local one in the last `[calendar] paths` folder.

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
