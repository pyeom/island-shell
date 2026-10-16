#!/usr/bin/env bash
# Creates the test fixtures in /tmp/ish-test (idempotent: rewrites them).
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
t=/tmp/ish-test
mkdir -p "$t"/{activities,state,cache,backlight} "$t"/calendars/{personal,trabajo} /tmp/ish-shots

cp "$root/examples/activities/Pomodoro.qml" "$t/activities/"
cat >"$t/config.toml" <<CONF
[activities.pomodoro]
minutes = 1

[calendar]
paths = ["$t/calendars"]

[notifications]
rules = [ { app = "Spam", mute = true }, { app = "quiet*", no_popup = true } ]

[weather]
city = "Santiago"
CONF

echo 1000 >"$t/backlight/max_brightness"
echo 600 >"$t/backlight/actual_brightness"

echo Personal >"$t/calendars/personal/displayname"
echo '#30d158' >"$t/calendars/personal/color"
echo Trabajo >"$t/calendars/trabajo/displayname"
echo '#ff9f0a' >"$t/calendars/trabajo/color"

python3 -I - "$t/calendars" <<'PY'
import sys
from datetime import datetime, timedelta, timezone
cal = sys.argv[1]
now = datetime.now(timezone.utc)
fmt = lambda d: d.strftime("%Y%m%dT%H%M%SZ")
def write(path, body):
    open(path, "w").write("BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:-//ish//EN\r\n" + body + "END:VCALENDAR\r\n")
def event(uid, summary, start, minutes, extra=""):
    return (f"BEGIN:VEVENT\r\nUID:{uid}\r\nSUMMARY:{summary}\r\nDTSTART:{fmt(start)}\r\n"
            f"DTEND:{fmt(start + timedelta(minutes=minutes))}\r\n{extra}END:VEVENT\r\n")
soon = (now + timedelta(minutes=8)).replace(second=0, microsecond=0)
write(f"{cal}/trabajo/standup.ics", event("s1", "Standup del equipo", soon, 15,
      "LOCATION:Meet\r\nRRULE:FREQ=DAILY;COUNT=10\r\n"))
write(f"{cal}/personal/cine.ics", event("c1", "Cine con Ana",
      now.replace(hour=23, minute=0, second=0, microsecond=0) + timedelta(days=1), 120,
      "LOCATION:Cinemark\r\n"))
day = lambda n: (now + timedelta(days=n)).strftime("%Y%m%d")
write(f"{cal}/personal/feriado.ics",
      f"BEGIN:VEVENT\r\nUID:f1\r\nSUMMARY:Feriado\r\nDTSTART;VALUE=DATE:{day(5)}\r\n"
      f"DTEND;VALUE=DATE:{day(6)}\r\nEND:VEVENT\r\n")
PY
echo "fixtures in $t"
