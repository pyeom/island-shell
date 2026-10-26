pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Timers started from the launcher ("15 min", "timer 5 pasta"): the soonest
// counts down in the pill; a finished one rings, expands the island and
// stays until dismissed.
Activity {
    id: root

    readonly property var timers: Timers.timers
    readonly property var first: timers.length > 0 ? timers[0] : null
    readonly property bool ringing: first !== null && first.done

    name: "timer"
    priority: ringing ? 95 : 52
    active: first !== null

    property Connections _timers: Connections {
        target: Timers
        function onFinished() {
            root.alert(10000)
        }
    }

    // Seconds left, from the shared clock.
    function remaining(timer: var): int {
        return Math.max(0, Math.ceil((timer.endsAt - Clock.now) / 1000))
    }
    // 4:05, 1:02:03
    function clock(seconds: int): string {
        const h = Math.floor(seconds / 3600)
        const m = Math.floor(seconds % 3600 / 60)
        const s = seconds % 60
        const mm = h > 0 && m < 10 ? "0" + m : String(m)
        return (h > 0 ? h + ":" + mm : mm) + ":" + (s < 10 ? "0" + s : s)
    }
    // 1 h 2 min, 12 s
    function duration(seconds: int): string {
        const parts = []
        if (seconds >= 3600)
            parts.push(Math.floor(seconds / 3600) + " h")
        if (seconds % 3600 >= 60)
            parts.push(Math.floor(seconds % 3600 / 60) + " min")
        if (seconds % 60 > 0)
            parts.push(seconds % 60 + " s")
        return parts.join(" ")
    }
    function progress(timer: var): real {
        return timer.done ? 1 : 1 - remaining(timer) / timer.duration
    }

    readonly property color doneColor: "#ff9f0a"

    component Glyph: Item {
        id: glyph
        property var timer: root.first
        property int size: 20
        width: size
        height: size
        ProgressRing {
            anchors.centerIn: parent
            visible: glyph.timer !== null && !glyph.timer.done
            size: parent.size
            progress: glyph.timer ? root.progress(glyph.timer) : 0
            color: Theme.accent
        }
        IconImage {
            anchors.centerIn: parent
            visible: glyph.timer !== null && glyph.timer.done
            source: Icons.source("alarm-symbolic", "kalarm")
            size: parent.size - 2
        }
    }

    compactLeading: Component { Glyph {} }

    compactTrailing: Component {
        Text {
            text: !root.first ? ""
                : root.first.done ? (root.first.label || qsTr("Time's up"))
                : root.clock(root.remaining(root.first))
            color: root.ringing ? root.doneColor : Theme.foreground
            font.pixelSize: 13
            font.weight: Font.DemiBold
            font.features: { "tnum": 1 }
        }
    }

    minimal: Component { Glyph { size: 18 } }

    expanded: Component {
        Item {
            implicitWidth: 400
            implicitHeight: list.implicitHeight + 36

            Column {
                id: list
                anchors.fill: parent
                anchors.margins: 18
                spacing: 14

                Repeater {
                    model: root.timers
                    delegate: Column {
                        id: entry
                        required property var modelData
                        width: list.width
                        spacing: 8

                        Row {
                            width: parent.width
                            spacing: 12

                            Glyph {
                                anchors.verticalCenter: parent.verticalCenter
                                timer: entry.modelData
                                size: 32
                            }
                            Column {
                                anchors.verticalCenter: parent.verticalCenter
                                width: parent.width - 32 - 12 - buttons.width - 12
                                Text {
                                    width: parent.width
                                    text: entry.modelData.done ? qsTr("Time's up")
                                                               : root.clock(root.remaining(entry.modelData))
                                    color: entry.modelData.done ? root.doneColor : Theme.foreground
                                    font.pixelSize: 22
                                    font.weight: Font.DemiBold
                                    font.features: { "tnum": 1 }
                                }
                                Text {
                                    width: parent.width
                                    text: (entry.modelData.label ? entry.modelData.label + "  ·  " : "")
                                          + qsTr("%1 timer").arg(root.duration(entry.modelData.duration))
                                    elide: Text.ElideRight
                                    color: Theme.secondaryForeground
                                    font.pixelSize: 12
                                }
                            }
                            Row {
                                id: buttons
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 8
                                IconButton {
                                    height: 30
                                    text: qsTr("+1 min")
                                    onClicked: Timers.extend(entry.modelData.id, 60)
                                }
                                IconButton {
                                    height: 30
                                    width: 30
                                    icon: entry.modelData.done ? "object-select-symbolic" : "window-close-symbolic"
                                    iconSize: 14
                                    highlighted: entry.modelData.done
                                    onClicked: Timers.remove(entry.modelData.id)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
