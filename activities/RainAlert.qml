pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Rain likely within 2 hours ([weather] rain_alert). Alerts once when rain
// becomes likely, stays in the pill for a minute, then leaves it; the forecast
// is still in the expanded clock. Alerts again only after a dry spell.
Activity {
    id: root

    readonly property var rain: Weather.rainSoon
    readonly property bool likely: rain.probability !== undefined
    property bool _showing: false

    name: "rain"
    priority: 45
    ambient: true
    active: likely && _showing

    onLikelyChanged: {
        if (!likely) {
            _showing = false
            return
        }
        _showing = true
        _hide.restart()
        alert(4000)
    }
    Component.onCompleted: if (likely) likelyChanged()

    property Timer _hide: Timer {
        interval: 60 * 1000
        onTriggered: root._showing = false
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
