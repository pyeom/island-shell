#!/usr/bin/env bash
# Runs island-shell inside a nested Hyprland window with its own D-Bus session,
# so it never competes with the real session's bar or notification daemon.
#
#   scripts/dev-nested.sh            # build (dev preset) and launch
#   scripts/dev-nested.sh --demo     # extra args are passed to island-shell
#   NESTED_SIZE=1600x900 scripts/dev-nested.sh
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$root/build/dev"
size="${NESTED_SIZE:-1920x1080}"

if [[ -z "${WAYLAND_DISPLAY:-}" ]]; then
    echo "dev-nested: needs a running Wayland session" >&2
    exit 1
fi

cmake --preset dev >/dev/null
cmake --build --preset dev

terminal="$(command -v kitty || command -v alacritty || command -v foot || true)"
runtime="${XDG_RUNTIME_DIR:-/tmp}/island-shell-nested"
mkdir -p "$runtime"
conf="$runtime/hyprland.conf"

island_cmd="$build/src/island-shell"
for arg in "$@"; do island_cmd+=" $(printf '%q' "$arg")"; done

sed -e "s|@ISLAND_CMD@|$island_cmd|" \
    -e "s|@TERMINAL@|${terminal:-true}|" \
    -e "s|@SIZE@|$size|" \
    "$root/scripts/nested/hyprland.conf.in" >"$conf"

echo "dev-nested: config $conf"
exec dbus-run-session --config-file="$root/scripts/nested/session.conf" -- Hyprland --config "$conf"
