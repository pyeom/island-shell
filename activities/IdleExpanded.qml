import QtQuick
import IslandShell

// Clock, workspaces, the month and upcoming events; notification history
// below.
Item {
    id: root

    implicitWidth: 540
    implicitHeight: Math.max(Config.expandedHeight, column.implicitHeight + 36)

    Column {
        id: column
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 18
        width: parent.width - 36
        spacing: 14

        Row {
            width: parent.width
            spacing: 20

            Column {
                width: parent.width - month.width - 20
                spacing: 4

                Text {
                    text: Qt.formatTime(Clock.now, Config.clockFormat)
                    color: Theme.foreground
                    font.pixelSize: 48
                    font.weight: Font.Bold
                    font.features: { "tnum": 1 }
                }
                Text {
                    text: Clock.now.toLocaleDateString(Qt.locale(), "dddd d MMMM yyyy")
                    color: Theme.secondaryForeground
                    font.pixelSize: 14
                }
                Item { width: 1; height: 6; visible: Weather.available }
                WeatherLine {
                }
                Item { width: 1; height: 8 }
                WorkspaceDots {
                    interactive: true
                    dotSize: 10
                }
                Item { width: 1; height: 10 }
                EventList {
                    width: parent.width
                    readonly property bool isToday: month.selected.toDateString() === Clock.now.toDateString()
                    // Calendar.revision: re-evaluate when calendars reload.
                    events: Calendar.revision < 0 ? []
                          : isToday ? Calendar.upcoming(3)
                          : Calendar.eventsOn(month.selected).slice(0, 3)
                    emptyText: isToday ? qsTr("No upcoming events") : qsTr("No events")
                }
            }

            CalendarMonth {
                id: month
            }
        }

        // Who is using the microphone, camera or screen.
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: Privacy.any
            spacing: 16
            Repeater {
                model: [
                    { on: Privacy.microphone, color: "#ff9f0a", label: qsTr("Microphone"), detail: Privacy.microphoneApps.join(", ") },
                    { on: Privacy.camera, color: "#30d158", label: qsTr("Camera"), detail: Privacy.cameraApps.join(", ") },
                    { on: Privacy.screen, color: "#bf5af2", label: qsTr("Screen"), detail: qsTr("being shared") }
                ].filter(i => i.on)
                delegate: Row {
                    id: use
                    required property var modelData
                    spacing: 6
                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 8
                        height: 8
                        radius: 4
                        color: use.modelData.color
                    }
                    Text {
                        text: use.modelData.label
                        color: Theme.foreground
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                    }
                    Text {
                        text: use.modelData.detail
                        color: Theme.secondaryForeground
                        font.pixelSize: 12
                    }
                }
            }
        }

        SystemStatsRow {
            anchors.horizontalCenter: parent.horizontalCenter
        }

        // Shortcuts, so the expanded island is useful even with nothing else
        // to show.
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 10

            IconButton {
                icon: "system-search-symbolic"
                text: qsTr("Search apps")
                onClicked: ShellControl.ToggleLauncher()
            }
            IconButton {
                icon: "preferences-system-symbolic"
                text: qsTr("Quick settings")
                onClicked: ShellControl.ShowQuickSettings()
            }
            IconButton {
                icon: ShellControl.dnd ? "notifications-disabled-symbolic" : "notifications-symbolic"
                highlighted: ShellControl.dnd
                onClicked: ShellControl.dnd = !ShellControl.dnd
            }
            IconButton {
                icon: "system-shutdown-symbolic"
                onClicked: ShellControl.ShowPower()
            }
        }

        NotificationHistory {
            id: history
            width: parent.width
            maxHeight: Config.surfaceHeight - Config.topMargin - (Privacy.any ? 360 : 330)
        }
    }
}
