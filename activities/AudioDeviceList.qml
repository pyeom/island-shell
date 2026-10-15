pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Sinks or sources to pick the default from.
Column {
    id: root

    property var devices: []
    property string icon: "audio-speakers-symbolic"

    spacing: 2

    Repeater {
        model: root.devices
        delegate: Rectangle {
            id: row
            required property var modelData

            width: root.width
            height: 34
            radius: 10
            color: area.containsMouse ? Qt.rgba(1, 1, 1, 0.1) : "transparent"

            IconImage {
                id: rowIcon
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                source: Icons.source(row.modelData.description.indexOf("HDMI") >= 0
                                     ? "video-television" : root.icon, root.icon)
                size: 16
            }
            Text {
                anchors.left: rowIcon.right
                anchors.leftMargin: 10
                anchors.right: check.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                text: row.modelData.description
                elide: Text.ElideRight
                color: Theme.foreground
                font.pixelSize: 12
                font.weight: row.modelData.isDefault ? Font.DemiBold : Font.Normal
            }
            IconImage {
                id: check
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                visible: row.modelData.isDefault
                source: Icons.source("object-select-symbolic")
                size: 14
            }
            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: Audio.setDefault(row.modelData.id)
            }
        }
    }
}
