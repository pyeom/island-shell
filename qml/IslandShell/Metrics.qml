pragma Singleton

import QtQuick

// Island geometry. Replaced by config.toml values in F1.
QtObject {
    // Layer surface: created once at the largest size and never resized.
    readonly property int surfaceWidth: 600
    readonly property int surfaceHeight: 420
    readonly property int exclusiveZone: 44

    readonly property int topMargin: 6

    readonly property size compact: Qt.size(190, 36)
    readonly property size peek: Qt.size(240, 42)
    readonly property size expanded: Qt.size(420, 200)

    readonly property real compactRadius: compact.height / 2
    readonly property real expandedRadius: 34
}
