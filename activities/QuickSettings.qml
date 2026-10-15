pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Quick settings. Right click on the island or `islandctl quick`.
Activity {
    id: root

    property bool open: false

    name: "quicksettings"
    priority: 1000
    modal: true
    active: open

    onDismissRequested: open = false

    property Connections _control: Connections {
        target: ShellControl
        function onQuickSettingsRequested() {
            root.open = !root.open
        }
    }

    expanded: Component {
        QuickSettingsPanel {
            onClose: root.open = false
        }
    }
}
