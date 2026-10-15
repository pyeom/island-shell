pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Power menu. `islandctl power` opens it on the focused monitor.
Activity {
    id: root

    property bool open: false

    name: "power"
    priority: 1000
    modal: true
    active: open

    onDismissRequested: open = false

    property Connections _control: Connections {
        target: ShellControl
        function onPowerRequested() {
            root.open = !root.open
        }
    }

    expanded: Component {
        PowerPanel {
            onClose: root.open = false
        }
    }
}
