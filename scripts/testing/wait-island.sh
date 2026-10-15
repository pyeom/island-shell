#!/usr/bin/env bash
# Waits (up to 30 s) until the nested island answers on D-Bus.
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
for _ in $(seq 1 60); do
    sleep 0.5
    if exports=$("$here/env.sh" 2>/dev/null) && eval "$exports" &&
        [[ -n "${DBUS_SESSION_BUS_ADDRESS:-}" ]] &&
        "$here/../../build/dev/tools/islandctl/islandctl" version >/dev/null 2>&1; then
        exit 0
    fi
done
echo "island-shell did not come up" >&2
exit 1
