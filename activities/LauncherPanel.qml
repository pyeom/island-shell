pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Search field and results (apps, windows, calculator, commands, web,
// emoji; see LauncherSearch). Keys: Up/Down or Tab to move, Left/Right to
// pick an app's desktop action, Enter to activate (Shift+Enter runs a
// command in a terminal), Esc to close.
Item {
    id: panel

    signal close()

    // LauncherSearch.revision: re-query when the window list arrives.
    property var results: LauncherSearch.revision >= 0 ? LauncherSearch.query(search.text, 30) : []
    property int selected: 0
    // -1: the item itself, otherwise an index into an app's actions.
    property int selectedAction: -1

    readonly property int rowHeight: 48
    readonly property int visibleRows: 6
    readonly property bool showHelp: search.text.length === 0

    implicitWidth: 560
    implicitHeight: 64 + (results.length > 0 ? Math.min(results.length, visibleRows) * rowHeight + 12 : 40)
                    + (showHelp ? 28 : 0)

    Component.onCompleted: LauncherSearch.refresh()

    onResultsChanged: {
        selected = 0
        selectedAction = -1
    }

    function activateSelected(alternate: bool) {
        const item = results[selected]
        if (!item || item.kind === "error")
            return
        LauncherSearch.activate(item, alternate ? 1 : selectedAction)
        panel.close()
    }

    function move(delta: int) {
        if (results.length === 0)
            return
        selected = (selected + delta + results.length) % results.length
        selectedAction = -1
        list.positionViewAtIndex(selected, ListView.Contain)
    }

    Rectangle {
        id: field
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 14
        height: 40
        radius: 20
        color: Qt.rgba(1, 1, 1, 0.1)

        IconImage {
            id: searchIcon
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            source: Icons.source("system-search-symbolic")
            size: 16
        }

        TextInput {
            id: search
            anchors.left: searchIcon.right
            anchors.leftMargin: 10
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            focus: true
            color: Theme.foreground
            font.pixelSize: 16
            clip: true
            Component.onCompleted: forceActiveFocus()

            Keys.onPressed: event => {
                switch (event.key) {
                case Qt.Key_Escape:
                    panel.close()
                    break
                case Qt.Key_Down:
                case Qt.Key_Tab:
                    panel.move(1)
                    break
                case Qt.Key_Up:
                case Qt.Key_Backtab:
                    panel.move(-1)
                    break
                case Qt.Key_Right: {
                    const item = panel.results[panel.selected]
                    if (!item || !item.actions || cursorPosition < text.length)
                        return
                    panel.selectedAction = Math.min(panel.selectedAction + 1, item.actions.length - 1)
                    break
                }
                case Qt.Key_Left:
                    if (panel.selectedAction < 0)
                        return
                    panel.selectedAction -= 1
                    break
                case Qt.Key_Return:
                case Qt.Key_Enter:
                    panel.activateSelected(event.modifiers & Qt.ShiftModifier)
                    break
                default:
                    return
                }
                event.accepted = true
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                visible: !search.text
                text: qsTr("Search apps and windows")
                color: Theme.secondaryForeground
                font: search.font
            }
        }
    }

    Text {
        id: help
        anchors.top: field.bottom
        anchors.topMargin: 8
        anchors.horizontalCenter: parent.horizontalCenter
        visible: panel.showHelp
        text: qsTr("= calculate   > command   ? web   : emoji")
        color: Theme.secondaryForeground
        font.pixelSize: 11
        font.family: "monospace"
    }

    Text {
        anchors.top: field.bottom
        anchors.topMargin: 12
        anchors.horizontalCenter: parent.horizontalCenter
        visible: panel.results.length === 0 && !panel.showHelp
        text: qsTr("No results")
        color: Theme.secondaryForeground
        font.pixelSize: 13
    }

    ListView {
        id: list
        anchors.top: panel.showHelp ? help.bottom : field.bottom
        anchors.topMargin: 8
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8
        height: Math.min(panel.results.length, panel.visibleRows) * panel.rowHeight
        clip: true
        model: panel.results
        boundsBehavior: Flickable.StopAtBounds

        delegate: Rectangle {
            id: row

            required property var modelData
            required property int index
            readonly property bool current: index === panel.selected
            readonly property var actions: modelData.actions ?? []

            width: list.width
            height: panel.rowHeight
            radius: 14
            color: current ? Qt.rgba(1, 1, 1, 0.12)
                 : area.containsMouse ? Qt.rgba(1, 1, 1, 0.06) : "transparent"

            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                onClicked: mouse => {
                    panel.selected = row.index
                    panel.selectedAction = -1
                    panel.activateSelected(mouse.modifiers & Qt.ShiftModifier)
                }
            }

            Item {
                id: glyph
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                width: 30
                height: 30

                IconImage {
                    anchors.centerIn: parent
                    visible: row.modelData.kind !== "emoji"
                    source: row.modelData.icon || ""
                    size: 30
                }
                Text {
                    anchors.centerIn: parent
                    visible: row.modelData.kind === "emoji"
                    text: row.modelData.glyph ?? ""
                    font.pixelSize: 24
                }
            }

            Column {
                anchors.left: glyph.right
                anchors.leftMargin: 12
                anchors.right: actionsRow.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    width: parent.width
                    text: row.modelData.title
                    elide: Text.ElideRight
                    color: row.modelData.kind === "error" ? Theme.secondaryForeground : Theme.foreground
                    font.pixelSize: row.modelData.kind === "calc" ? 18 : 14
                    font.weight: Font.Medium
                    font.features: { "tnum": 1 }
                }
                Text {
                    width: parent.width
                    visible: text !== ""
                    text: row.modelData.subtitle
                    elide: Text.ElideRight
                    color: Theme.secondaryForeground
                    font.pixelSize: 11
                }
            }

            // Desktop actions of the selected app.
            Row {
                id: actionsRow
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                spacing: 6
                visible: row.current && row.actions.length > 0

                Repeater {
                    model: row.current ? row.actions.slice(0, 3) : []
                    delegate: IconButton {
                        required property var modelData
                        required property int index
                        height: 26
                        text: modelData.name
                        highlighted: index === panel.selectedAction
                        onClicked: {
                            panel.selectedAction = index
                            panel.activateSelected(false)
                        }
                    }
                }
            }
        }
    }
}
