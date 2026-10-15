# shellcheck shell=bash
# Source from bash or zsh: talk to the nested island-shell and take screenshots.
#   source scripts/testing/lib.sh && $CTL activities
if [ -n "${BASH_SOURCE[0]:-}" ]; then
    _ish_self="${BASH_SOURCE[0]}"
else
    _ish_self="${(%):-%x}" # zsh: path of the sourced file
fi
_ish_dir="$(cd "$(dirname "$_ish_self")" && pwd)"
eval "$("$_ish_dir/env.sh")"
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
