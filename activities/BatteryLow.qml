pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// A battery under 15%: the laptop's (while discharging) or a peripheral's.
Activity {
    id: root

    readonly property var low: Batteries.low
    readonly property bool has: low.percentage !== undefined
    property string _alerted: ""

    name: "battery-low"
    priority: 82
    scope: Activity.AllScreens
    active: has

    onLowChanged: {
        if (!has)
            return
        const key = low.model
        if (key !== _alerted) {
            _alerted = key
            alert(5000)
        }
    }

    compactLeading: Component {
        Row {
            spacing: 8
            IconImage {
                anchors.verticalCenter: parent.verticalCenter
                source: Icons.source(root.low.icon ?? "battery-caution", "battery-caution")
                size: 16
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("%1 · %2%").arg(root.low.model ?? "").arg(root.low.percentage ?? 0)
                color: Theme.danger
                font.pixelSize: 12
            }
        }
    }
    compactTrailing: Component { ClockText {} }
    minimal: Component {
        IconImage {
            source: Icons.source("battery-caution")
            size: 16
        }
    }
}
