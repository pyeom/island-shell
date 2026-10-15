#!/usr/bin/env bash
# Starts the nested session with the test fixtures and waits for the island.
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
"$here/fixtures.sh" >/dev/null
unset DBUS_SESSION_BUS_ADDRESS HYPRLAND_INSTANCE_SIGNATURE
export WAYLAND_DISPLAY="${ISH_HOST_WAYLAND:-wayland-1}"
t=/tmp/ish-test
ISLAND_SHELL_BACKLIGHT=$t/backlight XDG_STATE_HOME=$t/state XDG_CACHE_HOME=$t/cache \
ISLAND_SHELL_DRY_RUN=1 ISLAND_SHELL_CONFIG=$t/config.toml ISLAND_SHELL_ACTIVITIES=$t/activities \
NESTED_SIZE="${NESTED_SIZE:-1280x720}" \
    setsid "$here/../dev-nested.sh" "$@" >/tmp/ish-nested.log 2>&1 &
"$here/wait-island.sh"
