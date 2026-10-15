pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// The next calendar event, from [calendar] reminder_minutes before it starts.
Activity {
    id: root

    readonly property var event: Calendar.soon
    readonly property bool has: event.summary !== undefined
    readonly property int minutesLeft: has ? Math.ceil((event.start - Clock.now) / 60000) : 0

    name: "event"
    priority: 55
    active: has

    onHasChanged: if (has) alert(5000)

    readonly property string when: minutesLeft > 0 ? qsTr("in %n min", "", minutesLeft) : qsTr("now")

    compactLeading: Component {
        Row {
            spacing: 8
            IconImage {
                anchors.verticalCenter: parent.verticalCenter
                source: Icons.source("x-office-calendar-symbolic", "office-calendar")
                size: 16
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(implicitWidth, 140)
                text: root.event.summary
                elide: Text.ElideRight
                color: Theme.foreground
                font.pixelSize: 12
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.when
                color: Theme.accent
                font.pixelSize: 12
                font.weight: Font.DemiBold
            }
        }
    }
    compactTrailing: Component { ClockText {} }
    minimal: Component {
        IconImage {
            source: Icons.source("x-office-calendar-symbolic", "office-calendar")
            size: 16
        }
    }

    expanded: Component {
        Item {
            implicitWidth: 380
            implicitHeight: column.implicitHeight + 36
            Column {
                id: column
                anchors.fill: parent
                anchors.margins: 18
                spacing: 4
                Text {
                    text: root.when
                    color: Theme.accent
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }
                Text {
                    width: parent.width
                    text: root.event.summary
                    wrapMode: Text.Wrap
                    color: Theme.foreground
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                }
                Text {
                    text: Qt.formatTime(root.event.start, Config.clockFormat) + " – "
                        + Qt.formatTime(root.event.end, Config.clockFormat)
                        + (root.event.calendar ? "  ·  " + root.event.calendar : "")
                    color: Theme.secondaryForeground
                    font.pixelSize: 12
                }
                Text {
                    width: parent.width
                    visible: !!root.event.location
                    text: root.event.location
                    elide: Text.ElideRight
                    color: Theme.secondaryForeground
                    font.pixelSize: 12
                }
            }
        }
    }
}
