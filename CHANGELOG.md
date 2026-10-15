# Changelog

The QML API (`import IslandShell`) is unstable until 1.0; breaking changes are
listed here.

## Unreleased

### Added
- Activity framework: `Activity` with compact, minimal and expanded
  presentations; per-monitor arbiter with primary and secondary bubble.
- User activities loaded from `~/.config/island-shell/activities/` with hot
  reload and per-file error isolation.
- `config.toml` with hot reload; theme with optional pywal colors and blur style.
- D-Bus interface `org.islandshell.Control` and the `islandctl` client.
- Hyprland IPC service; built-in idle activity with workspaces and clock.
- Fixed-size layer surface per monitor with a fixed exclusive zone; the island
  animates inside it with springs and only the input region follows it.
