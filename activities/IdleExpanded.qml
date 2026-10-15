import QtQuick
import IslandShell

Item {
    implicitWidth: Config.expandedWidth
    implicitHeight: Config.expandedHeight

    Column {
        anchors.centerIn: parent
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
    }
}
