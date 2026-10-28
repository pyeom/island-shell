pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// A tray app that wants attention (NeedsAttention, or unread messages it
// reports): briefly expands the island, then stays in the bubble until read.
Activity {
    id: root

    required property string key
    required property string title
    required property string tooltip
    required property string icon
    required property int badge
    required property bool attention
    required property bool shown
    required property bool hasMenu

    readonly property string subtitle: tooltip !== "" ? tooltip
                                     : badge > 0 ? qsTr("%n unread", "", badge)
                                     : qsTr("Wants attention")

    name: "tray:" + key
    priority: 45
    active: shown && attention

    // False for the first seconds of the island: what apps report while
    // the session starts is not news.
    property bool settled: true
    property int seenBadge: 0
    function notify() {
        if (settled && Config.trayAlerts)
            alert(3000)
    }
    onActiveChanged: {
        if (active)
            notify()
    }
    onBadgeChanged: {
        if (badge > seenBadge && active)
            notify()
        seenBadge = badge
    }

    component Glyph: Item {
        id: glyph
        property int size: 20
        property bool showBadge: false
        width: size
        height: size
        Rectangle {
            anchors.centerIn: parent
            width: glyph.size + 6
            height: width
            radius: width / 3
            color: "transparent"
            border.width: 1.5
            border.color: Theme.accent
        }
        IconImage {
            anchors.centerIn: parent
            source: root.icon
            size: glyph.size
        }
        Rectangle {
            visible: glyph.showBadge && root.badge > 0
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
                text: root.badge > 99 ? "99+" : root.badge
                color: "white"
                font.pixelSize: 9
                font.weight: Font.Bold
            }
        }
    }

    compactLeading: Component { Glyph { size: 18 } }

    compactTrailing: Component {
        Text {
            width: Math.min(implicitWidth, 160)
            text: root.badge > 0 ? root.badge : root.title
            elide: Text.ElideRight
            color: Theme.foreground
            font.pixelSize: 13
            font.features: { "tnum": 1 }
        }
    }

    minimal: Component { Glyph { size: 16 } }

    expanded: Component {
        Item {
            implicitWidth: 380
            implicitHeight: 72

            Row {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 20
                anchors.rightMargin: 16
                spacing: 14

                Glyph {
                    anchors.verticalCenter: parent.verticalCenter
                    size: 32
                    showBadge: true
                }
                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - 32 - buttons.width - 2 * parent.spacing
                    Text {
                        width: parent.width
                        text: root.title
                        elide: Text.ElideRight
                        color: Theme.foreground
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }
                    Text {
                        width: parent.width
                        text: root.subtitle
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
                            Tray.activate(root.key)
                            ShellControl.Collapse()
                        }
                    }
                    IconButton {
                        visible: root.hasMenu
                        width: 30
                        height: 30
                        icon: "view-more-horizontal-symbolic"
                        iconSize: 14
                        onClicked: Tray.showMenu(root.key)
                    }
                }
            }
        }
    }
}
