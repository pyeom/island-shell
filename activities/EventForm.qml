pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// New calendar event form; the calendar's "+" opens it on the selected day.
Activity {
    id: root

    property bool open: false
    property date day: new Date()

    name: "event-form"
    priority: 1000
    modal: true
    active: open

    onDismissRequested: open = false

    property Connections _calendar: Connections {
        target: Calendar
        function onNewEventRequested(day: date) {
            root.day = day
            root.open = true
        }
    }

    expanded: Component {
        Item {
            id: form

            property bool allDay: false
            property int minutes: 60
            property int collection: 0
            property date day: root.day

            readonly property var startTime: {
                const m = time.text.match(/^(\d{1,2}):(\d{2})$/)
                return m ? { h: Number(m[1]), m: Number(m[2]) } : null
            }
            readonly property bool valid: title.text.trim().length > 0
                                          && (allDay || (startTime && startTime.h < 24 && startTime.m < 60))
                                          && Calendar.collections.length > 0

            function save() {
                if (!valid)
                    return
                const start = new Date(day.getFullYear(), day.getMonth(), day.getDate(),
                                       allDay ? 0 : startTime.h, allDay ? 0 : startTime.m)
                const end = allDay ? new Date(start.getTime() + 86400000)
                                   : new Date(start.getTime() + minutes * 60000)
                const target = Calendar.collections[collection]
                if (Calendar.addEvent(title.text, start, end, allDay, target ? target.path : ""))
                    root.open = false
            }

            implicitWidth: 460
            implicitHeight: column.implicitHeight + 36

            component Chip: IconButton {
                height: 28
            }

            component Field: Rectangle {
                height: 38
                radius: 12
                color: Qt.rgba(1, 1, 1, 0.1)
            }

            Column {
                id: column
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 18
                spacing: 12

                Text {
                    text: qsTr("New event")
                    color: Theme.foreground
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                }

                Field {
                    width: parent.width
                    TextInput {
                        id: title
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        verticalAlignment: TextInput.AlignVCenter
                        color: Theme.foreground
                        font.pixelSize: 14
                        clip: true
                        focus: true
                        Component.onCompleted: forceActiveFocus()
                        Keys.onReturnPressed: form.save()
                        Keys.onEscapePressed: root.open = false
                        KeyNavigation.tab: time
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            visible: !title.text
                            text: qsTr("Title")
                            color: Theme.secondaryForeground
                            font: title.font
                        }
                    }
                }

                // Day.
                Row {
                    spacing: 8
                    IconButton {
                        width: 28
                        height: 28
                        icon: "go-previous-symbolic"
                        iconSize: 12
                        onClicked: form.day = new Date(form.day.getTime() - 86400000)
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 170
                        horizontalAlignment: Text.AlignHCenter
                        text: form.day.toLocaleDateString(Qt.locale(), "dddd d MMMM")
                        color: Theme.foreground
                        font.pixelSize: 13
                        font.capitalization: Font.Capitalize
                    }
                    IconButton {
                        width: 28
                        height: 28
                        icon: "go-next-symbolic"
                        iconSize: 12
                        onClicked: form.day = new Date(form.day.getTime() + 86400000)
                    }
                }

                // Time and duration.
                Row {
                    spacing: 8
                    Chip {
                        text: qsTr("All day")
                        highlighted: form.allDay
                        onClicked: form.allDay = !form.allDay
                    }
                    Field {
                        width: 74
                        height: 28
                        visible: !form.allDay
                        TextInput {
                            id: time
                            anchors.fill: parent
                            horizontalAlignment: TextInput.AlignHCenter
                            verticalAlignment: TextInput.AlignVCenter
                            // Today: the next full hour (at most 23:00); other days: 09:00.
                            text: {
                                const now = new Date()
                                if (form.day.toDateString() !== now.toDateString())
                                    return "09:00"
                                return String(Math.min(now.getHours() + 1, 23)).padStart(2, "0") + ":00"
                            }
                            inputMask: "99:99"
                            color: form.startTime ? Theme.foreground : Theme.danger
                            font.pixelSize: 13
                            font.features: { "tnum": 1 }
                            Keys.onReturnPressed: form.save()
                            Keys.onEscapePressed: root.open = false
                            KeyNavigation.tab: title
                        }
                    }
                    Repeater {
                        model: form.allDay ? [] : [30, 60, 120]
                        delegate: Chip {
                            required property int modelData
                            text: modelData < 60 ? modelData + " min" : (modelData / 60) + " h"
                            highlighted: form.minutes === modelData
                            onClicked: form.minutes = modelData
                        }
                    }
                }

                // Calendar.
                Flow {
                    width: parent.width
                    spacing: 6
                    visible: Calendar.collections.length > 1
                    Repeater {
                        model: Calendar.collections
                        delegate: Chip {
                            required property var modelData
                            required property int index
                            text: modelData.name
                            highlighted: form.collection === index
                            onClicked: form.collection = index
                        }
                    }
                }

                Text {
                    visible: Calendar.collections.length === 0
                    width: parent.width
                    wrapMode: Text.Wrap
                    text: qsTr("No calendar folder found; set [calendar] paths (see README).")
                    color: Theme.danger
                    font.pixelSize: 12
                }

                Row {
                    anchors.right: parent.right
                    spacing: 8
                    IconButton {
                        text: qsTr("Cancel")
                        onClicked: root.open = false
                    }
                    IconButton {
                        text: qsTr("Save")
                        highlighted: form.valid
                        opacity: form.valid ? 1 : 0.5
                        onClicked: form.save()
                    }
                }
            }
        }
    }
}
