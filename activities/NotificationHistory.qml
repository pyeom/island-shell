pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// History list for the expanded idle view, grouped by app.
Column {
    id: root

    property int maxHeight: 220

    spacing: 6
    visible: Notifications.history.count > 0

    Item {
        width: root.width
        height: 24

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Notifications")
            color: Theme.foreground
            font.pixelSize: 13
            font.weight: Font.DemiBold
        }
        IconButton {
            anchors.right: parent.right
            height: 24
            text: qsTr("Clear")
            onClicked: Notifications.clearHistory()
        }
    }

    ListView {
        id: list
        width: root.width
        height: Math.min(contentHeight, root.maxHeight)
        clip: true
        spacing: 4
        model: Notifications.history
        boundsBehavior: Flickable.StopAtBounds

        section.property: "appName"
        section.delegate: Text {
            required property string section
            width: list.width
            topPadding: 6
            text: section
            color: Theme.secondaryForeground
            font.pixelSize: 11
            font.capitalization: Font.AllUppercase
        }

        delegate: Rectangle {
            id: row

            required property int id
            required property string icon
            required property string summary
            required property string body
            required property date time

            width: list.width
            height: 44
            radius: 12
            color: area.containsMouse ? Qt.rgba(1, 1, 1, 0.1) : Qt.rgba(1, 1, 1, 0.05)

            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                onClicked: Notifications.invokeAction(row.id, "default")
            }

            IconImage {
                id: rowIcon
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                source: row.icon
                size: 22
            }

            Column {
                anchors.left: rowIcon.right
                anchors.leftMargin: 10
                anchors.right: dismiss.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter

                Text {
                    width: parent.width
                    text: row.summary
                    elide: Text.ElideRight
                    color: Theme.foreground
                    font.pixelSize: 12
                    font.weight: Font.Medium
                }
                Text {
                    width: parent.width
                    visible: text !== ""
                    text: row.body.replace(/<[^>]*>/g, " ")
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    color: Theme.secondaryForeground
                    font.pixelSize: 11
                }
            }

            IconButton {
                id: dismiss
                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                width: 22
                height: 22
                icon: "window-close-symbolic"
                iconSize: 10
                opacity: area.containsMouse || hovered ? 1 : 0
                onClicked: Notifications.dismiss(row.id)
            }
        }
    }
}
