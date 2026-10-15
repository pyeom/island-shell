pragma ComponentBehavior: Bound

import QtQuick
import IslandShell
import IslandShell.Activities

// Root object: built-in activities plus one island per monitor (recreated on
// hotplug). User activities are created from C++ by UserActivities.
QtObject {
    id: shell

    property bool demo: false

    property list<QtObject> builtins: [
        Idle {},
        ShellErrors {}
    ]

    property Instantiator islands: Instantiator {
        model: Qt.application.screens

        delegate: IslandWindow {
            id: window

            required property var modelData

            screenName: modelData.name
            // Without Hyprland every island acts as focused.
            focused: !Hyprland.available || Hyprland.focusedMonitor === screenName
            width: Config.surfaceWidth
            height: Config.surfaceHeight
            exclusiveZone: Config.exclusiveZone
            inputRects: island.inputRects

            Island {
                id: island
                anchors.fill: parent
                demo: shell.demo
                focused: window.focused
            }
        }
    }
}
