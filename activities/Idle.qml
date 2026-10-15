import QtQuick
import IslandShell

// Resting state: workspaces and clock. Always active, lowest priority.
Activity {
    name: "idle"
    ambient: true
    priority: 0
    scope: Activity.AllScreens

    compactLeading: Component {
        WorkspaceDots {}
    }

    compactTrailing: Component {
        ClockText {}
    }

    expanded: Component {
        IdleExpanded {}
    }
}
