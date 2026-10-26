pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// A short list of events: either one day's, or the next upcoming ones.
// Clicking one opens it in the settings window's calendar.
Column {
    id: root

    property var events: []
    property string emptyText: ""

    spacing: 6

    function timeLabel(e: var): string {
        if (e.allDay)
            return qsTr("All day")
        const sameDay = e.start.toDateString() === Clock.minute.toDateString()
        const time = Qt.formatTime(e.start, Config.clockFormat)
        return sameDay ? time : e.start.toLocaleDateString(Qt.locale(), "ddd d") + "  " + time
    }

    Text {
        visible: root.events.length === 0 && root.emptyText !== ""
        text: root.emptyText
        color: Theme.secondaryForeground
        font.pixelSize: 12
    }

    Repeater {
        model: root.events
        delegate: Item {
            id: item
            required property var modelData
            width: root.width
            height: line.implicitHeight

            Row {
                id: line
                width: parent.width
                spacing: 8

                Rectangle {
                    width: 3
                    height: 30
                    radius: 1.5
                    color: item.modelData.color || Theme.accent
                }
                Column {
                    width: parent.width - 11
                    Text {
                        width: parent.width
                        text: item.modelData.summary
                        elide: Text.ElideRight
                        color: Theme.foreground
                        font.pixelSize: 12
                        font.weight: Font.Medium
                    }
                    Text {
                        width: parent.width
                        text: root.timeLabel(item.modelData) + (item.modelData.location ? "  ·  " + item.modelData.location : "")
                        elide: Text.ElideRight
                        color: Theme.secondaryForeground
                        font.pixelSize: 11
                    }
                }
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    ShellControl.Collapse()
                    ShellControl.showSettingsPage("calendar", { uid: item.modelData.uid, start: item.modelData.start })
                }
            }
        }
    }
}
