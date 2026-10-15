#!/usr/bin/env bash
# Prints exports for the newest island-shell process (the nested one).
pid=$(pgrep -n -x island-shell)
[ -z "$pid" ] && { echo "echo 'no island-shell running' >&2; return 1 2>/dev/null || exit 1"; exit; }
tr '\0' '\n' </proc/"$pid"/environ |
    grep -E '^(DBUS_SESSION_BUS_ADDRESS|HYPRLAND_INSTANCE_SIGNATURE|WAYLAND_DISPLAY)=' |
    sed "s/^/export /"
