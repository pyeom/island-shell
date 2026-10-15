import QtQuick

// Round button with an icon or a short label.
Rectangle {
    id: button

    property string icon: ""
    property string text: ""
    property int iconSize: 18
    property bool highlighted: false
    readonly property alias pressed: area.pressed
    readonly property alias hovered: area.containsMouse

    signal clicked()

    implicitWidth: text ? label.implicitWidth + 24 : 36
    implicitHeight: 36
    radius: height / 2
    color: highlighted ? Theme.accent
         : area.pressed ? Qt.rgba(1, 1, 1, 0.22)
         : area.containsMouse ? Qt.rgba(1, 1, 1, 0.16)
         : Qt.rgba(1, 1, 1, 0.1)

    Behavior on color { ColorAnimation { duration: 120 } }

    IconImage {
        anchors.centerIn: parent
        visible: button.icon !== ""
        source: button.icon ? Icons.source(button.icon) : ""
        size: button.iconSize
    }

    Text {
        id: label
        anchors.centerIn: parent
        visible: button.text !== ""
        text: button.text
        color: Theme.foreground
        font.pixelSize: 13
        font.weight: Font.Medium
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: button.clicked()
    }
}
