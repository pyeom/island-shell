#!/usr/bin/env bash
# Starts a private bus with fake NetworkManager/BlueZ/UPower and restarts the
# nested island-shell using it as its system bus. Prints the fake services'
# log to /tmp/ish-fake-system.log. Stop with: pkill -f fake-system.py
set -euo pipefail
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$here/../.."
source "$here/lib.sh" || exit 1

address=$(dbus-daemon --config-file="$root/scripts/nested/session.conf" --fork --print-address)
"$here/fake-system-stop.sh"
DBUS_SESSION_BUS_ADDRESS="$address" python3 -I "$here/fake-system.py" >/tmp/ish-fake-system.log 2>&1 &
echo $! >/tmp/ish-fake-system.pid
for _ in $(seq 1 20); do grep -q ready /tmp/ish-fake-system.log 2>/dev/null && break; sleep 0.2; done

t=/tmp/ish-test
"$here/../nested/kill-island.sh"
hyprctl dispatch exec "sleep 0.5; DBUS_SYSTEM_BUS_ADDRESS=$address ISLAND_SHELL_BACKLIGHT=$t/backlight XDG_STATE_HOME=$t/state XDG_CACHE_HOME=$t/cache ISLAND_SHELL_DRY_RUN=1 ISLAND_SHELL_CONFIG=$t/config.toml ISLAND_SHELL_ACTIVITIES=$t/activities $root/build/dev/src/island-shell" >/dev/null
sleep 1.5
"$here/wait-island.sh"
echo "fake system bus: $address"
