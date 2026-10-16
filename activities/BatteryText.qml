import QtQuick
import IslandShell

// Laptop battery next to the clock; hidden on machines without one.
Row {
    visible: Batteries.hasLaptopBattery
    spacing: 4

    IconImage {
        anchors.verticalCenter: parent.verticalCenter
        source: Icons.source(Batteries.charging ? "battery-good-charging-symbolic"
                             : Batteries.laptopPercentage < 15 ? "battery-caution-symbolic"
                             : Batteries.laptopPercentage < 50 ? "battery-low-symbolic"
                             : "battery-good-symbolic", "battery")
        size: 14
    }
    Text {
        anchors.verticalCenter: parent.verticalCenter
        text: Batteries.laptopPercentage + "%"
        color: Batteries.laptopPercentage < 15 && !Batteries.charging ? Theme.danger : Theme.foreground
        font.pixelSize: 12
        font.features: { "tnum": 1 }
    }
}
