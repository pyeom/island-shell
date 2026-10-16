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
        ShellErrors {},
        Notification {},
        VolumeOsd {},
        Media {},
        Launcher {},
        PowerMenu {},
        QuickSettings {},
        PrivacyIndicator {},
        EventSoon {},
        SystemHeat {},
        RainAlert {},
        ScreenshotActivity {},
        WallpaperPicker {}
    ]

    // One activity per live activity pushed over D-Bus / islandctl.
    property Instantiator liveActivities: Instantiator {
        model: LiveActivities
        delegate: LiveActivity {}
    }

    property Instantiator islands: Instantiator {
        model: Qt.application.screens

        delegate: IslandWindow {
            id: window

            required property var modelData

            screenName: modelData.name
            // The active workspace on this monitor has a fullscreen window.
            readonly property bool fullscreen: {
                const monitor = Hyprland.monitors.find(m => m.name === screenName)
                const ws = monitor ? Hyprland.workspaces.find(w => w.id === monitor.activeWorkspace) : null
                return !!ws && ws.hasFullscreen
            }
            // Without Hyprland every island acts as focused.
            focused: !Hyprland.available || Hyprland.focusedMonitor === screenName
            width: Config.surfaceWidth
            height: Config.surfaceHeight
            exclusiveZone: Config.exclusiveZone
            inputRects: island.inputRects
            keyboardMode: island.keyboardMode

            // Catches clicks outside the island while a modal is open.
            property ScrimWindow scrim: ScrimWindow {
                screenName: window.screenName
                visible: island.modal
                width: 1
                height: 1

                MouseArea {
                    anchors.fill: parent
                    onClicked: island.collapse()
                }
            }

            Island {
                id: island
                anchors.fill: parent
                demo: shell.demo
                focused: window.focused
                fullscreen: window.fullscreen
            }
        }
    }
}
