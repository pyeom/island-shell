pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Now playing (MPRIS). Stays for a few minutes after pausing.
Activity {
    id: root

    readonly property var player: Mpris.current
    readonly property bool playing: !!player && player.playing
    property bool recentlyPaused: false

    name: "media"
    priority: 40
    active: !!player && (playing || recentlyPaused)

    onPlayingChanged: {
        recentlyPaused = !playing && !!player
        if (recentlyPaused)
            _pauseTimer.restart()
    }

    property Timer _pauseTimer: Timer {
        interval: 5 * 60 * 1000
        onTriggered: root.recentlyPaused = false
    }

    component Art: Rectangle {
        property int size: 22
        width: size
        height: size
        radius: size / 4
        color: Qt.rgba(1, 1, 1, 0.1)
        clip: true
        Image {
            anchors.fill: parent
            visible: status === Image.Ready
            source: root.player?.artUrl ?? ""
            sourceSize: Qt.size(parent.size * 2, parent.size * 2)
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
        }
        IconImage {
            anchors.centerIn: parent
            visible: !(root.player?.artUrl)
            source: Icons.source(root.player?.desktopEntry ?? "", "audio-x-generic-symbolic")
            size: parent.size * 0.7
        }
    }

    // Art and bars on the left so the clock stays visible while music plays.
    compactLeading: Component {
        Row {
            spacing: 8
            Art { anchors.verticalCenter: parent.verticalCenter }
            Bars {
                anchors.verticalCenter: parent.verticalCenter
                playing: root.playing
            }
        }
    }

    compactTrailing: Component { ClockText {} }

    minimal: Component { Art { size: 20 } }

    expanded: Component {
        MediaPanel { player: root.player }
    }
}
