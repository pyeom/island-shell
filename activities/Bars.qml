pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Small equalizer-style bars; they move while `playing`. They step 8 times
// a second without animating in between, like a level meter: smooth motion
// would make the island draw a frame on every refresh (144 Hz monitors) for
// as long as music plays.
Row {
    id: root

    property bool playing: false
    property color color: Theme.accent

    spacing: 2
    height: 14

    Timer {
        id: tick
        running: root.playing && root.visible
        repeat: true
        interval: 125
        onTriggered: {
            for (let i = 0; i < bars.count; ++i)
                (bars.itemAt(i) as Rectangle).level = 0.25 + Math.random() * 0.75
        }
        onRunningChanged: {
            if (running)
                return
            for (let i = 0; i < bars.count; ++i)
                (bars.itemAt(i) as Rectangle).level = 0.3
        }
    }

    Repeater {
        id: bars
        model: 4
        delegate: Rectangle {
            property real level: 0.3

            anchors.bottom: parent.bottom
            width: 3
            height: Math.max(3, root.height * level)
            radius: 1.5
            color: root.color
        }
    }
}
