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
    ambient: true
    priority: 60
    scope: Activity.AllScreens
    active: Privacy.any

    // Alert when a device starts being used, at most once a minute per
    // device, so a voice call that keeps toggling capture does not keep
    // expanding the island.
    property var _lastAlert: ({})
    property var _wasOn: ({})
    function _maybeAlert(device: string, on: bool) {
        const turnedOn = on && !_wasOn[device]
        _wasOn[device] = on
        const now = Date.now()
        if (!turnedOn || now - (_lastAlert[device] ?? 0) < 60000)
            return
        _lastAlert[device] = now
        alerting = true
        alertEnd.restart()
        alert(2500)
    }
    // During the alert the island shows just who uses what; tapped, the
    // usual expanded view (clock, calendar, gauges) with that line in it.
    property bool alerting: false
    property Timer alertEnd: Timer {
        interval: 3000 // past the alert's collapse animation
        onTriggered: root.alerting = false
    }
    property Connections _privacy: Connections {
        target: Privacy
        function onChanged() {
            root._maybeAlert("microphone", Privacy.microphone)
            root._maybeAlert("camera", Privacy.camera)
            root._maybeAlert("screen", Privacy.screen)
        }
    }

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
        Loader {
            sourceComponent: root.alerting ? root.details : home
            Component {
                id: home
                IdleExpanded {}
            }
        }
    }

    property Component details: Component {
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
