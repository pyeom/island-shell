pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Search field and results. Keys: Up/Down (or Tab) to move, Left/Right to
// pick a desktop action, Enter to launch, Esc to close.
Item {
    id: panel

    signal close()

    property var results: Apps.search(search.text, 30)
    property int selected: 0
    // -1: the app itself, otherwise an index into its actions.
    property int selectedAction: -1

    readonly property int rowHeight: 48
    readonly property int visibleRows: 6

    implicitWidth: 560
    implicitHeight: 64 + (results.length > 0 ? Math.min(results.length, visibleRows) * rowHeight + 12
                                             : 40)

    onResultsChanged: {
        selected = 0
        selectedAction = -1
    }

    function launchSelected() {
        const app = results[selected]
        if (!app)
            return
        const action = selectedAction >= 0 ? app.actions[selectedAction].id : ""
        Apps.launch(app.id, action)
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
                    const app = panel.results[panel.selected]
                    if (!app || cursorPosition < text.length)
                        return
                    panel.selectedAction = Math.min(panel.selectedAction + 1, app.actions.length - 1)
                    break
                }
                case Qt.Key_Left:
                    if (panel.selectedAction < 0)
                        return
                    panel.selectedAction -= 1
                    break
                case Qt.Key_Return:
                case Qt.Key_Enter:
                    panel.launchSelected()
                    break
                default:
                    return
                }
                event.accepted = true
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                visible: !search.text
                text: qsTr("Search apps")
                color: Theme.secondaryForeground
                font: search.font
            }
        }
    }

    Text {
        anchors.top: field.bottom
        anchors.topMargin: 12
        anchors.horizontalCenter: parent.horizontalCenter
        visible: panel.results.length === 0
        text: qsTr("No results")
        color: Theme.secondaryForeground
        font.pixelSize: 13
    }

    ListView {
        id: list
        anchors.top: field.bottom
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

            width: list.width
            height: panel.rowHeight
            radius: 14
            color: current ? Qt.rgba(1, 1, 1, 0.12)
                 : area.containsMouse ? Qt.rgba(1, 1, 1, 0.06) : "transparent"

            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    panel.selected = row.index
                    panel.selectedAction = -1
                    panel.launchSelected()
                }
            }

            IconImage {
                id: appIcon
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                source: row.modelData.icon
                size: 30
            }

            Column {
                anchors.left: appIcon.right
                anchors.leftMargin: 12
                anchors.right: actionsRow.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    width: parent.width
                    text: row.modelData.name
                    elide: Text.ElideRight
                    color: Theme.foreground
                    font.pixelSize: 14
                    font.weight: Font.Medium
                }
                Text {
                    width: parent.width
                    visible: text !== ""
                    text: row.modelData.genericName || row.modelData.comment
                    elide: Text.ElideRight
                    color: Theme.secondaryForeground
                    font.pixelSize: 11
                }
            }

            // Desktop actions of the selected row.
            Row {
                id: actionsRow
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                spacing: 6
                visible: row.current && row.modelData.actions.length > 0

                Repeater {
                    model: row.current ? row.modelData.actions.slice(0, 3) : []
                    delegate: IconButton {
                        required property var modelData
                        required property int index
                        height: 26
                        text: modelData.name
                        highlighted: index === panel.selectedAction
                        onClicked: {
                            panel.selectedAction = index
                            panel.launchSelected()
                        }
                    }
                }
            }
        }
    }
}
