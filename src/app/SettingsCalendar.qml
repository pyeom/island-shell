pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import IslandShell

// The calendar page of the settings window: the month with its events, one
// event in detail, and a form for new ones. Clicking an event in the island
// opens it here (`args`: { uid, start }).
ColumnLayout {
    id: cal

    property var args: ({})

    property date month: new Date(new Date().getFullYear(), new Date().getMonth(), 1)
    property date selected: _day(new Date())
    property var event: null // the event shown in detail
    property bool adding: false

    readonly property int firstWeekday: Qt.locale().firstDayOfWeek % 7 // 0 = Sunday
    readonly property date gridStart: {
        const offset = (month.getDay() - firstWeekday + 7) % 7
        return new Date(month.getFullYear(), month.getMonth(), 1 - offset)
    }
    readonly property date gridEnd: new Date(gridStart.getFullYear(), gridStart.getMonth(), gridStart.getDate() + 42)
    // Calendar.revision: re-read when the calendars reload.
    readonly property var monthEvents: Calendar.revision >= 0 ? Calendar.eventsBetween(gridStart, gridEnd) : []
    readonly property var dayEvents: eventsOn(selected)

    // The event detail or the form opened: the window scrolls to it.
    signal reveal(Item target)

    spacing: 12

    function _day(d: date): date {
        return new Date(d.getFullYear(), d.getMonth(), d.getDate())
    }
    function sameDay(a: date, b: date): bool {
        return a.toDateString() === b.toDateString()
    }
    function eventsOn(day: date): var {
        const start = _day(day)
        const end = new Date(start.getFullYear(), start.getMonth(), start.getDate() + 1)
        return monthEvents.filter(e => e.start < end && e.end > start)
    }
    function select(day: date) {
        selected = _day(day)
        if (day.getMonth() !== month.getMonth() || day.getFullYear() !== month.getFullYear())
            month = new Date(day.getFullYear(), day.getMonth(), 1)
    }
    function timeRange(e: var): string {
        if (e.allDay) {
            const last = new Date(e.end.getTime() - 1)
            return sameDay(e.start, last) ? qsTr("All day")
                 : qsTr("All day, until %1").arg(last.toLocaleDateString(Qt.locale(), "ddd d MMM"))
        }
        const from = Qt.formatTime(e.start, Config.clockFormat)
        const to = Qt.formatTime(e.end, Config.clockFormat)
        return sameDay(e.start, e.end) ? from + " – " + to
             : from + " – " + e.end.toLocaleDateString(Qt.locale(), "ddd d MMM") + " " + to
    }

    onArgsChanged: {
        if (!args || !args.uid)
            return
        const e = Calendar.findEvent(args.uid, args.start)
        select(args.start)
        adding = false
        event = e && e.uid ? e : null
        if (event)
            Qt.callLater(() => cal.reveal(detailCard))
    }
    onAddingChanged: if (adding) Qt.callLater(() => cal.reveal(formCard))

    // Month header.
    RowLayout {
        Layout.fillWidth: true
        spacing: 4
        Label {
            text: cal.month.toLocaleDateString(Qt.locale(), "MMMM yyyy")
            font.pixelSize: 18
            font.weight: Font.DemiBold
            font.capitalization: Font.Capitalize
        }
        Item { Layout.fillWidth: true }
        ToolButton {
            icon.source: Icons.source("go-previous-symbolic", "go-previous")
            onClicked: cal.month = new Date(cal.month.getFullYear(), cal.month.getMonth() - 1, 1)
        }
        Button {
            flat: true
            text: qsTr("Today")
            onClicked: cal.select(new Date())
        }
        ToolButton {
            icon.source: Icons.source("go-next-symbolic", "go-next")
            onClicked: cal.month = new Date(cal.month.getFullYear(), cal.month.getMonth() + 1, 1)
        }
        Button {
            highlighted: true
            text: qsTr("New event")
            enabled: Calendar.collections.length > 0 || Config.get("calendar.paths", []).length > 0
            onClicked: {
                cal.event = null
                cal.adding = true
            }
        }
    }

    // Weekday names.
    Row {
        Layout.fillWidth: true
        Repeater {
            model: 7
            delegate: Label {
                required property int index
                width: cal.width / 7
                horizontalAlignment: Text.AlignHCenter
                text: Qt.locale().dayName((cal.firstWeekday + index) % 7, Locale.ShortFormat)
                opacity: 0.6
                font.pixelSize: 12
                font.capitalization: Font.Capitalize
            }
        }
    }

    // Days: six weeks, up to two events per day.
    Grid {
        Layout.fillWidth: true
        columns: 7
        Repeater {
            model: 42
            delegate: Rectangle {
                id: cell
                required property int index
                readonly property date day: new Date(cal.gridStart.getFullYear(), cal.gridStart.getMonth(),
                                                     cal.gridStart.getDate() + index)
                readonly property bool inMonth: day.getMonth() === cal.month.getMonth()
                readonly property bool isToday: cal.sameDay(day, Clock.minute)
                readonly property bool isSelected: cal.sameDay(day, cal.selected)
                readonly property var events: cal.eventsOn(day)

                width: cal.width / 7
                height: 74
                color: isSelected ? Qt.rgba(1, 1, 1, 0.1) : area.containsMouse ? Qt.rgba(1, 1, 1, 0.05) : "transparent"
                border.width: isSelected ? 1 : 0
                border.color: Theme.accent
                radius: 8

                Column {
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 2

                    Rectangle {
                        width: 22
                        height: 22
                        radius: 11
                        color: cell.isToday ? Theme.accent : "transparent"
                        Label {
                            anchors.centerIn: parent
                            text: cell.day.getDate()
                            font.pixelSize: 12
                            font.weight: cell.isToday ? Font.Bold : Font.Normal
                            opacity: cell.inMonth ? 1 : 0.35
                        }
                    }
                    Repeater {
                        model: cell.events.slice(0, 2)
                        delegate: Rectangle {
                            id: chip
                            required property var modelData
                            width: parent.width
                            height: 16
                            radius: 4
                            color: Qt.alpha(chip.modelData.color || Theme.accent, 0.3)
                            opacity: cell.inMonth ? 1 : 0.5
                            Label {
                                anchors.fill: parent
                                anchors.leftMargin: 4
                                anchors.rightMargin: 2
                                verticalAlignment: Text.AlignVCenter
                                text: chip.modelData.summary
                                elide: Text.ElideRight
                                font.pixelSize: 10
                            }
                        }
                    }
                    Label {
                        visible: cell.events.length > 2
                        text: qsTr("+%1 more").arg(cell.events.length - 2)
                        font.pixelSize: 10
                        opacity: 0.6
                    }
                }

                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        cal.select(cell.day)
                        cal.event = null
                    }
                    onDoubleClicked: {
                        cal.select(cell.day)
                        cal.event = null
                        cal.adding = true
                    }
                }
            }
        }
    }

    // The selected day's events.
    Label {
        Layout.topMargin: 8
        text: cal.selected.toLocaleDateString(Qt.locale(), "dddd d MMMM")
        font.pixelSize: 15
        font.weight: Font.DemiBold
        font.capitalization: Font.Capitalize
    }
    Label {
        visible: cal.dayEvents.length === 0
        text: qsTr("No events. Double-click a day to add one.")
        opacity: 0.6
    }
    Repeater {
        model: cal.dayEvents
        delegate: ItemDelegate {
            id: entry
            required property var modelData
            readonly property bool current: cal.event !== null && cal.event.uid === modelData.uid
                                            && cal.event.start.getTime() === modelData.start.getTime()
            Layout.fillWidth: true
            highlighted: current
            onClicked: {
                cal.adding = false
                cal.event = entry.modelData
                Qt.callLater(() => cal.reveal(detailCard))
            }
            contentItem: RowLayout {
                spacing: 12
                Rectangle {
                    Layout.preferredWidth: 4
                    Layout.fillHeight: true
                    radius: 2
                    color: entry.modelData.color || Theme.accent
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Label {
                        Layout.fillWidth: true
                        text: entry.modelData.summary
                        elide: Text.ElideRight
                        font.weight: Font.Medium
                    }
                    Label {
                        Layout.fillWidth: true
                        text: cal.timeRange(entry.modelData)
                              + (entry.modelData.location ? "  ·  " + entry.modelData.location : "")
                        elide: Text.ElideRight
                        opacity: 0.6
                        font.pixelSize: 12
                    }
                }
            }
        }
    }

    // One event in detail.
    Rectangle {
        id: detailCard
        Layout.fillWidth: true
        visible: cal.event !== null && !cal.adding
        implicitHeight: detail.implicitHeight + 32
        radius: 12
        color: "#1c1c1c"

        ColumnLayout {
            id: detail
            x: 16
            y: 16
            width: parent.width - 32
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                Label {
                    Layout.fillWidth: true
                    text: cal.event ? cal.event.summary : ""
                    wrapMode: Text.Wrap
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                }
                ToolButton {
                    icon.source: Icons.source("window-close-symbolic", "window-close")
                    onClicked: cal.event = null
                }
            }
            Label {
                text: cal.event ? cal.event.start.toLocaleDateString(Qt.locale(), "dddd d MMMM yyyy")
                                  + "  ·  " + cal.timeRange(cal.event) : ""
                font.capitalization: Font.Capitalize
            }
            RowLayout {
                spacing: 8
                Rectangle {
                    Layout.preferredWidth: 10
                    Layout.preferredHeight: 10
                    radius: 5
                    color: cal.event && cal.event.color ? cal.event.color : Theme.accent
                }
                Label {
                    text: cal.event ? cal.event.calendar : ""
                    opacity: 0.7
                }
            }
            Label {
                Layout.fillWidth: true
                visible: !!cal.event && !!cal.event.location
                text: cal.event ? "📍 " + cal.event.location : ""
                wrapMode: Text.Wrap
            }
            TextArea {
                Layout.fillWidth: true
                visible: !!cal.event && !!cal.event.description
                text: cal.event ? cal.event.description : ""
                readOnly: true
                wrapMode: TextEdit.Wrap
                selectByMouse: true
                textFormat: TextEdit.PlainText
            }
            Button {
                flat: true
                visible: !!cal.event && !!cal.event.path
                text: qsTr("Open the .ics file")
                onClicked: Apps.openUrl("file://" + cal.event.path)
            }
        }
    }

    // New event.
    Rectangle {
        id: formCard
        Layout.fillWidth: true
        visible: cal.adding
        implicitHeight: form.implicitHeight + 32
        radius: 12
        color: "#1c1c1c"

        ColumnLayout {
            id: form
            x: 16
            y: 16
            width: parent.width - 32
            spacing: 10

            property string error: ""

            function reset() {
                summary.text = ""
                error = ""
            }

            // Fields start from the selected day each time the form opens.
            Connections {
                target: cal
                function onAddingChanged() {
                    if (cal.adding) {
                        form.reset()
                        date.text = Qt.formatDate(cal.selected, "yyyy-MM-dd")
                        summary.forceActiveFocus()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label {
                    Layout.fillWidth: true
                    text: qsTr("New event")
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                }
                ToolButton {
                    icon.source: Icons.source("window-close-symbolic", "window-close")
                    onClicked: cal.adding = false
                }
            }

            TextField {
                id: summary
                Layout.fillWidth: true
                placeholderText: qsTr("Title")
                onAccepted: add.clicked()
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                TextField {
                    id: date
                    Layout.preferredWidth: 140
                    placeholderText: "yyyy-MM-dd"
                    inputMask: "9999-99-99"
                }
                TextField {
                    id: startTime
                    Layout.preferredWidth: 90
                    enabled: !allDay.checked
                    text: "09:00"
                    inputMask: "99:99"
                }
                Label { text: "–"; enabled: !allDay.checked }
                TextField {
                    id: endTime
                    Layout.preferredWidth: 90
                    enabled: !allDay.checked
                    text: "10:00"
                    inputMask: "99:99"
                }
                Switch {
                    id: allDay
                    text: qsTr("All day")
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Label { text: qsTr("Calendar") }
                ComboBox {
                    id: collection
                    Layout.fillWidth: true
                    model: Calendar.collections.length > 0 ? Calendar.collections.map(c => c.name) : [qsTr("Local")]
                }
            }
            Label {
                Layout.fillWidth: true
                visible: form.error !== ""
                text: form.error
                color: Theme.danger
                wrapMode: Text.Wrap
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Button {
                    flat: true
                    text: qsTr("Cancel")
                    onClicked: cal.adding = false
                }
                Button {
                    id: add
                    highlighted: true
                    text: qsTr("Add")
                    onClicked: {
                        const d = date.text.split("-").map(Number)
                        const s = startTime.text.split(":").map(Number)
                        const e = endTime.text.split(":").map(Number)
                        const start = allDay.checked ? new Date(d[0], d[1] - 1, d[2])
                                                     : new Date(d[0], d[1] - 1, d[2], s[0], s[1])
                        let end = allDay.checked ? new Date(d[0], d[1] - 1, d[2] + 1)
                                                 : new Date(d[0], d[1] - 1, d[2], e[0], e[1])
                        if (!allDay.checked && end <= start) // ends after midnight
                            end = new Date(end.getTime() + 24 * 3600 * 1000)
                        if (summary.text.trim() === "") {
                            form.error = qsTr("The event needs a title.")
                            return
                        }
                        if (isNaN(start.getTime()) || isNaN(end.getTime()) || d[1] < 1 || d[1] > 12
                                || s[0] > 23 || s[1] > 59 || e[0] > 23 || e[1] > 59) {
                            form.error = qsTr("Check the date and times.")
                            return
                        }
                        const path = Calendar.collections.length > 0
                                   ? Calendar.collections[collection.currentIndex].path : ""
                        if (!Calendar.addEvent(summary.text, start, end, allDay.checked, path)) {
                            form.error = qsTr("Could not save the event.")
                            return
                        }
                        cal.adding = false
                        cal.select(start)
                    }
                }
            }
        }
    }
}
