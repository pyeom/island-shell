#!/usr/bin/env bash
# Stops the fake system services started by fake-system.sh (by PID, so it
# never matches unrelated processes).
pidfile=/tmp/ish-fake-system.pid
[[ -f $pidfile ]] && kill "$(cat "$pidfile")" 2>/dev/null
rm -f "$pidfile"
