pragma Singleton

import QtQuick

// Shared wall clock, ticking once per second.
//   Text { text: Qt.formatTime(Clock.now, Config.clockFormat) }
QtObject {
    id: clock

    property date now: new Date()

    property Timer _timer: Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: clock.now = new Date()
    }
}
