import QtQuick
import QtQuick.Shapes

// Circular progress, 0..1. Negative progress spins as indeterminate.
Item {
    id: ring

    property real progress: 0
    property color color: Theme.accent
    property color trackColor: Qt.rgba(1, 1, 1, 0.18)
    property real lineWidth: 3
    property int size: 20

    readonly property bool indeterminate: progress < 0

    width: size
    height: size

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        RotationAnimation on rotation {
            running: ring.indeterminate && ring.visible
            from: 0
            to: 360
            duration: 1000
            loops: Animation.Infinite
        }

        ShapePath {
            fillColor: "transparent"
            strokeColor: ring.trackColor
            strokeWidth: ring.lineWidth
            PathAngleArc {
                centerX: ring.size / 2
                centerY: ring.size / 2
                radiusX: (ring.size - ring.lineWidth) / 2
                radiusY: radiusX
                startAngle: 0
                sweepAngle: 360
            }
        }
        ShapePath {
            fillColor: "transparent"
            strokeColor: ring.color
            strokeWidth: ring.lineWidth
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: ring.size / 2
                centerY: ring.size / 2
                radiusX: (ring.size - ring.lineWidth) / 2
                radiusY: radiusX
                startAngle: -90
                sweepAngle: ring.indeterminate ? 90 : 360 * Math.max(0, Math.min(1, ring.progress))
                Behavior on sweepAngle { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }
            }
        }
    }
}
