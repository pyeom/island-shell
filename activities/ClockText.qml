import QtQuick
import IslandShell

// The compact clock, shared by idle and activities that keep it visible;
// the laptop battery (if any) sits next to it.
Row {
    spacing: 10

    BatteryText {
        anchors.verticalCenter: parent.verticalCenter
    }
    Text {
        anchors.verticalCenter: parent.verticalCenter
        text: Qt.formatTime(Clock.formatHasSeconds ? Clock.now : Clock.minute, Config.clockFormat)
        color: Theme.foreground
        font.pixelSize: 14
        font.weight: Font.DemiBold
        font.features: { "tnum": 1 }
    }
}
