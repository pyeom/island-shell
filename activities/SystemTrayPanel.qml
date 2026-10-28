pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// The tray items, and the menu of one of them. Click: activate (or the menu
// for menu-only items); right click: menu; middle click: secondary action;
// wheel: scroll.
Item {
    id: panel

    signal close()

    implicitWidth: 420
    implicitHeight: column.implicitHeight + 36

    // The item whose menu is open; "" shows the items.
    property string menuKey: ""
    property string menuTitle: ""
    property string menuIcon: ""
    // Submenu ids from the top of the menu.
    property var menuStack: []
    // Title of the hovered item, shown under the grid.
    property string hint: ""

    readonly property var menuEntries: {
        let list = trayMenu.entries
        for (const id of panel.menuStack) {
            const entry = list.find(e => e.id === id)
            if (!entry)
                return []
            list = entry.children
        }
        return list
    }
    readonly property string submenuTitle: {
        let list = trayMenu.entries
        let title = ""
        for (const id of panel.menuStack) {
            const entry = list.find(e => e.id === id)
            if (!entry)
                break
            title = entry.label
            list = entry.children
        }
        return title
    }

    function openMenu(key: string) {
        const item = Tray.item(key)
        if (!item.menuPath)
            return
        menuStack = []
        menuTitle = item.title
        menuIcon = item.icon
        trayMenu.service = item.service
        trayMenu.path = item.menuPath
        menuKey = key
    }

    function back() {
        if (menuStack.length > 0) {
            menuStack = menuStack.slice(0, -1)
            return
        }
        menuKey = ""
        trayMenu.service = ""
        trayMenu.path = ""
    }

    focus: true
    Component.onCompleted: forceActiveFocus()
    Keys.onEscapePressed: menuKey !== "" ? back() : close()

    TrayMenu {
        id: trayMenu
    }

    Connections {
        target: Tray
        function onActivated(key: string) {
            panel.close()
        }
        function onMenuRequested(key: string) {
            panel.openMenu(key)
        }
        // The app quit while its menu was open.
        function onRowsRemoved() {
            if (panel.menuKey !== "" && !Tray.item(panel.menuKey).service)
                panel.back()
        }
    }

    Column {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 18
        spacing: 12

        // Header: title, or back button and the app in the menu view.
        Row {
            width: column.width
            height: 28
            spacing: 10

            IconButton {
                visible: panel.menuKey !== ""
                width: 28
                height: 28
                icon: "go-previous-symbolic"
                iconSize: 12
                onClicked: panel.back()
            }
            IconImage {
                anchors.verticalCenter: parent.verticalCenter
                visible: panel.menuKey !== "" && panel.menuIcon !== ""
                source: panel.menuIcon
                size: 18
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: column.width - x
                text: panel.menuKey === "" ? qsTr("Tray")
                    : panel.submenuTitle !== "" ? panel.menuTitle + "  ›  " + panel.submenuTitle
                    : panel.menuTitle
                elide: Text.ElideRight
                color: Theme.foreground
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }
        }

        // Items.
        Text {
            visible: panel.menuKey === "" && Tray.count === 0
            width: column.width
            text: Tray.available ? qsTr("No apps in the tray") : qsTr("Another program shows the tray")
            color: Theme.secondaryForeground
            font.pixelSize: 12
            bottomPadding: 6
        }

        Grid {
            id: grid
            visible: panel.menuKey === "" && Tray.count > 0
            columns: 6
            spacing: 6

            Repeater {
                model: Tray
                delegate: Rectangle {
                    id: cell
                    required property string key
                    required property string title
                    required property string tooltip
                    required property string status
                    required property string icon
                    required property bool shown

                    visible: shown
                    width: (column.width - (grid.columns - 1) * grid.spacing) / grid.columns
                    height: width
                    radius: 16
                    color: area.pressed ? Qt.rgba(1, 1, 1, 0.22)
                         : area.containsMouse ? Qt.rgba(1, 1, 1, 0.16)
                         : Qt.rgba(1, 1, 1, 0.08)
                    Behavior on color { ColorAnimation { duration: 120 } }

                    IconImage {
                        anchors.centerIn: parent
                        source: cell.icon
                        size: 26
                    }
                    // Wants attention (a new message...).
                    Rectangle {
                        visible: cell.status === "NeedsAttention"
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: 8
                        width: 8
                        height: 8
                        radius: 4
                        color: Theme.accent
                    }

                    MouseArea {
                        id: area
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
                        onContainsMouseChanged: {
                            if (containsMouse)
                                panel.hint = cell.tooltip !== "" ? cell.title + " — " + cell.tooltip : cell.title
                            else if (panel.hint.startsWith(cell.title))
                                panel.hint = ""
                        }
                        onClicked: mouse => {
                            if (mouse.button === Qt.RightButton)
                                Tray.contextMenu(cell.key)
                            else if (mouse.button === Qt.MiddleButton)
                                Tray.secondaryActivate(cell.key)
                            else
                                Tray.activate(cell.key)
                        }
                        onWheel: wheel => {
                            const horizontal = wheel.angleDelta.y === 0
                            Tray.scroll(cell.key, horizontal ? wheel.angleDelta.x : wheel.angleDelta.y, horizontal)
                        }
                    }
                }
            }
        }

        Text {
            visible: panel.menuKey === "" && Tray.count > 0
            width: column.width
            text: panel.hint !== "" ? panel.hint : qsTr("Right click for the menu")
            elide: Text.ElideRight
            color: Theme.secondaryForeground
            font.pixelSize: 11
        }

        // Menu.
        Text {
            visible: panel.menuKey !== "" && panel.menuEntries.length === 0
            text: trayMenu.loading ? qsTr("Loading…") : qsTr("Empty menu")
            color: Theme.secondaryForeground
            font.pixelSize: 12
            bottomPadding: 6
        }

        ListView {
            id: menuList
            visible: panel.menuKey !== "" && count > 0
            width: column.width
            height: Math.min(contentHeight, 380)
            clip: true
            interactive: contentHeight > height
            boundsBehavior: Flickable.StopAtBounds
            model: panel.menuEntries
            readonly property bool hasToggles: panel.menuEntries.some(e => e.toggle !== "")
            readonly property bool hasIcons: panel.menuEntries.some(e => e.icon !== "")

            delegate: Item {
                id: entry
                required property var modelData
                width: menuList.width
                height: modelData.separator ? 11 : 34

                Rectangle {
                    visible: entry.modelData.separator
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    height: 1
                    color: Qt.rgba(1, 1, 1, 0.12)
                }

                Rectangle {
                    visible: !entry.modelData.separator
                    anchors.fill: parent
                    radius: 10
                    color: entryArea.containsMouse && entry.modelData.enabled ? Qt.rgba(1, 1, 1, 0.1) : "transparent"
                    opacity: entry.modelData.enabled ? 1 : 0.4

                    Row {
                        anchors.left: parent.left
                        anchors.right: chevron.left
                        anchors.leftMargin: 10
                        anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 10

                        Item {
                            visible: menuList.hasToggles
                            width: 16
                            height: 16
                            anchors.verticalCenter: parent.verticalCenter
                            IconImage {
                                anchors.centerIn: parent
                                visible: entry.modelData.toggle === "checkmark" && entry.modelData.checked
                                source: Icons.source("object-select-symbolic")
                                size: 14
                            }
                            Rectangle {
                                anchors.centerIn: parent
                                visible: entry.modelData.toggle === "radio"
                                width: 12
                                height: 12
                                radius: 6
                                color: "transparent"
                                border.width: 1.5
                                border.color: entry.modelData.checked ? Theme.accent : Theme.secondaryForeground
                                Rectangle {
                                    anchors.centerIn: parent
                                    visible: entry.modelData.checked
                                    width: 6
                                    height: 6
                                    radius: 3
                                    color: Theme.accent
                                }
                            }
                        }
                        IconImage {
                            visible: menuList.hasIcons
                            anchors.verticalCenter: parent.verticalCenter
                            source: entry.modelData.icon
                            size: 16
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width - x
                            text: entry.modelData.label
                            elide: Text.ElideRight
                            color: Theme.foreground
                            font.pixelSize: 12
                        }
                    }

                    IconImage {
                        id: chevron
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        visible: entry.modelData.submenu
                        width: visible ? size : 0
                        source: Icons.source("go-next-symbolic")
                        size: 12
                    }

                    MouseArea {
                        id: entryArea
                        anchors.fill: parent
                        hoverEnabled: true
                        enabled: entry.modelData.enabled
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (entry.modelData.submenu) {
                                trayMenu.aboutToShow(entry.modelData.id)
                                panel.menuStack = panel.menuStack.concat([entry.modelData.id])
                            } else {
                                trayMenu.activate(entry.modelData.id)
                                panel.close()
                            }
                        }
                    }
                }
            }
        }
    }
}
