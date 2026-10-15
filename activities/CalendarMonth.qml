pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Month grid with today highlighted and dots on days with events. Arrows
// change month; clicking a day selects it.
Column {
    id: root

    property date shown: new Date(Clock.now.getFullYear(), Clock.now.getMonth(), 1)
    property date selected: Clock.now

    readonly property int year: shown.getFullYear()
    readonly property int month: shown.getMonth() // 0-based
    // Re-evaluated when calendars reload (revision changes).
    readonly property var busy: Calendar.revision >= 0 ? Calendar.busyDays(year, month + 1) : []
    // Monday-first offset of the 1st.
    readonly property int offset: (new Date(year, month, 1).getDay() + 6) % 7
    readonly property int daysInMonth: new Date(year, month + 1, 0).getDate()
    readonly property int cell: 30

    spacing: 6

    function sameDay(a: date, b: date): bool {
        return a.getFullYear() === b.getFullYear() && a.getMonth() === b.getMonth()
            && a.getDate() === b.getDate()
    }

    Item {
        width: root.cell * 7
        height: 24

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.shown.toLocaleDateString(Qt.locale(), "MMMM yyyy")
            color: Theme.foreground
            font.pixelSize: 13
            font.weight: Font.DemiBold
            font.capitalization: Font.Capitalize
        }
        Row {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4
            IconButton {
                width: 24
                height: 24
                icon: "go-previous-symbolic"
                iconSize: 12
                onClicked: root.shown = new Date(root.year, root.month - 1, 1)
            }
            IconButton {
                width: 24
                height: 24
                icon: "go-next-symbolic"
                iconSize: 12
                onClicked: root.shown = new Date(root.year, root.month + 1, 1)
            }
        }
    }

    Grid {
        columns: 7

        Repeater {
            // Weekday initials, Monday first, from the locale.
            model: 7
            delegate: Text {
                required property int index
                width: root.cell
                horizontalAlignment: Text.AlignHCenter
                text: Qt.locale().dayName((index + 1) % 7, Locale.NarrowFormat)
                color: Theme.secondaryForeground
                font.pixelSize: 10
            }
        }

        Repeater {
            model: 42
            delegate: Item {
                id: day

                required property int index
                readonly property int number: index - root.offset + 1
                readonly property bool inMonth: number >= 1 && number <= root.daysInMonth
                readonly property date value: new Date(root.year, root.month, number)
                readonly property bool today: inMonth && root.sameDay(value, Clock.now)
                readonly property bool isSelected: inMonth && root.sameDay(value, root.selected)

                width: root.cell
                height: inMonth || index < 35 ? root.cell : 0
                visible: height > 0

                Rectangle {
                    anchors.centerIn: parent
                    width: 26
                    height: 26
                    radius: 13
                    visible: day.inMonth && (day.today || day.isSelected)
                    color: day.today ? Theme.accent : Qt.rgba(1, 1, 1, 0.16)
                }
                Text {
                    anchors.centerIn: parent
                    visible: day.inMonth
                    text: day.number
                    color: Theme.foreground
                    font.pixelSize: 12
                    font.weight: day.today ? Font.Bold : Font.Normal
                    font.features: { "tnum": 1 }
                }
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 1
                    width: 4
                    height: 4
                    radius: 2
                    visible: day.inMonth && root.busy.indexOf(day.number) >= 0
                    color: day.today ? Theme.foreground : Theme.accent
                }
                MouseArea {
                    anchors.fill: parent
                    enabled: day.inMonth
                    onClicked: root.selected = day.value
                }
            }
        }
    }
}
