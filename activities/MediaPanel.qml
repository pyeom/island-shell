pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

Item {
    id: panel

    required property var player

    function format(seconds: real): string {
        const s = Math.max(0, Math.floor(seconds))
        const m = Math.floor(s / 60)
        return m + ":" + String(s % 60).padStart(2, "0")
    }

    implicitWidth: 420
    implicitHeight: 176

    Row {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 16

        Rectangle {
            width: 96
            height: 96
            radius: 18
            color: Qt.rgba(1, 1, 1, 0.08)
            clip: true

            RoundedImage {
                id: art
                radius: parent.radius
                anchors.fill: parent
                source: panel.player?.artUrl ?? ""
                sourceSize: Qt.size(192, 192)
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
            }
            IconImage {
                anchors.centerIn: parent
                visible: art.status !== Image.Ready
                source: Icons.source(panel.player?.desktopEntry ?? "", "audio-x-generic-symbolic")
                size: 44
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: panel.player?.raise()
            }
        }

        Column {
            width: parent.width - 112
            spacing: 4

            Text {
                width: parent.width
                text: panel.player?.title || qsTr("Nothing playing")
                elide: Text.ElideRight
                color: Theme.foreground
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }
            Text {
                width: parent.width
                text: panel.player?.artist || panel.player?.identity || ""
                elide: Text.ElideRight
                color: Theme.secondaryForeground
                font.pixelSize: 13
            }

            Item { width: 1; height: 6 }

            // Progress; click to seek when the player allows it.
            Item {
                width: parent.width
                height: 18
                visible: (panel.player?.length ?? 0) > 0

                Rectangle {
                    id: track
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width
                    height: 4
                    radius: 2
                    color: Qt.rgba(1, 1, 1, 0.2)

                    Rectangle {
                        width: parent.width * Math.min(1, panel.player.position / panel.player.length)
                        height: parent.height
                        radius: parent.radius
                        color: Theme.foreground
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: panel.player?.canSeek ?? false
                    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: mouse => panel.player.seekTo(panel.player.length * mouse.x / width)
                }
            }

            Item {
                width: parent.width
                height: 14
                visible: (panel.player?.length ?? 0) > 0
                Text {
                    text: panel.format(panel.player?.position ?? 0)
                    color: Theme.secondaryForeground
                    font.pixelSize: 11
                    font.features: { "tnum": 1 }
                }
                Text {
                    anchors.right: parent.right
                    text: panel.format(panel.player?.length ?? 0)
                    color: Theme.secondaryForeground
                    font.pixelSize: 11
                    font.features: { "tnum": 1 }
                }
            }

            Row {
                spacing: 10
                IconButton {
                    icon: "media-skip-backward-symbolic"
                    enabled: panel.player?.canGoPrevious ?? false
                    opacity: enabled ? 1 : 0.4
                    onClicked: panel.player.previous()
                }
                IconButton {
                    width: 44
                    height: 44
                    iconSize: 22
                    icon: panel.player?.playing ? "media-playback-pause-symbolic"
                                                : "media-playback-start-symbolic"
                    onClicked: panel.player.playPause()
                }
                IconButton {
                    icon: "media-skip-forward-symbolic"
                    enabled: panel.player?.canGoNext ?? false
                    opacity: enabled ? 1 : 0.4
                    onClicked: panel.player.next()
                }
            }
        }
    }

    // Switch between players when more than one is around.
    Row {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 12
        spacing: 4
        visible: Mpris.players.length > 1

        Repeater {
            model: Mpris.players
            delegate: IconButton {
                required property var modelData
                height: 22
                text: modelData.identity || modelData.service.replace("org.mpris.MediaPlayer2.", "")
                highlighted: modelData === panel.player
                onClicked: Mpris.setCurrent(modelData)
            }
        }
    }
}
