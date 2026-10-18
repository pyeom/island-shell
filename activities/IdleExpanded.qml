import QtQuick
import IslandShell

// The dashboard: clock, workspaces, the month and upcoming events, what is
// playing; notification history below.
Item {
    id: root

    readonly property Activity media: ActivityRegistry.find("media")

    implicitWidth: 540
    implicitHeight: Math.max(Config.expandedHeight, column.implicitHeight + 36)

    Column {
        id: column
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 18
        width: parent.width - 36
        spacing: 14

        Row {
            width: parent.width
            spacing: 20

            Column {
                width: parent.width - month.width - 20
                spacing: 4

                Text {
                    text: Qt.formatTime(Clock.formatHasSeconds ? Clock.now : Clock.minute, Config.clockFormat)
                    color: Theme.foreground
                    font.pixelSize: 48
                    font.weight: Font.Bold
                    font.features: { "tnum": 1 }
                }
                Text {
                    text: Clock.minute.toLocaleDateString(Qt.locale(), "dddd d MMMM yyyy")
                    color: Theme.secondaryForeground
                    font.pixelSize: 14
                }
                Item { width: 1; height: 6; visible: Weather.available }
                WeatherLine {
                }
                Item { width: 1; height: 8 }
                WorkspaceDots {
                    interactive: true
                    dotSize: 10
                }
                Item { width: 1; height: 10 }
                EventList {
                    width: parent.width
                    readonly property bool isToday: month.selected.toDateString() === Clock.minute.toDateString()
                    // Calendar.revision: re-evaluate when calendars reload.
                    events: Calendar.revision < 0 ? []
                          : isToday ? Calendar.upcoming(3)
                          : Calendar.eventsOn(month.selected).slice(0, 3)
                    emptyText: isToday ? qsTr("No upcoming events") : qsTr("No events")
                }
            }

            CalendarMonth {
                id: month
            }
        }

        // Who is using the microphone, camera or screen.
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: Privacy.any
            spacing: 16
            Repeater {
                model: [
                    { on: Privacy.microphone, color: "#ff9f0a", label: qsTr("Microphone"), detail: Privacy.microphoneApps.join(", ") },
                    { on: Privacy.camera, color: "#30d158", label: qsTr("Camera"), detail: Privacy.cameraApps.join(", ") },
                    { on: Privacy.screen, color: "#bf5af2", label: qsTr("Screen"), detail: qsTr("being shared") }
                ].filter(i => i.on)
                delegate: Row {
                    id: use
                    required property var modelData
                    spacing: 6
                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 8
                        height: 8
                        radius: 4
                        color: use.modelData.color
                    }
                    Text {
                        text: use.modelData.label
                        color: Theme.foreground
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                    }
                    Text {
                        text: use.modelData.detail
                        color: Theme.secondaryForeground
                        font.pixelSize: 12
                    }
                }
            }
        }

        // Now playing; tap for the full player.
        Rectangle {
            id: nowPlaying
            readonly property var player: Mpris.current
            width: parent.width
            height: 56
            radius: 14
            visible: root.media?.active ?? false
            color: playingArea.containsMouse ? Qt.rgba(1, 1, 1, 0.1) : Qt.rgba(1, 1, 1, 0.06)

            MouseArea {
                id: playingArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: ShellControl.Expand("media")
            }

            Row {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 12

                Rectangle {
                    width: 40
                    height: 40
                    radius: 10
                    color: Qt.rgba(1, 1, 1, 0.08)
                    RoundedImage {
                        id: art
                        anchors.fill: parent
                        radius: parent.radius
                        source: nowPlaying.player?.artUrl ?? ""
                        sourceSize: Qt.size(80, 80)
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                    }
                    IconImage {
                        anchors.centerIn: parent
                        visible: art.status !== Image.Ready
                        source: Icons.source(nowPlaying.player?.desktopEntry ?? "", "audio-x-generic-symbolic")
                        size: 24
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - 40 - controls.width - 24
                    Text {
                        width: parent.width
                        text: nowPlaying.player?.title || qsTr("Nothing playing")
                        color: Theme.foreground
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        text: nowPlaying.player?.artist || nowPlaying.player?.identity || ""
                        color: Theme.secondaryForeground
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                }

                Row {
                    id: controls
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 4
                    IconButton {
                        icon: "media-skip-backward-symbolic"
                        enabled: nowPlaying.player?.canGoPrevious ?? false
                        opacity: enabled ? 1 : 0.4
                        onClicked: nowPlaying.player.previous()
                    }
                    IconButton {
                        icon: nowPlaying.player?.playing ? "media-playback-pause-symbolic"
                                                         : "media-playback-start-symbolic"
                        onClicked: nowPlaying.player.playPause()
                    }
                    IconButton {
                        icon: "media-skip-forward-symbolic"
                        enabled: nowPlaying.player?.canGoNext ?? false
                        opacity: enabled ? 1 : 0.4
                        onClicked: nowPlaying.player.next()
                    }
                }
            }
        }

        SystemStatsRow {
            anchors.horizontalCenter: parent.horizontalCenter
        }

        // Shortcuts, so the expanded island is useful even with nothing else
        // to show.
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 10

            IconButton {
                icon: "system-search-symbolic"
                text: qsTr("Search apps")
                onClicked: ShellControl.ToggleLauncher()
            }
            IconButton {
                icon: "preferences-system-symbolic"
                text: qsTr("Quick settings")
                onClicked: ShellControl.ShowQuickSettings()
            }
            IconButton {
                icon: ShellControl.dnd ? "notifications-disabled-symbolic" : "notifications-symbolic"
                highlighted: ShellControl.dnd
                onClicked: ShellControl.dnd = !ShellControl.dnd
            }
            IconButton {
                icon: "system-shutdown-symbolic"
                onClicked: ShellControl.ShowPower()
            }
        }

        NotificationHistory {
            id: history
            width: parent.width
            maxHeight: Config.surfaceHeight - Config.topMargin - (Privacy.any ? 360 : 330) - (nowPlaying.visible ? 70 : 0)
        }
    }
}
