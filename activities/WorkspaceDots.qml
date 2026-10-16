pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Workspaces of the monitor this island is on; the active one is a pill.
Row {
    id: root

    property bool interactive: false
    property int dotSize: 6

    readonly property string monitor: IslandScreen.name
    readonly property int activeId: {
        const m = Hyprland.monitors.find(m => m.name === root.monitor)
        return m ? m.activeWorkspace : -1
    }

    spacing: dotSize * 0.8

    Repeater {
        model: Hyprland.workspaces.filter(w => w.monitor === root.monitor)

        delegate: Rectangle {
            id: dot

            required property var modelData
            readonly property bool current: modelData.id === root.activeId

            anchors.verticalCenter: parent ? parent.verticalCenter : undefined
            width: current ? root.dotSize * 2.6 : root.dotSize
            height: root.dotSize
            radius: height / 2
            color: current ? Theme.foreground : Theme.secondaryForeground

            Behavior on width { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }

            TapHandler {
                enabled: root.interactive
                margin: root.dotSize
                onTapped: Hyprland.focusWorkspace(dot.modelData.id)
            }
        }
    }
}
