pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Rain likely within 2 hours ([weather] rain_alert). Alerts once per rainy
// hour; then sits in the pill until that hour passes.
Activity {
    id: root

    readonly property var rain: Weather.rainSoon
    readonly property bool likely: rain.probability !== undefined
    property string _alertedFor: ""

    name: "rain"
    priority: 45
    ambient: true
    active: likely

    onRainChanged: {
        if (!likely)
            return
        const key = String(rain.time)
        if (key !== _alertedFor) {
            _alertedFor = key
            alert(4000)
        }
    }

    readonly property string label: qsTr("Rain %1% · %2").arg(rain.probability ?? 0)
                                    .arg(likely ? Qt.formatTime(rain.time, Config.clockFormat) : "")

    compactLeading: Component {
        Row {
            spacing: 8
            IconImage {
                anchors.verticalCenter: parent.verticalCenter
                source: Icons.source("weather-showers")
                size: 18
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.label
                color: "#64d2ff"
                font.pixelSize: 12
            }
        }
    }
    compactTrailing: Component { ClockText {} }
    minimal: Component {
        IconImage {
            source: Icons.source("weather-showers")
            size: 18
        }
    }
    expanded: Component {
        Item {
            implicitWidth: 380
            implicitHeight: 120
            WeatherLine {
                anchors.fill: parent
                anchors.margins: 18
            }
        }
    }
}
