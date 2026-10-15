import QtQuick
import IslandShell

// Root object: one island per monitor, recreated on hotplug.
QtObject {
    id: shell

    property bool demo: false

    property Instantiator islands: Instantiator {
        model: Qt.application.screens

        delegate: IslandWindow {
            id: window

            required property var modelData

            screenName: modelData.name
            width: Metrics.surfaceWidth
            height: Metrics.surfaceHeight
            exclusiveZone: Metrics.exclusiveZone
            inputRect: island.pillRect

            Island {
                id: island
                anchors.fill: parent
                demo: shell.demo
            }
        }
    }
}
