#!/usr/bin/env bash
# Runs island-shell inside a nested Hyprland window with its own D-Bus session,
# so it never competes with the real session's bar or notification daemon.
#
#   scripts/dev-nested.sh            # build (dev preset) and launch
#   scripts/dev-nested.sh --demo     # extra args are passed to island-shell
#   NESTED_SIZE=1600x900 scripts/dev-nested.sh
#
# The power menu never runs real actions here: ISLAND_SHELL_DRY_RUN defaults
# to 1, and island-shell also refuses them under any nested Hyprland.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export ISLAND_SHELL_DRY_RUN="${ISLAND_SHELL_DRY_RUN:-1}"
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

# Hyprland does not hand its whole environment to exec-once, so pass the
# variables island-shell reads explicitly.
island_cmd="env"
while IFS='=' read -r name value; do
    island_cmd+=" $name=$(printf '%q' "$value")"
done < <(env | grep -E '^(ISLAND_SHELL_[A-Z_]*|XDG_STATE_HOME|XDG_CACHE_HOME|QT_LOGGING_RULES)=' || true)
island_cmd+=" $build/src/island-shell"
for arg in "$@"; do island_cmd+=" $(printf '%q' "$arg")"; done

sed -e "s|@ISLAND_CMD@|$island_cmd|" \
    -e "s|@TERMINAL@|${terminal:-true}|" \
    -e "s|@SIZE@|$size|" \
    "$root/scripts/nested/hyprland.conf.in" >"$conf"

echo "dev-nested: config $conf"
exec dbus-run-session --config-file="$root/scripts/nested/session.conf" -- Hyprland --config "$conf"
