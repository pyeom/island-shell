#!/usr/bin/env bash
# Restarts island-shell in the running nested session (same test env).
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$here/lib.sh" || exit 1
t=/tmp/ish-test
"$here/../nested/kill-island.sh"
hyprctl dispatch exec "sleep 0.5; ISLAND_SHELL_BACKLIGHT=$t/backlight XDG_STATE_HOME=$t/state XDG_CACHE_HOME=$t/cache ISLAND_SHELL_DRY_RUN=1 ISLAND_SHELL_CONFIG=$t/config.toml ISLAND_SHELL_ACTIVITIES=$t/activities $here/../../build/dev/src/island-shell $*" >/dev/null
sleep 1.5
"$here/wait-island.sh"
