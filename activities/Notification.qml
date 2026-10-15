pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// The current notification popup. Critical ones stay until dismissed.
Activity {
    id: root

    readonly property var current: Notifications.current
    readonly property bool showing: current.id !== undefined

    name: "notification"
    priority: showing && current.urgency >= 2 ? 100 : 70
    active: showing

    compactLeading: Component {
        IconImage {
            source: root.current.image || root.current.icon
            size: 20
        }
    }

    compactTrailing: Component {
        Text {
            width: Math.min(implicitWidth, 220)
            text: root.current.summary
            elide: Text.ElideRight
            color: Theme.foreground
            font.pixelSize: 13
        }
    }

    minimal: Component {
        IconImage {
            source: root.current.image || root.current.icon
            size: 20
        }
    }

    expanded: Component {
        NotificationCard {
            notification: root.current
            popup: true
        }
    }

    property Connections _popups: Connections {
        target: Notifications
        function onPopupShown(timeoutMs: int) {
            root.alert(timeoutMs)
        }
    }
}
