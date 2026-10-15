pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// App launcher. `islandctl launcher toggle` opens it on the focused monitor.
Activity {
    id: root

    property bool open: false

    name: "launcher"
    priority: 1000
    modal: true
    active: open

    onDismissRequested: open = false

    property Connections _control: Connections {
        target: ShellControl
        function onLauncherToggleRequested() {
            root.open = !root.open
        }
    }

    expanded: Component {
        LauncherPanel {
            onClose: root.open = false
        }
    }
}
