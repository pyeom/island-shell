// Example user activity. Copy to ~/.config/island-shell/activities/ and it
// loads (and reloads on save) without restarting the shell.
//
//   islandctl expand pomodoro      # open it
//
// Click the expanded view to start or pause; the compact pill shows the
// remaining time while it runs.

pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

Activity {
    id: root

    name: "pomodoro"
    priority: 40
    // Only take over the pill while running; always reachable via islandctl.
    active: running

    property int length: Config.get("activities.pomodoro.minutes", 25) * 60
    property int remaining: length
    property bool running: false

    readonly property string label: {
        const m = Math.floor(remaining / 60)
        const s = remaining % 60
        return m + ":" + (s < 10 ? "0" : "") + s
    }

    Timer {
        interval: 1000
        repeat: true
        running: root.running
        onTriggered: {
            root.remaining -= 1
            if (root.remaining <= 0) {
                root.running = false
                root.remaining = root.length
                root.alert(5000)
            }
        }
    }

    compactLeading: Component {
        Rectangle {
            width: 12
            height: 12
            radius: 6
            color: Theme.accent
        }
    }

    compactTrailing: Component {
        Text {
            text: root.label
            color: Theme.foreground
            font.pixelSize: 14
            font.features: { "tnum": 1 }
        }
    }

    minimal: Component {
        Rectangle {
            width: 12
            height: 12
            radius: 6
            color: Theme.accent
        }
    }

    expanded: Component {
        Item {
            implicitWidth: 320
            implicitHeight: 140

            Column {
                anchors.centerIn: parent
                spacing: 6

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: root.label
                    color: Theme.foreground
                    font.pixelSize: 44
                    font.weight: Font.Bold
                    font.features: { "tnum": 1 }
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: root.running ? qsTr("Click to pause") : qsTr("Click to start")
                    color: Theme.secondaryForeground
                    font.pixelSize: 13
                }
            }

            TapHandler {
                onTapped: root.running = !root.running
            }
        }
    }
}
