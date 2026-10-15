# shellcheck shell=bash
# Source from bash or zsh: talk to the nested island-shell and take screenshots.
#   source scripts/testing/lib.sh && $CTL activities
# Scripts: `source scripts/testing/lib.sh || exit 1`.
if [ -n "${BASH_SOURCE[0]:-}" ]; then
    _ish_self="${BASH_SOURCE[0]}"
else
    _ish_self="${(%):-%x}" # zsh: path of the sourced file
fi
_ish_dir="$(cd "$(dirname "$_ish_self")" && pwd)"
# Refuse to continue without a nested session: every command below would
# otherwise hit the real one (its bus, its Hyprland).
if ! _ish_env="$("$_ish_dir/env.sh")"; then
    echo "lib.sh: not connected; start scripts/testing/run-nested.sh first" >&2
    return 1 2>/dev/null || exit 1
fi
eval "$_ish_env"
CTL="$(cd "$_ish_dir/../.." && pwd)/build/dev/tools/islandctl/islandctl"
mkdir -p /tmp/ish-shots

# Island layer position on the nested output.
read -r LX LW < <(hyprctl layers -j 2>/dev/null | python3 -I -c "
import json, sys
for v in json.load(sys.stdin).values():
    for ls in v['levels'].values():
        for l in ls:
            if l['namespace'] == 'island-shell':
                print(l['x'], l['w']); raise SystemExit
" 2>/dev/null)

# shot NAME [HEIGHT]: screenshot of the island area.
shot() { grim -g "${LX:-0},0 ${LW:-600}x${2:-260}" "/tmp/ish-shots/$1.png"; }
