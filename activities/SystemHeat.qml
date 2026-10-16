pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// A temperature over its [system] threshold for 10 s. Not ambient: it shows
// over fullscreen games, which is when it matters.
Activity {
    id: root

    readonly property string sensorName: SystemStats.hotSensor === "gpu" ? "GPU"
                                       : SystemStats.hotSensor === "nvme" ? "SSD" : "CPU"

    name: "system-heat"
    priority: 85
    scope: Activity.AllScreens
    active: SystemStats.hot

    property Connections _heat: Connections {
        target: SystemStats
        function onHotChanged() {
            if (SystemStats.hot)
                root.alert(5000)
        }
    }

    compactLeading: Component {
        Row {
            spacing: 8
            IconImage {
                anchors.verticalCenter: parent.verticalCenter
                source: Icons.source("temperature-warm")
                size: 16
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.sensorName + " " + Math.round(SystemStats.hotTemp) + "°C"
                color: Theme.danger
                font.pixelSize: 13
                font.weight: Font.DemiBold
                font.features: { "tnum": 1 }
            }
        }
    }
    compactTrailing: Component { ClockText {} }
    minimal: Component {
        IconImage {
            source: Icons.source("temperature-warm")
            size: 16
        }
    }
    expanded: Component {
        Item {
            implicitWidth: 400
            implicitHeight: 110
            Column {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 12
                Text {
                    text: qsTr("%1 is running hot: %2 °C").arg(root.sensorName).arg(Math.round(SystemStats.hotTemp))
                    color: Theme.danger
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                }
                SystemStatsRow {}
            }
        }
    }
}
