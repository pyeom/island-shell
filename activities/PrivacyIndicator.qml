pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Microphone, camera and screen sharing indicators, on every monitor.
// As the secondary bubble it is just colored dots, like iOS.
Activity {
    id: root

    readonly property color micColor: "#ff9f0a"
    readonly property color cameraColor: "#30d158"
    readonly property color screenColor: "#bf5af2"

    name: "privacy"
    priority: 60
    scope: Activity.AllScreens
    active: Privacy.any

    // Alert once when something new starts using a device.
    readonly property string signature: [Privacy.microphone, Privacy.camera, Privacy.screen].join()
    onSignatureChanged: if (Privacy.any) alert(2500)

    component Indicators: Row {
        id: indicators
        property bool withIcons: true
        spacing: 6
        Repeater {
            model: [
                { on: Privacy.microphone, color: root.micColor, icon: "audio-input-microphone-symbolic" },
                { on: Privacy.camera, color: root.cameraColor, icon: "camera-web-symbolic" },
                { on: Privacy.screen, color: root.screenColor, icon: "video-display-symbolic" }
            ].filter(i => i.on)
            delegate: Rectangle {
                id: dot
                required property var modelData
                width: indicators.withIcons ? 22 : 8
                height: width
                radius: width / 2
                color: modelData.color
                IconImage {
                    anchors.centerIn: parent
                    visible: indicators.withIcons
                    source: Icons.source(dot.modelData.icon)
                    size: 13
                }
            }
        }
    }

    compactLeading: Component { Indicators {} }
    compactTrailing: Component { ClockText {} }
    minimal: Component { Indicators { withIcons: false; spacing: 3 } }

    expanded: Component {
        Item {
            implicitWidth: 360
            implicitHeight: list.implicitHeight + 36

            Column {
                id: list
                anchors.fill: parent
                anchors.margins: 18
                spacing: 10

                component Line: Row {
                    property color dot
                    property string label
                    property string detail
                    spacing: 10
                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 10
                        height: 10
                        radius: 5
                        color: parent.dot
                    }
                    Text {
                        text: parent.label
                        color: Theme.foreground
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                    }
                    Text {
                        text: parent.detail
                        color: Theme.secondaryForeground
                        font.pixelSize: 13
                    }
                }

                Line {
                    visible: Privacy.microphone
                    dot: root.micColor
                    label: qsTr("Microphone")
                    detail: Privacy.microphoneApps.join(", ")
                }
                Line {
                    visible: Privacy.camera
                    dot: root.cameraColor
                    label: qsTr("Camera")
                    detail: Privacy.cameraApps.join(", ")
                }
                Line {
                    visible: Privacy.screen
                    dot: root.screenColor
                    label: qsTr("Screen")
                    detail: qsTr("being shared")
                }
            }
        }
    }
}
