pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Small equalizer-style bars; they move while `playing`.
Row {
    id: root

    property bool playing: false
    property color color: Theme.accent

    spacing: 2
    height: 14

    Repeater {
        model: 4
        delegate: Rectangle {
            id: bar
            required property int index
            property real level: 0.3

            anchors.bottom: parent.bottom
            width: 3
            height: Math.max(3, root.height * level)
            radius: 1.5
            color: root.color

            Behavior on height { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

            Timer {
                running: root.playing && root.visible
                repeat: true
                interval: 160 + bar.index * 35
                onTriggered: bar.level = 0.25 + Math.random() * 0.75
                onRunningChanged: if (!running) bar.level = 0.3
            }
        }
    }
}
