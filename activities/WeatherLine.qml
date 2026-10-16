pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// "18°  Nublado · Santiago" plus the next hours; hidden without [weather] city.
Column {
    id: root

    visible: Weather.available
    spacing: 6

    Row {
        spacing: 8
        IconImage {
            anchors.verticalCenter: parent.verticalCenter
            source: Icons.source(Weather.iconName, "weather-overcast")
            size: 22
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: Math.round(Weather.temperature) + Weather.unit
            color: Theme.foreground
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: Weather.description + "  ·  " + Weather.place
            color: Theme.secondaryForeground
            font.pixelSize: 12
        }
    }

    Row {
        spacing: 12
        Repeater {
            model: Weather.hours.slice(1, 6)
            delegate: Column {
                id: hour
                required property var modelData
                spacing: 1
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: Qt.formatTime(hour.modelData.time, "HH")
                    color: Theme.secondaryForeground
                    font.pixelSize: 10
                }
                IconImage {
                    anchors.horizontalCenter: parent.horizontalCenter
                    source: Icons.source(hour.modelData.icon, "weather-overcast")
                    size: 16
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: Math.round(hour.modelData.temperature) + "°"
                    color: Theme.foreground
                    font.pixelSize: 11
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: hour.modelData.precipitation >= 20
                    text: hour.modelData.precipitation + "%"
                    color: "#64d2ff"
                    font.pixelSize: 9
                }
            }
        }
    }
}
