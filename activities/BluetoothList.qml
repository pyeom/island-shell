pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Bluetooth devices (with battery when reported); discovers while open.
Column {
    id: root

    spacing: 2
    Component.onCompleted: Bluetooth.setDiscovering(true)
    Component.onDestruction: Bluetooth.setDiscovering(false)

    Text {
        visible: Bluetooth.devices.length === 0
        text: Bluetooth.powered ? qsTr("Looking for devices…") : qsTr("Bluetooth is off")
        color: Theme.secondaryForeground
        font.pixelSize: 12
        topPadding: 6
        bottomPadding: 6
    }

    Repeater {
        model: Bluetooth.devices.slice(0, 8)
        delegate: Rectangle {
            id: entry
            required property var modelData
            width: root.width
            height: 34
            radius: 10
            color: area.containsMouse ? Qt.rgba(1, 1, 1, 0.1) : "transparent"

            IconImage {
                id: kind
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                source: Icons.source(entry.modelData.icon, "network-bluetooth")
                size: 16
            }
            Text {
                anchors.left: kind.right
                anchors.leftMargin: 10
                anchors.right: statusText.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                text: entry.modelData.name
                elide: Text.ElideRight
                color: Theme.foreground
                font.pixelSize: 12
                font.weight: entry.modelData.connected ? Font.DemiBold : Font.Normal
            }
            Text {
                id: statusText
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                text: (entry.modelData.battery >= 0 ? entry.modelData.battery + "%  " : "")
                      + (entry.modelData.connected ? qsTr("Connected")
                         : entry.modelData.paired ? qsTr("Paired") : qsTr("New"))
                color: entry.modelData.battery >= 0 && entry.modelData.battery < 15 ? Theme.danger
                     : Theme.secondaryForeground
                font.pixelSize: 11
            }
            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: entry.modelData.connected ? Bluetooth.disconnectDevice(entry.modelData.path)
                                                     : Bluetooth.connectDevice(entry.modelData.path)
            }
        }
    }
}
