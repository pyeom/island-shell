#!/usr/bin/env bash
# Prints exports for the nested island-shell: the newest island-shell whose
# WAYLAND_DISPLAY is not the host session's ($ISH_HOST_WAYLAND, default
# wayland-1). Fails, printing nothing, when there is none, so callers never
# end up talking to the real session.
host="${ISH_HOST_WAYLAND:-wayland-1}"
for pid in $(pgrep -x island-shell | sort -rn); do
    env=$(tr '\0' '\n' </proc/"$pid"/environ 2>/dev/null) || continue
    display=$(sed -n 's/^WAYLAND_DISPLAY=//p' <<<"$env")
    [[ -n "$display" && "$display" != "$host" ]] || continue
    grep -E '^(DBUS_SESSION_BUS_ADDRESS|HYPRLAND_INSTANCE_SIGNATURE|WAYLAND_DISPLAY)=' <<<"$env" |
        sed "s/^/export /"
    exit 0
done
echo "env.sh: no nested island-shell running" >&2
exit 1
