import QtQuick
import IslandShell

// Resting state: workspaces and clock. Always active, lowest priority.
Activity {
    name: "idle"
    priority: 0
    scope: Activity.AllScreens

    compactLeading: Component {
        WorkspaceDots {}
    }

    compactTrailing: Component {
        Text {
            text: Qt.formatTime(Clock.now, Config.clockFormat)
            color: Theme.foreground
            font.pixelSize: 14
            font.weight: Font.DemiBold
            font.features: { "tnum": 1 }
        }
    }

    expanded: Component {
        IdleExpanded {}
    }
}
