#!/usr/bin/env bash
# Stops the island-shell running under this Hyprland instance only.
# `pkill -x island-shell` would also stop the one in the real session.
sig="${HYPRLAND_INSTANCE_SIGNATURE:?not inside a Hyprland session}"
for pid in $(pgrep -x island-shell); do
    if grep -qz "^HYPRLAND_INSTANCE_SIGNATURE=$sig$" "/proc/$pid/environ" 2>/dev/null; then
        kill "$pid"
    fi
done
