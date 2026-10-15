pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Volume and brightness changes from anywhere (keys, pactl, apps) show in
// the pill.
Activity {
    id: root

    property bool showing: false
    // "volume" | "brightness" | "mic": whichever changed last.
    property string kind: "volume"

    readonly property bool isMic: kind === "mic"
    readonly property real level: kind === "brightness" ? Brightness.level
                                : Audio.muted ? 0 : Audio.volume
    readonly property string label: isMic ? (Audio.micMuted ? qsTr("Microphone muted") : qsTr("Microphone on"))
                                   : kind === "brightness" ? Math.round(Brightness.level * 100) + "%"
                                   : Audio.muted ? qsTr("Mute")
                                   : Math.round(Audio.volume * 100) + "%"
    readonly property bool overdrive: kind === "volume" && Audio.volume > 1

    readonly property string iconName: isMic ? (Audio.micMuted ? "microphone-sensitivity-muted-symbolic"
                                                               : "audio-input-microphone-symbolic")
                                     : kind === "brightness" ? "brightness-high-symbolic"
                                     : Audio.muted || Audio.volume <= 0 ? "audio-volume-muted-symbolic"
                                     : Audio.volume < 0.34 ? "audio-volume-low-symbolic"
                                     : Audio.volume < 0.67 ? "audio-volume-medium-symbolic"
                                     : "audio-volume-high-symbolic"

    function show(what: string) {
        kind = what
        showing = true
        _hide.restart()
    }

    name: "osd"
    priority: 80
    active: showing

    property Timer _hide: Timer {
        interval: 1500
        onTriggered: root.showing = false
    }

    property Connections _audio: Connections {
        target: Audio
        function onChanged() { root.show("volume") }
    }

    property Connections _mic: Connections {
        target: Audio
        function onMicMuteChanged() { root.show("mic") }
    }

    property Connections _brightness: Connections {
        target: Brightness
        function onChanged() { root.show("brightness") }
    }

    compactLeading: Component {
        IconImage {
            source: Icons.source(root.iconName)
            size: 18
        }
    }

    compactTrailing: Component {
        Row {
            spacing: 10

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                visible: !root.isMic
                width: 120
                height: 6
                radius: 3
                color: Qt.rgba(1, 1, 1, 0.2)

                Rectangle {
                    width: parent.width * Math.min(root.level, 1)
                    height: parent.height
                    radius: parent.radius
                    color: root.overdrive ? "#ff9f0a" : Theme.foreground
                    Behavior on width { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                }
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: root.isMic ? implicitWidth : 34
                horizontalAlignment: Text.AlignRight
                color: root.isMic && Audio.micMuted ? Theme.danger : Theme.foreground
                text: root.label
                font.pixelSize: 12
                font.features: { "tnum": 1 }
            }
        }
    }
}
