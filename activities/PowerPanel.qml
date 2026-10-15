pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Five session actions. Shutdown and reboot need press-and-hold (mouse or
// Enter) so a stray click cannot end the session.
Item {
    id: panel

    signal close()

    readonly property var items: [
        { action: "shutdown", label: qsTr("Shut down"), icon: "system-shutdown-symbolic", hold: true },
        { action: "reboot", label: qsTr("Restart"), icon: "system-reboot-symbolic", hold: true },
        { action: "logout", label: qsTr("Log out"), icon: "system-log-out-symbolic", hold: false },
        { action: "suspend", label: qsTr("Suspend"), icon: "system-suspend-symbolic", hold: false },
        { action: "lock", label: qsTr("Lock"), icon: "system-lock-screen-symbolic", hold: false }
    ]
    property int selected: 0
    property int holding: -1
    property real holdProgress: 0

    implicitWidth: 470
    implicitHeight: 150

    function trigger(index: int) {
        const item = items[index]
        if (item.hold) {
            holding = index
            holdAnim.restart()
        } else {
            run(index)
        }
    }

    function release() {
        holdAnim.stop()
        holding = -1
        holdProgress = 0
    }

    function run(index: int) {
        Power.run(items[index].action)
        panel.close()
    }

    NumberAnimation {
        id: holdAnim
        target: panel
        property: "holdProgress"
        from: 0
        to: 1
        duration: Config.powerHoldMs
        onFinished: {
            if (panel.holding >= 0)
                panel.run(panel.holding)
        }
    }

    focus: true
    Component.onCompleted: forceActiveFocus()

    Keys.onPressed: event => {
        if (event.isAutoRepeat) {
            event.accepted = true
            return
        }
        switch (event.key) {
        case Qt.Key_Escape:
            panel.close()
            break
        case Qt.Key_Left:
        case Qt.Key_Backtab:
            panel.release()
            panel.selected = (panel.selected + panel.items.length - 1) % panel.items.length
            break
        case Qt.Key_Right:
        case Qt.Key_Tab:
            panel.release()
            panel.selected = (panel.selected + 1) % panel.items.length
            break
        case Qt.Key_Return:
        case Qt.Key_Enter:
        case Qt.Key_Space:
            panel.trigger(panel.selected)
            break
        default:
            return
        }
        event.accepted = true
    }
    Keys.onReleased: event => {
        if (!event.isAutoRepeat && (event.key === Qt.Key_Return || event.key === Qt.Key_Enter
                                    || event.key === Qt.Key_Space))
            panel.release()
    }

    Row {
        id: buttons
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 22
        spacing: 18

        Repeater {
            model: panel.items
            delegate: Column {
                id: cell

                required property var modelData
                required property int index
                readonly property bool current: index === panel.selected

                spacing: 8

                Item {
                    width: 64
                    height: 64
                    anchors.horizontalCenter: parent.horizontalCenter

                    Rectangle {
                        anchors.fill: parent
                        radius: width / 2
                        color: cell.current ? Qt.rgba(1, 1, 1, 0.22)
                             : area.containsMouse ? Qt.rgba(1, 1, 1, 0.14)
                             : Qt.rgba(1, 1, 1, 0.08)
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }
                    ProgressRing {
                        anchors.centerIn: parent
                        visible: panel.holding === cell.index
                        size: 70
                        lineWidth: 3
                        progress: panel.holdProgress
                        color: cell.modelData.action === "shutdown" ? Theme.danger : Theme.accent
                        trackColor: "transparent"
                    }
                    IconImage {
                        anchors.centerIn: parent
                        source: Icons.source(cell.modelData.icon)
                        size: 26
                    }

                    MouseArea {
                        id: area
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onEntered: panel.selected = cell.index
                        onPressed: panel.trigger(cell.index)
                        onReleased: if (cell.modelData.hold) panel.release()
                        onCanceled: panel.release()
                    }
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: cell.modelData.label
                    color: cell.current ? Theme.foreground : Theme.secondaryForeground
                    font.pixelSize: 12
                }
            }
        }
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12
        text: Power.dryRun ? qsTr("Dry run: actions are only logged")
            : panel.items[panel.selected].hold ? qsTr("Hold to confirm") : " "
        color: Theme.secondaryForeground
        font.pixelSize: 11
    }
}
