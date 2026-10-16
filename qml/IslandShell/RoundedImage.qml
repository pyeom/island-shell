import QtQuick
import QtQuick.Effects

// An Image with rounded corners. `clip: true` on a rounded parent only clips
// to its bounding rectangle, so album art and thumbnails need a mask.
Image {
    id: image

    property real radius: 0

    layer.enabled: radius > 0
    layer.effect: MultiEffect {
        maskEnabled: true
        maskSource: mask
        maskThresholdMin: 0.5
        maskSpreadAtMin: 1.0
    }

    Rectangle {
        id: mask
        width: image.width
        height: image.height
        radius: image.radius
        visible: false
        layer.enabled: true
    }
}
