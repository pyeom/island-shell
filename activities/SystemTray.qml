pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// System tray: apps' status icons and their menus. `islandctl tray` or the
// tile in quick settings.
Activity {
    id: root

    property bool open: false

    name: "tray"
    priority: 1000
    modal: true
    active: open

    onDismissRequested: open = false

    property Connections _control: Connections {
        target: ShellControl
        function onTrayRequested() {
            root.open = !root.open
        }
    }

    expanded: Component {
        SystemTrayPanel {
            onClose: root.open = false
        }
    }
}
