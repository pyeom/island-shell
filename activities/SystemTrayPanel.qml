pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// The tray apps as a list (those that want attention first), and the menu of
// one of them. Click: activate (or the menu for menu-only items); right
// click or ⋯: menu; middle click: secondary action; wheel: scroll.
Item {
    id: panel

    signal close()

    // Opens on this item's menu (Tray.showMenu).
    property string initialMenu: ""

    implicitWidth: 400
    implicitHeight: column.implicitHeight + 36

    // The item whose menu is open; "" shows the list.
    property string menuKey: ""
    property string menuTitle: ""
    property string menuIcon: ""
    // Submenu ids from the top of the menu.
    property var menuStack: []

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
        slide(1)
    }

    function enter(id: int) {
        trayMenu.aboutToShow(id)
        menuStack = menuStack.concat([id])
        slide(1)
    }

    function back() {
        if (menuStack.length > 0) {
            menuStack = menuStack.slice(0, -1)
        } else {
            menuKey = ""
            trayMenu.service = ""
            trayMenu.path = ""
        }
        slide(-1)
    }

    // Pages come in from the side they lead to.
    function slide(direction: int) {
        if (Config.animations === "off")
            return
        slideIn.from = direction * 28
        slideAnimation.restart()
    }

    onInitialMenuChanged: {
        if (initialMenu !== "")
            openMenu(initialMenu)
    }

    focus: true
    Component.onCompleted: {
        forceActiveFocus()
        if (initialMenu !== "")
            openMenu(initialMenu)
    }
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

    ParallelAnimation {
        id: slideAnimation
        NumberAnimation { id: slideIn; target: shift; property: "x"; to: 0; duration: 200; easing.type: Easing.OutCubic }
        NumberAnimation { target: pages; property: "opacity"; from: 0.3; to: 1; duration: 200 }
    }

    component Badge: Rectangle {
        required property int count
        visible: count > 0
        width: Math.max(height, label.implicitWidth + 8)
        height: 16
        radius: 8
        color: Theme.danger
        border.width: 2
        border.color: Theme.background
        Text {
            id: label
            anchors.centerIn: parent
            text: parent.count > 99 ? "99+" : parent.count
            color: "white"
            font.pixelSize: 9
            font.weight: Font.Bold
        }
    }

    // One tray app. `wanted`: the section it belongs to (attention or not).
    component TrayRow: Rectangle {
        id: row
        required property string key
        required property string title
        required property string tooltip
        required property string icon
        required property int badge
        required property bool attention
        required property bool shown
        required property bool hasMenu
        property bool wanted: false

        visible: shown && attention === wanted
        width: list.width
        height: 46
        radius: 14
        color: area.pressed ? Qt.rgba(1, 1, 1, 0.16)
             : area.containsMouse ? Qt.rgba(1, 1, 1, 0.1)
             : "transparent"
        Behavior on color { ColorAnimation { duration: 120 } }

        IconImage {
            id: rowIcon
            anchors.left: parent.left
            anchors.leftMargin: 9
            anchors.verticalCenter: parent.verticalCenter
            source: row.icon
            size: 28
        }
        Badge {
            count: row.badge
            anchors.right: rowIcon.right
            anchors.top: rowIcon.top
            anchors.rightMargin: -7
            anchors.topMargin: -5
        }
        Column {
            anchors.left: rowIcon.right
            anchors.leftMargin: 12
            anchors.right: more.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            Text {
                width: parent.width
                text: row.title
                elide: Text.ElideRight
                color: Theme.foreground
                font.pixelSize: 13
                font.weight: row.attention ? Font.DemiBold : Font.Normal
            }
            Text {
                width: parent.width
                visible: text !== ""
                text: row.tooltip !== "" ? row.tooltip
                    : row.badge > 0 ? qsTr("%n unread", "", row.badge)
                    : ""
                elide: Text.ElideRight
                color: Theme.secondaryForeground
                font.pixelSize: 11
            }
        }

        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
            onClicked: mouse => {
                if (mouse.button === Qt.RightButton)
                    Tray.contextMenu(row.key)
                else if (mouse.button === Qt.MiddleButton)
                    Tray.secondaryActivate(row.key)
                else
                    Tray.activate(row.key)
            }
            onWheel: wheel => {
                const horizontal = wheel.angleDelta.y === 0
                Tray.scroll(row.key, horizontal ? wheel.angleDelta.x : wheel.angleDelta.y, horizontal)
            }
        }

        // Above the row's MouseArea.
        IconButton {
            id: more
            anchors.right: parent.right
            anchors.rightMargin: 9
            anchors.verticalCenter: parent.verticalCenter
            visible: row.hasMenu
            opacity: area.containsMouse || hovered ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 120 } }
            width: 26
            height: 26
            icon: "view-more-horizontal-symbolic"
            iconSize: 12
            onClicked: Tray.contextMenu(row.key)
        }
    }

    component SectionLabel: Text {
        width: list.width
        leftPadding: 9
        topPadding: 4
        bottomPadding: 2
        color: Theme.secondaryForeground
        font.pixelSize: 10
        font.weight: Font.DemiBold
        font.capitalization: Font.AllUppercase
        font.letterSpacing: 0.6
    }

    Column {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 18
        spacing: 10

        // Header: title and count, or back button and the app in the menu.
        Item {
            width: column.width
            height: 28

            Row {
                anchors.left: parent.left
                anchors.right: countText.left
                anchors.verticalCenter: parent.verticalCenter
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
                    size: 20
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - x
                    leftPadding: panel.menuKey === "" ? 9 : 0
                    text: panel.menuKey === "" ? qsTr("Tray")
                        : panel.submenuTitle !== "" ? panel.menuTitle + "  ›  " + panel.submenuTitle
                        : panel.menuTitle
                    elide: Text.ElideRight
                    color: Theme.foreground
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                }
            }
            Text {
                id: countText
                anchors.right: parent.right
                anchors.rightMargin: 9
                anchors.verticalCenter: parent.verticalCenter
                visible: panel.menuKey === "" && Tray.count > 0
                text: qsTr("%n app(s)", "", Tray.count)
                color: Theme.secondaryForeground
                font.pixelSize: 11
            }
        }

        Item {
            width: column.width
            height: pages.height
            clip: true

            Column {
                id: pages
                width: parent.width
                spacing: 10
                transform: Translate { id: shift }

                // The list.
                Text {
                    visible: panel.menuKey === "" && Tray.count === 0
                    width: column.width
                    leftPadding: 9
                    text: Tray.available ? qsTr("No apps in the tray") : qsTr("Another program shows the tray")
                    color: Theme.secondaryForeground
                    font.pixelSize: 12
                    bottomPadding: 6
                }

                Flickable {
                    id: list
                    visible: panel.menuKey === "" && Tray.count > 0
                    width: column.width
                    height: Math.min(contentHeight, 400)
                    contentHeight: rows.implicitHeight
                    clip: contentHeight > height
                    interactive: contentHeight > height
                    boundsBehavior: Flickable.StopAtBounds

                    Column {
                        id: rows
                        width: list.width
                        spacing: 2

                        SectionLabel {
                            visible: Tray.attentionCount > 0
                            text: qsTr("Want attention")
                        }
                        Repeater {
                            model: Tray
                            delegate: TrayRow { wanted: true }
                        }
                        SectionLabel {
                            visible: Tray.attentionCount > 0 && Tray.count > Tray.attentionCount
                            topPadding: 10
                            text: qsTr("Apps")
                        }
                        Repeater {
                            model: Tray
                            delegate: TrayRow { wanted: false }
                        }
                    }
                }

                Text {
                    visible: panel.menuKey === "" && Tray.count > 0
                    width: column.width
                    leftPadding: 9
                    text: qsTr("Click opens the app · ⋯ or right click, its menu")
                    elide: Text.ElideRight
                    color: Qt.rgba(1, 1, 1, 0.35)
                    font.pixelSize: 11
                }

                // The menu.
                Text {
                    visible: panel.menuKey !== "" && panel.menuEntries.length === 0
                    leftPadding: 9
                    text: trayMenu.loading ? qsTr("Loading…") : qsTr("Empty menu")
                    color: Theme.secondaryForeground
                    font.pixelSize: 12
                    bottomPadding: 6
                }

                ListView {
                    id: menuList
                    visible: panel.menuKey !== "" && count > 0
                    width: column.width
                    height: Math.min(contentHeight, 400)
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
                        height: modelData.separator ? 11 : 36

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
                            radius: 12
                            color: entryArea.pressed && entry.modelData.enabled ? Qt.rgba(1, 1, 1, 0.16)
                                 : entryArea.containsMouse && entry.modelData.enabled ? Qt.rgba(1, 1, 1, 0.1)
                                 : "transparent"
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
                                    font.pixelSize: 13
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
                                        panel.enter(entry.modelData.id)
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
    }
}
