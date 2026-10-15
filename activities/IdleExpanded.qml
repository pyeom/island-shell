import QtQuick
import IslandShell

// Clock, workspaces and (with calendars configured) the month and upcoming
// events; notification history below.
Item {
    id: root

    readonly property bool withCalendar: Calendar.available

    implicitWidth: withCalendar ? 540 : Config.expandedWidth
    implicitHeight: Math.max(Config.expandedHeight, column.implicitHeight + 36)

    Column {
        id: column
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: root.withCalendar || history.visible ? 18
                         : (parent.height - implicitHeight) / 2
        width: parent.width - 36
        spacing: 14

        Row {
            width: parent.width
            spacing: 20

            Column {
                width: root.withCalendar ? parent.width - month.width - 20 : parent.width
                anchors.verticalCenter: root.withCalendar ? undefined : parent.verticalCenter
                spacing: 4

                Text {
                    anchors.horizontalCenter: root.withCalendar ? undefined : parent.horizontalCenter
                    text: Qt.formatTime(Clock.now, Config.clockFormat)
                    color: Theme.foreground
                    font.pixelSize: root.withCalendar ? 48 : 56
                    font.weight: Font.Bold
                    font.features: { "tnum": 1 }
                }
                Text {
                    anchors.horizontalCenter: root.withCalendar ? undefined : parent.horizontalCenter
                    text: Clock.now.toLocaleDateString(Qt.locale(), "dddd d MMMM yyyy")
                    color: Theme.secondaryForeground
                    font.pixelSize: 14
                }
                Item { width: 1; height: 8 }
                WorkspaceDots {
                    anchors.horizontalCenter: root.withCalendar ? undefined : parent.horizontalCenter
                    interactive: true
                    dotSize: 10
                }
                Item { width: 1; height: 10; visible: root.withCalendar }
                EventList {
                    visible: root.withCalendar
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
                visible: root.withCalendar
            }
        }

        NotificationHistory {
            id: history
            width: parent.width
            maxHeight: Config.surfaceHeight - Config.topMargin - (root.withCalendar ? 330 : 220)
        }
    }
}
