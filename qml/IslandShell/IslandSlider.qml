import QtQuick

// Horizontal slider, 0..1 by default. `moved(value)` fires while dragging;
// bind `value` to the source of truth and write back in `onMoved`.
Item {
    id: slider

    property real value: 0
    property real from: 0
    property real to: 1
    property color fillColor: Theme.foreground
    readonly property alias pressed: area.pressed
    readonly property real position: Math.max(0, Math.min(1, (value - from) / (to - from)))

    signal moved(real value)

    implicitWidth: 200
    implicitHeight: 24

    Rectangle {
        id: track
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width
        height: 8
        radius: 4
        color: Qt.rgba(1, 1, 1, 0.18)

        Rectangle {
            width: Math.max(height, parent.width * slider.position)
            height: parent.height
            radius: parent.radius
            color: slider.fillColor
        }
    }

    MouseArea {
        id: area
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        preventStealing: true

        function emitAt(x: real) {
            const t = Math.max(0, Math.min(1, x / width))
            slider.moved(slider.from + t * (slider.to - slider.from))
        }

        onPressed: mouse => emitAt(mouse.x)
        onPositionChanged: mouse => { if (pressed) emitAt(mouse.x) }
    }

    WheelHandler {
        onWheel: event => {
            const step = (slider.to - slider.from) * 0.05 * (event.angleDelta.y > 0 ? 1 : -1)
            slider.moved(Math.max(slider.from, Math.min(slider.to, slider.value + step)))
        }
    }
}
