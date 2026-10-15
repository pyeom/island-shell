# Testing helpers

Tools for exercising island-shell inside the nested compositor without
touching the real session. Everything they write goes to `/tmp/ish-test`.

| Script | What it does |
|---|---|
| `fixtures.sh` | Creates `/tmp/ish-test`: config, calendars (one event a few minutes from now), a fake backlight, the example activity. |
| `run-nested.sh` | Starts `scripts/dev-nested.sh` with the fixtures, dry-run power and state/cache under `/tmp/ish-test`. |
| `lib.sh` | `source` it: exports the nested session's bus and Hyprland instance, `$CTL` (islandctl), `shot NAME [HEIGHT]` (screenshot of the island to `/tmp/ish-shots/`). |
| `relaunch.sh` | Restarts island-shell inside the running nested session. |
| `close-nested.sh` | Exits every nested Hyprland; never the real session. |
| `notify.py` | `notify-send` replacement with actions, inline reply, image data and signal printing. |
| `fakeplayer.py` | Fake MPRIS player. |

Run Python helpers with `python3 -I`.
