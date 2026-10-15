import QtQuick
import IslandShell

Item {
    implicitWidth: Config.expandedWidth
    implicitHeight: Math.max(Config.expandedHeight, column.implicitHeight + 40)

    Column {
        id: column
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: history.visible ? 18 : (parent.height - implicitHeight) / 2
        width: parent.width - 36
        spacing: 4

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Qt.formatTime(Clock.now, Config.clockFormat)
            color: Theme.foreground
            font.pixelSize: 56
            font.weight: Font.Bold
            font.features: { "tnum": 1 }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Clock.now.toLocaleDateString(Qt.locale(), "dddd d MMMM yyyy")
            color: Theme.secondaryForeground
            font.pixelSize: 15
        }

        Item { width: 1; height: 10 }

        WorkspaceDots {
            anchors.horizontalCenter: parent.horizontalCenter
            interactive: true
            dotSize: 10
        }

        Item { width: 1; height: history.visible ? 12 : 0 }

        NotificationHistory {
            id: history
            width: parent.width
            maxHeight: Config.surfaceHeight - Config.topMargin - 220
        }
    }
}
