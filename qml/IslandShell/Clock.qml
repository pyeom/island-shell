pragma Singleton

import QtQuick

// Shared wall clock, ticking once per second.
//   Text { text: Qt.formatTime(Clock.minute, "HH:mm") }
// Bind to `minute` unless you show seconds: it changes once a minute, so
// bindings on it are not re-evaluated every second.
QtObject {
    id: clock

    property date now: new Date()
    // `now` without seconds.
    property date minute: _floor(new Date())
    // The configured clock format shows seconds.
    readonly property bool formatHasSeconds: Config.clockFormat.indexOf("s") >= 0

    function _floor(d: date): date {
        return new Date(d.getFullYear(), d.getMonth(), d.getDate(), d.getHours(), d.getMinutes())
    }

    property Timer _timer: Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            const d = new Date()
            clock.now = d
            if (d - clock.minute >= 60000 || d < clock.minute) // next minute, or the clock went back
                clock.minute = clock._floor(d)
        }
    }
}
