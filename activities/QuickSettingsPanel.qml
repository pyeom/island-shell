pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

Item {
    id: panel

    signal close()

    implicitWidth: 440
    implicitHeight: column.implicitHeight + 36

    focus: true
    Component.onCompleted: forceActiveFocus()
    Keys.onEscapePressed: panel.close()

    component Tile: Rectangle {
        id: tile

        property string icon
        property string fallbackIcon: ""
        property string title
        property string subtitle
        property bool on: false
        property bool available: true

        signal clicked()

        width: (column.width - 2 * 10) / 3
        height: 64
        radius: 18
        color: !available ? Qt.rgba(1, 1, 1, 0.05)
             : on ? Theme.accent
             : area.containsMouse ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(1, 1, 1, 0.1)
        opacity: available ? 1 : 0.5
        Behavior on color { ColorAnimation { duration: 150 } }

        IconImage {
            id: tileIcon
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: 12
            source: Icons.source(tile.icon, tile.fallbackIcon)
            size: 18
        }
        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 12
            Text {
                width: parent.width
                text: tile.title
                elide: Text.ElideRight
                color: Theme.foreground
                font.pixelSize: 12
                font.weight: Font.DemiBold
            }
            Text {
                width: parent.width
                text: tile.subtitle
                elide: Text.ElideRight
                color: tile.on ? Qt.rgba(1, 1, 1, 0.8) : Theme.secondaryForeground
                font.pixelSize: 10
            }
        }

        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            enabled: tile.available
            cursorShape: Qt.PointingHandCursor
            onClicked: tile.clicked()
        }
    }

    component SliderRow: Row {
        id: sliderRow
        property string icon
        property real value
        property string label
        signal iconClicked()
        signal moved(real value)

        width: column.width
        spacing: 12

        IconButton {
            anchors.verticalCenter: parent.verticalCenter
            width: 32
            height: 32
            icon: sliderRow.icon
            iconSize: 16
            onClicked: sliderRow.iconClicked()
        }
        IslandSlider {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - 32 - 44 - 24
            value: sliderRow.value
            onMoved: v => sliderRow.moved(v)
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            width: 44
            horizontalAlignment: Text.AlignRight
            text: sliderRow.label
            color: Theme.secondaryForeground
            font.pixelSize: 12
            font.features: { "tnum": 1 }
        }
    }

    Column {
        id: column
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 18
        spacing: 14

        Row {
            spacing: 10

            Tile {
                readonly property bool wifi: Network.hasWifi && Network.primaryType !== "ethernet"
                icon: Network.iconName
                title: wifi ? qsTr("Wi-Fi") : Network.primaryType === "vpn" ? qsTr("VPN") : qsTr("Network")
                subtitle: !Network.available ? qsTr("Unavailable")
                        : wifi && !Network.wifiEnabled ? qsTr("Off")
                        : Network.connected ? (Network.ssid || Network.connectionName)
                        : qsTr("Disconnected")
                on: Network.connected
                available: Network.available
                onClicked: {
                    if (Network.hasWifi)
                        Network.setWifiEnabled(!Network.wifiEnabled)
                }
            }

            Tile {
                icon: Bluetooth.powered ? "bluetooth-active-symbolic" : "bluetooth-disabled-symbolic"
                fallbackIcon: Bluetooth.powered ? "network-bluetooth-activated-symbolic"
                                                : "network-bluetooth-inactive-symbolic"
                title: qsTr("Bluetooth")
                subtitle: !Bluetooth.available ? qsTr("Unavailable")
                        : !Bluetooth.powered ? qsTr("Off")
                        : Bluetooth.connectedDevices.length > 0 ? Bluetooth.connectedDevices.join(", ")
                        : qsTr("On")
                on: Bluetooth.powered
                available: Bluetooth.available
                onClicked: Bluetooth.setPowered(!Bluetooth.powered)
            }

            Tile {
                icon: ShellControl.dnd ? "notifications-disabled-symbolic" : "notifications-symbolic"
                title: qsTr("Do not disturb")
                subtitle: ShellControl.dnd ? qsTr("On") : qsTr("Off")
                on: ShellControl.dnd
                onClicked: ShellControl.dnd = !ShellControl.dnd
            }
        }

        SliderRow {
            visible: Audio.available
            icon: Audio.muted ? "audio-volume-muted-symbolic" : "audio-volume-high-symbolic"
            value: Audio.muted ? 0 : Math.min(Audio.volume, 1)
            label: Audio.muted ? qsTr("Mute") : Math.round(Audio.volume * 100) + "%"
            onIconClicked: Audio.setMuted(!Audio.muted)
            onMoved: v => {
                if (Audio.muted)
                    Audio.setMuted(false)
                Audio.setVolume(v)
            }
        }

        SliderRow {
            visible: Brightness.available
            icon: "brightness-high-symbolic"
            value: Brightness.level
            label: Math.round(Brightness.level * 100) + "%"
            onMoved: v => Brightness.setLevel(v)
        }

        // Power profile: segmented control.
        Rectangle {
            visible: PowerProfiles.available && PowerProfiles.profiles.length > 0
            width: column.width
            height: 36
            radius: 18
            color: Qt.rgba(1, 1, 1, 0.08)

            Row {
                anchors.fill: parent
                anchors.margins: 3
                Repeater {
                    model: PowerProfiles.profiles
                    delegate: Rectangle {
                        id: segment
                        required property string modelData
                        readonly property bool current: modelData === PowerProfiles.active
                        width: parent.width / PowerProfiles.profiles.length
                        height: parent.height
                        radius: height / 2
                        color: current ? Qt.rgba(1, 1, 1, 0.2) : "transparent"
                        Behavior on color { ColorAnimation { duration: 150 } }

                        Text {
                            anchors.centerIn: parent
                            text: segment.modelData === "power-saver" ? qsTr("Power saver")
                                : segment.modelData === "performance" ? qsTr("Performance")
                                : qsTr("Balanced")
                            color: segment.current ? Theme.foreground : Theme.secondaryForeground
                            font.pixelSize: 12
                            font.weight: segment.current ? Font.DemiBold : Font.Normal
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: PowerProfiles.setActive(segment.modelData)
                        }
                    }
                }
            }
        }

        Row {
            anchors.right: parent.right
            spacing: 8
            IconButton {
                height: 30
                icon: "system-shutdown-symbolic"
                iconSize: 14
                onClicked: {
                    panel.close()
                    ShellControl.ShowPower()
                }
            }
        }
    }
}
