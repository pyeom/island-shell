pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Volume of each app playing audio.
Column {
    id: root

    spacing: 4
    visible: Audio.streams.length > 0

    Text {
        text: qsTr("Apps")
        color: Theme.secondaryForeground
        font.pixelSize: 11
        font.weight: Font.DemiBold
        leftPadding: 10
        topPadding: 6
    }

    Repeater {
        model: Audio.streams
        delegate: Row {
            id: row
            required property AudioStream modelData

            width: root.width
            height: 40
            spacing: 10

            Item {
                anchors.verticalCenter: parent.verticalCenter
                width: 32
                height: 32

                IconImage {
                    anchors.centerIn: parent
                    source: Apps.iconForClass(row.modelData.iconName)
                    size: 20
                    opacity: row.modelData.muted ? 0.4 : 1
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: Audio.setStreamMuted(row.modelData.id, !row.modelData.muted)
                }
            }
            Column {
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width - 32 - 44 - 2 * parent.spacing
                spacing: 2
                Text {
                    width: parent.width
                    text: row.modelData.title ? row.modelData.name + "  ·  " + row.modelData.title
                                              : row.modelData.name
                    elide: Text.ElideRight
                    color: Theme.foreground
                    font.pixelSize: 11
                }
                IslandSlider {
                    width: parent.width
                    height: 16
                    value: row.modelData.muted ? 0 : Math.min(row.modelData.volume, 1)
                    onMoved: v => {
                        if (row.modelData.muted)
                            Audio.setStreamMuted(row.modelData.id, false)
                        Audio.setStreamVolume(row.modelData.id, v)
                    }
                }
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                width: 44
                horizontalAlignment: Text.AlignRight
                text: row.modelData.muted ? qsTr("Mute") : Math.round(row.modelData.volume * 100) + "%"
                color: Theme.secondaryForeground
                font.pixelSize: 12
                font.features: { "tnum": 1 }
            }
        }
    }
}
