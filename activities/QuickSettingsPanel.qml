pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

Item {
    id: panel

    signal close()

    implicitWidth: 520
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
        // Shows a chevron that opens a detail list (Wi-Fi networks, devices).
        property bool hasDetail: false

        signal clicked()
        signal detailClicked()

        width: (column.width - 3 * 10) / 4
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

        IconButton {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 6
            visible: tile.hasDetail && tile.available
            width: 22
            height: 22
            icon: "go-next-symbolic"
            iconSize: 10
            color: "transparent"
            onClicked: tile.detailClicked()
        }
    }

    // Which device list is open: "", "sinks" or "sources".
    property string devicePicker: ""

    component SliderRow: Row {
        id: sliderRow
        property string icon
        property real value
        property string label
        // Shows a chevron that opens a device list.
        property bool expandable: false
        property bool expanded: false
        signal iconClicked()
        signal moved(real value)
        signal expandClicked()

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
            width: parent.width - 32 - 44 - 24 - (sliderRow.expandable ? 28 + 12 : 0)
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
        IconButton {
            anchors.verticalCenter: parent.verticalCenter
            visible: sliderRow.expandable
            width: 28
            height: 28
            icon: "go-down-symbolic"
            iconSize: 12
            highlighted: sliderRow.expanded
            rotation: sliderRow.expanded ? 180 : 0
            Behavior on rotation { NumberAnimation { duration: 150 } }
            onClicked: sliderRow.expandClicked()
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
            visible: panel.devicePicker === ""

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
                hasDetail: Network.hasWifi
                onDetailClicked: panel.devicePicker = "wifi"
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
                hasDetail: true
                onDetailClicked: panel.devicePicker = "bluetooth"
                onClicked: Bluetooth.setPowered(!Bluetooth.powered)
            }

            Tile {
                icon: "redshift-status-on"
                title: qsTr("Night light")
                subtitle: NightLight.running ? qsTr("On") : qsTr("Off")
                on: NightLight.running
                available: NightLight.available
                onClicked: NightLight.setRunning(!NightLight.running)
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
            visible: Audio.available && (panel.devicePicker === "" || panel.devicePicker === "sinks")
            icon: Audio.muted ? "audio-volume-muted-symbolic" : "audio-volume-high-symbolic"
            value: Audio.muted ? 0 : Math.min(Audio.volume, 1)
            label: Audio.muted ? qsTr("Mute") : Math.round(Audio.volume * 100) + "%"
            expandable: Audio.sinks.length > 1
            expanded: panel.devicePicker === "sinks"
            onIconClicked: Audio.setMuted(!Audio.muted)
            onExpandClicked: panel.devicePicker = expanded ? "" : "sinks"
            onMoved: v => {
                if (Audio.muted)
                    Audio.setMuted(false)
                Audio.setVolume(v)
            }
        }

        // Wi-Fi / Bluetooth detail views.
        Row {
            visible: panel.devicePicker === "wifi" || panel.devicePicker === "bluetooth"
            width: column.width
            spacing: 10
            IconButton {
                width: 28
                height: 28
                icon: "go-previous-symbolic"
                iconSize: 12
                onClicked: panel.devicePicker = ""
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: panel.devicePicker === "wifi" ? qsTr("Wi-Fi") : qsTr("Bluetooth")
                color: Theme.foreground
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }
        }
        Loader {
            width: column.width
            active: panel.devicePicker === "wifi"
            visible: active
            sourceComponent: WifiList {}
        }
        Loader {
            width: column.width
            active: panel.devicePicker === "bluetooth"
            visible: active
            sourceComponent: BluetoothList {}
        }

        AudioDeviceList {
            visible: panel.devicePicker === "sinks"
            width: column.width
            devices: Audio.sinks
            icon: "audio-speakers-symbolic"
        }

        SliderRow {
            visible: Audio.hasMicrophone && (panel.devicePicker === "" || panel.devicePicker === "sources")
            icon: Audio.micMuted ? "microphone-sensitivity-muted-symbolic" : "audio-input-microphone-symbolic"
            value: Audio.micMuted ? 0 : Math.min(Audio.micVolume, 1)
            label: Audio.micMuted ? qsTr("Mute") : Math.round(Audio.micVolume * 100) + "%"
            expandable: Audio.sources.length > 1
            expanded: panel.devicePicker === "sources"
            onIconClicked: Audio.setMicMuted(!Audio.micMuted)
            onExpandClicked: panel.devicePicker = expanded ? "" : "sources"
            onMoved: v => {
                if (Audio.micMuted)
                    Audio.setMicMuted(false)
                Audio.setMicVolume(v)
            }
        }

        AudioDeviceList {
            visible: panel.devicePicker === "sources"
            width: column.width
            devices: Audio.sources
            icon: "audio-input-microphone-symbolic"
        }

        SliderRow {
            visible: Brightness.available && panel.devicePicker === ""
            icon: "brightness-high-symbolic"
            value: Brightness.level
            label: Math.round(Brightness.level * 100) + "%"
            onMoved: v => Brightness.setLevel(v)
        }

        // Power profile: segmented control.
        Rectangle {
            visible: PowerProfiles.available && PowerProfiles.profiles.length > 0
                     && panel.devicePicker === ""
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
            visible: panel.devicePicker === ""
            IconButton {
                height: 30
                icon: "configure-symbolic"
                iconSize: 14
                onClicked: {
                    panel.close()
                    ShellControl.ShowSettings()
                }
            }
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
