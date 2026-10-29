pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// The tray in the bubble beside the pill, below idle so the clock stays:
// the apps with unread messages (or NeedsAttention), else a tray glyph.
// Tapped, the tray panel. An app that starts wanting attention, or gets
// more unread messages, expands the island for a moment (`[tray] alerts`).
Activity {
    id: root

    name: "tray:apps"
    priority: -1
    ambient: true
    scope: Activity.AllScreens
    active: Tray.available && Tray.count > 0

    // False for the first seconds of the island: what apps report while
    // the session starts is not news.
    property bool settled: true

    // Shown items that want attention, in tray order.
    property var unread: []
    function refresh() {
        const list = []
        for (let i = 0; i < watchers.count; ++i) {
            const w = watchers.objectAt(i)
            if (w && w.shown && w.attention)
                list.push(w)
        }
        unread = list
    }

    // The app the current alert is about, while it shows.
    property Watcher alertItem: null
    property bool alerting: false
    property Timer alertEnd: Timer {
        interval: 3500 // past the alert's collapse animation
        onTriggered: root.alerting = false
    }
    function notify(item: Watcher) {
        if (!settled || !Config.trayAlerts)
            return
        alertItem = item
        alerting = true
        alertEnd.restart()
        alert(3000)
    }

    component Watcher: QtObject {
        id: watcher
        required property string key
        required property string title
        required property string tooltip
        required property string icon
        required property int badge
        required property bool attention
        required property bool shown
        required property bool hasMenu

        readonly property bool wants: shown && attention
        readonly property string subtitle: tooltip !== "" ? tooltip
                                         : badge > 0 ? qsTr("%n unread", "", badge)
                                         : qsTr("Wants attention")
        property int seenBadge: 0

        onWantsChanged: {
            root.refresh()
            if (wants)
                root.notify(watcher)
        }
        onBadgeChanged: {
            if (badge > seenBadge && wants)
                root.notify(watcher)
            seenBadge = badge
        }
        Component.onCompleted: seenBadge = badge
    }

    property Instantiator watchers: Instantiator {
        model: Tray
        delegate: Watcher {}
        onObjectAdded: root.refresh()
        onObjectRemoved: root.refresh()
    }

    // Up to three app icons, overlapping; a glyph when nothing is unread.
    minimal: Component {
        Item {
            readonly property int shownCount: Math.min(root.unread.length, 3)
            implicitWidth: shownCount > 0 ? 18 + (shownCount - 1) * 12 : 18
            implicitHeight: 18

            IconImage {
                anchors.centerIn: parent
                visible: parent.shownCount === 0
                source: Icons.source("view-more-horizontal-symbolic")
                size: 16
            }
            Repeater {
                model: root.unread.slice(0, 3)
                delegate: Rectangle {
                    id: chip
                    required property Watcher modelData
                    required property int index
                    x: index * 12
                    z: 3 - index
                    width: 20
                    height: 20
                    y: -1
                    radius: 6
                    color: Theme.background
                    IconImage {
                        anchors.centerIn: parent
                        source: chip.modelData.icon
                        size: 16
                    }
                }
            }
        }
    }

    expanded: Component {
        Loader {
            // Chosen once: a later alert does not swap out the open panel.
            Component.onCompleted: sourceComponent = root.alerting && root.alertItem ? card : panel
            Component {
                id: panel
                SystemTrayPanel {
                    onClose: ShellControl.Collapse()
                }
            }
            Component {
                id: card
                AlertCard {
                    item: root.alertItem
                }
            }
        }
    }

    component AlertCard: Item {
        id: alertCard
        required property Watcher item

        implicitWidth: 380
        implicitHeight: 72

        Row {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 20
            anchors.rightMargin: 16
            spacing: 14

            Item {
                anchors.verticalCenter: parent.verticalCenter
                width: 32
                height: 32
                Rectangle {
                    anchors.centerIn: parent
                    width: 38
                    height: width
                    radius: width / 3
                    color: "transparent"
                    border.width: 1.5
                    border.color: Theme.accent
                }
                IconImage {
                    anchors.centerIn: parent
                    source: alertCard.item ? alertCard.item.icon : ""
                    size: 32
                }
                Rectangle {
                    visible: (alertCard.item?.badge ?? 0) > 0
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.rightMargin: -7
                    anchors.topMargin: -5
                    width: Math.max(height, badgeText.implicitWidth + 8)
                    height: 16
                    radius: 8
                    color: Theme.danger
                    border.width: 2
                    border.color: Theme.background
                    Text {
                        id: badgeText
                        anchors.centerIn: parent
                        text: (alertCard.item?.badge ?? 0) > 99 ? "99+" : alertCard.item?.badge ?? ""
                        color: "white"
                        font.pixelSize: 9
                        font.weight: Font.Bold
                    }
                }
            }
            Column {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - 32 - buttons.width - 2 * parent.spacing
                Text {
                    width: parent.width
                    text: alertCard.item?.title ?? ""
                    elide: Text.ElideRight
                    color: Theme.foreground
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                }
                Text {
                    width: parent.width
                    text: alertCard.item?.subtitle ?? ""
                    elide: Text.ElideRight
                    color: Theme.secondaryForeground
                    font.pixelSize: 12
                }
            }
            Row {
                id: buttons
                anchors.verticalCenter: parent.verticalCenter
                spacing: 6
                IconButton {
                    height: 30
                    text: qsTr("Open")
                    onClicked: {
                        if (alertCard.item)
                            Tray.activate(alertCard.item.key)
                        ShellControl.Collapse()
                    }
                }
                IconButton {
                    visible: alertCard.item?.hasMenu ?? false
                    width: 30
                    height: 30
                    icon: "view-more-horizontal-symbolic"
                    iconSize: 14
                    onClicked: Tray.showMenu(alertCard.item.key)
                }
            }
        }
    }
}
