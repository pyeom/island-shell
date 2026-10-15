pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// One live activity from LiveActivities (D-Bus or `islandctl push`).
Activity {
    id: root

    required property string liveId
    required property string title
    required property string subtitle
    required property string icon
    required property real progress
    required property string state
    required property var actions
    required property int alertSerial

    readonly property color stateColor: state === "success" ? "#30d158"
                                      : state === "error" ? Theme.danger
                                      : Theme.accent
    readonly property bool hasProgress: progress >= 0

    name: "live:" + liveId
    priority: 50

    onAlertSerialChanged: alert(3000)
    Component.onCompleted: alert(3000)

    component Glyph: Item {
        property int size: 20
        width: size
        height: size
        ProgressRing {
            anchors.centerIn: parent
            visible: root.state === "running" && (root.hasProgress || !root.icon)
            size: parent.size
            progress: root.hasProgress ? root.progress : -1
            color: root.stateColor
        }
        IconImage {
            anchors.centerIn: parent
            visible: root.state !== "running" || (!root.hasProgress && !!root.icon)
            source: root.state === "success" ? Icons.source("emblem-ok-symbolic")
                  : root.state === "error" ? Icons.source("dialog-error-symbolic")
                  : root.icon
            size: parent.size - 2
        }
    }

    compactLeading: Component { Glyph {} }

    compactTrailing: Component {
        Text {
            width: Math.min(implicitWidth, 200)
            text: root.hasProgress && root.state === "running" ? Math.round(root.progress * 100) + "%"
                : root.subtitle || root.title
            elide: Text.ElideRight
            color: root.state === "running" ? Theme.foreground : root.stateColor
            font.pixelSize: 13
            font.features: { "tnum": 1 }
        }
    }

    minimal: Component { Glyph { size: 18 } }

    expanded: Component {
        Item {
            implicitWidth: 400
            implicitHeight: column.implicitHeight + 36

            Column {
                id: column
                anchors.fill: parent
                anchors.margins: 18
                spacing: 10

                Row {
                    width: parent.width
                    spacing: 12
                    Glyph { size: 32; anchors.verticalCenter: parent.verticalCenter }
                    Column {
                        width: parent.width - 44
                        anchors.verticalCenter: parent.verticalCenter
                        Text {
                            width: parent.width
                            text: root.title || root.liveId
                            elide: Text.ElideRight
                            color: Theme.foreground
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                        }
                        Text {
                            width: parent.width
                            visible: text !== ""
                            text: root.subtitle
                            elide: Text.ElideRight
                            color: Theme.secondaryForeground
                            font.pixelSize: 12
                        }
                    }
                }

                Rectangle {
                    visible: root.hasProgress
                    width: parent.width
                    height: 6
                    radius: 3
                    color: Qt.rgba(1, 1, 1, 0.2)
                    Rectangle {
                        width: parent.width * root.progress
                        height: parent.height
                        radius: parent.radius
                        color: root.stateColor
                        Behavior on width { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }
                    }
                }

                Row {
                    visible: root.actions.length > 0
                    spacing: 8
                    Repeater {
                        model: root.actions
                        delegate: IconButton {
                            required property var modelData
                            height: 30
                            text: modelData.label
                            onClicked: LiveActivities.invoke(root.liveId, modelData.key)
                        }
                    }
                }
            }
        }
    }
}
