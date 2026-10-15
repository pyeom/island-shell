import QtQuick
import IslandShell

// The pill. All motion happens inside the fixed-size layer surface; only
// `pillRect` (the input region) follows it.
Item {
    id: root

    property bool demo: false

    // compact | peek | expanded
    property string mode: "compact"

    readonly property rect pillRect: Qt.rect(pill.x, pill.y, pill.width, pill.height)

    readonly property size targetSize: mode === "expanded" ? Metrics.expanded
                                     : mode === "peek" ? Metrics.peek
                                     : Metrics.compact

    Spring {
        id: widthSpring
        target: root.targetSize.width
        response: 0.42
        dampingFraction: 0.72
    }
    Spring {
        id: heightSpring
        target: root.targetSize.height
        response: 0.42
        dampingFraction: 0.78
    }
    Spring {
        id: radiusSpring
        target: root.mode === "expanded" ? Metrics.expandedRadius : root.targetSize.height / 2
        response: 0.42
        dampingFraction: 0.9
    }

    Rectangle {
        id: pill

        width: widthSpring.value
        height: heightSpring.value
        radius: Math.min(radiusSpring.value, height / 2)
        x: Math.round((root.width - width) / 2)
        y: Metrics.topMargin
        color: "black"
        clip: true
        antialiasing: true

        ClockFace {
            anchors.fill: parent
            mode: root.mode
        }

        HoverHandler {
            id: hover
            onHoveredChanged: {
                if (root.demo)
                    return
                if (hovered && root.mode === "compact")
                    root.mode = "peek"
                collapseTimer.restart()
            }
        }

        TapHandler {
            onTapped: {
                if (root.demo)
                    return
                root.mode = root.mode === "expanded" ? "peek" : "expanded"
            }
        }
    }

    // Leaving the pill collapses it; expanded waits a bit longer so a
    // pointer that slips off the edge does not slam it shut.
    Timer {
        id: collapseTimer
        interval: root.mode === "expanded" ? 700 : 150
        onTriggered: {
            if (!hover.hovered && !root.demo)
                root.mode = "compact"
        }
    }

    Timer {
        running: root.demo
        repeat: true
        interval: 1500
        property int step: 0
        onTriggered: {
            step = (step + 1) % 4
            root.mode = ["compact", "peek", "expanded", "peek"][step]
        }
    }
}
