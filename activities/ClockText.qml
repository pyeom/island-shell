import QtQuick
import IslandShell

// The compact clock, shared by idle and activities that keep it visible.
Text {
    text: Qt.formatTime(Clock.now, Config.clockFormat)
    color: Theme.foreground
    font.pixelSize: 14
    font.weight: Font.DemiBold
    font.features: { "tnum": 1 }
}
