pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Surfaces config.toml problems and user activities that failed to load or
// threw at runtime.
Activity {
    id: root

    name: "errors"
    priority: 90
    active: count > 0

    readonly property var items: Config.errors.map(m => ({ file: "config.toml", message: m }))
                                 .concat(UserActivities.errors)
                                 .concat(Notifications.available || !Notifications.unavailableReason ? []
                                         : [{ file: qsTr("notifications"),
                                              message: Notifications.unavailableReason }])
    readonly property int count: items.length

    component Badge: Rectangle {
        width: 20
        height: 20
        radius: 10
        color: Theme.danger
        Text {
            anchors.centerIn: parent
            text: "!"
            color: "white"
            font.pixelSize: 13
            font.weight: Font.Bold
        }
    }

    compactLeading: Component { Badge {} }

    compactTrailing: Component {
        Text {
            text: qsTr("%n error(s)", "", root.count)
            color: Theme.foreground
            font.pixelSize: 13
        }
    }

    minimal: Component { Badge {} }

    expanded: Component {
        Item {
            implicitWidth: Config.expandedWidth
            implicitHeight: Math.min(column.implicitHeight + 36, Config.surfaceHeight - Config.topMargin)

            Column {
                id: column
                anchors.fill: parent
                anchors.margins: 18
                spacing: 8

                Text {
                    text: qsTr("Shell errors")
                    color: Theme.foreground
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                }

                Repeater {
                    model: root.items
                    delegate: Text {
                        required property var modelData
                        width: column.width
                        text: "<b>" + modelData.file + "</b>  " + modelData.message
                        textFormat: Text.StyledText
                        wrapMode: Text.Wrap
                        maximumLineCount: 3
                        elide: Text.ElideRight
                        color: Theme.secondaryForeground
                        font.pixelSize: 12
                    }
                }

                Text {
                    text: qsTr("Fix the file and save, or run: islandctl reload")
                    color: Theme.secondaryForeground
                    font.pixelSize: 11
                }
            }
        }
    }

    onCountChanged: if (count > 0) alert(5000)
}
