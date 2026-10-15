# shellcheck shell=bash
# Source from bash: talk to the nested island-shell and take screenshots.
_ish_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
eval "$("$_ish_dir/env.sh")"
CTL="$_ish_dir/../../build/dev/tools/islandctl/islandctl"
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
