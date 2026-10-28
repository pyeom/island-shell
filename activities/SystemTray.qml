pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// System tray: apps' status icons and their menus. `islandctl tray` or the
// tile in quick settings.
Activity {
    id: root

    property bool open: false
    // Item whose menu to open with the panel (Tray.showMenu).
    property string menuKey: ""

    name: "tray"
    priority: 1000
    modal: true
    active: open

    onDismissRequested: open = false
    onOpenChanged: {
        if (!open)
            menuKey = ""
    }

    property Connections _control: Connections {
        target: ShellControl
        function onTrayRequested() {
            root.open = !root.open
        }
    }
    property Connections _tray: Connections {
        target: Tray
        function onOpenRequested(key: string) {
            root.menuKey = key
            root.open = true
        }
    }

    expanded: Component {
        SystemTrayPanel {
            initialMenu: root.menuKey
            onClose: root.open = false
        }
    }
}
