#!/usr/bin/env bash
# Exits every nested Hyprland. The real session (on $ISH_HOST_WAYLAND,
# default wayland-1) is never touched.
host="${ISH_HOST_WAYLAND:-wayland-1}"
for sig in $(hyprctl -j instances | python3 -I -c "
import json, sys
for i in json.load(sys.stdin):
    if i['wl_socket'] != '$host':
        print(i['instance'])"); do
    HYPRLAND_INSTANCE_SIGNATURE=$sig hyprctl dispatch exit >/dev/null
done
