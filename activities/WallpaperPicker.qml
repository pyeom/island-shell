pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Wallpaper picker. `islandctl wallpaper` opens it; `islandctl wallpaper
// random` applies a random one without opening anything.
Activity {
    id: root

    property bool open: false

    name: "wallpaper"
    priority: 1000
    modal: true
    active: open

    onDismissRequested: open = false

    property Connections _control: Connections {
        target: ShellControl
        function onWallpaperPickRequested() {
            root.open = !root.open
            if (root.open)
                Wallpapers.refresh()
        }
        function onWallpaperRandomRequested() {
            Wallpapers.random()
        }
    }

    expanded: Component {
        Item {
            id: panel

            readonly property int columns: 4
            readonly property int cell: 128

            implicitWidth: columns * (cell + 10) + 26
            implicitHeight: Math.min(grid.contentHeight, 3 * (cell * 9 / 16 + 10)) + 90

            focus: true
            Component.onCompleted: forceActiveFocus()
            Keys.onEscapePressed: root.open = false

            Row {
                id: titleRow
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 18
                spacing: 10
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - shuffle.width - 10
                    text: qsTr("Wallpaper")
                    color: Theme.foreground
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                }
                IconButton {
                    id: shuffle
                    icon: "media-playlist-shuffle-symbolic"
                    text: qsTr("Random")
                    onClicked: {
                        Wallpapers.random()
                        root.open = false
                    }
                }
            }

            Text {
                anchors.centerIn: grid
                visible: Wallpapers.files.length === 0
                text: qsTr("No images in %1").arg(Config.get("wallpaper.directory", "~/.config/wallpapers"))
                color: Theme.secondaryForeground
                font.pixelSize: 12
            }

            GridView {
                id: grid
                anchors.top: titleRow.bottom
                anchors.topMargin: 14
                anchors.horizontalCenter: parent.horizontalCenter
                width: panel.columns * (panel.cell + 10)
                height: parent.height - titleRow.height - 50
                clip: true
                cellWidth: panel.cell + 10
                cellHeight: panel.cell * 9 / 16 + 10
                model: Wallpapers.files
                boundsBehavior: Flickable.StopAtBounds

                delegate: Item {
                    id: tile
                    required property string modelData
                    readonly property bool current: modelData === Wallpapers.current
                    width: grid.cellWidth
                    height: grid.cellHeight

                    Rectangle {
                        anchors.centerIn: parent
                        width: panel.cell
                        height: panel.cell * 9 / 16
                        radius: 10
                        color: Qt.rgba(1, 1, 1, 0.08)
                        clip: true

                        RoundedImage {
                            anchors.fill: parent
                            radius: parent.radius
                            source: "file://" + tile.modelData
                            sourceSize: Qt.size(panel.cell * 2, panel.cell * 9 / 8)
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                        }
                        // Above the image: current wallpaper and hover.
                        Rectangle {
                            anchors.fill: parent
                            radius: parent.radius
                            color: "transparent"
                            border.width: tile.current ? 3 : area.containsMouse ? 2 : 0
                            border.color: tile.current ? Theme.accent : Qt.rgba(1, 1, 1, 0.6)
                        }
                        MouseArea {
                            id: area
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                Wallpapers.apply(tile.modelData)
                                root.open = false
                            }
                        }
                    }
                }
            }
        }
    }
}
