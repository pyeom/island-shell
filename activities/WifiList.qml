pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// Visible Wi-Fi networks; click to connect (asks for a password when needed).
Column {
    id: root

    property string askingFor: ""

    spacing: 2
    Component.onCompleted: Network.setWatching(true)
    Component.onDestruction: Network.setWatching(false)

    Text {
        visible: Network.networks.length === 0
        text: Network.wifiEnabled ? qsTr("Looking for networks…") : qsTr("Wi-Fi is off")
        color: Theme.secondaryForeground
        font.pixelSize: 12
        topPadding: 6
        bottomPadding: 6
    }

    Text {
        visible: Network.lastError !== ""
        width: root.width
        wrapMode: Text.Wrap
        text: Network.lastError
        color: Theme.danger
        font.pixelSize: 11
    }

    Repeater {
        model: Network.networks.slice(0, 8)
        delegate: Column {
            id: entry
            required property var modelData
            width: root.width

            Rectangle {
                width: parent.width
                height: 34
                radius: 10
                color: area.containsMouse ? Qt.rgba(1, 1, 1, 0.1) : "transparent"

                IconImage {
                    id: signal
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    source: Icons.source("network-wireless-signal-" + (entry.modelData.strength > 75 ? "excellent"
                                         : entry.modelData.strength > 50 ? "good"
                                         : entry.modelData.strength > 25 ? "ok" : "weak") + "-symbolic")
                    size: 16
                }
                Text {
                    anchors.left: signal.right
                    anchors.leftMargin: 10
                    anchors.right: marks.left
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: entry.modelData.ssid
                    elide: Text.ElideRight
                    color: Theme.foreground
                    font.pixelSize: 12
                    font.weight: entry.modelData.active ? Font.DemiBold : Font.Normal
                }
                Row {
                    id: marks
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 6
                    IconImage {
                        visible: entry.modelData.secured
                        source: Icons.source("object-locked-symbolic", "changes-prevent-symbolic")
                        size: 12
                    }
                    IconImage {
                        visible: entry.modelData.active
                        source: Icons.source("object-select-symbolic")
                        size: 14
                    }
                }
                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (entry.modelData.active)
                            Network.disconnectWifi()
                        else if (entry.modelData.secured && !entry.modelData.known)
                            root.askingFor = entry.modelData.ssid
                        else
                            Network.connectTo(entry.modelData.ssid)
                    }
                }
            }

            // Password for a new secured network.
            Rectangle {
                visible: root.askingFor === entry.modelData.ssid
                width: parent.width
                height: 36
                radius: 10
                color: Qt.rgba(1, 1, 1, 0.08)
                TextInput {
                    id: password
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    verticalAlignment: TextInput.AlignVCenter
                    echoMode: TextInput.Password
                    color: Theme.foreground
                    font.pixelSize: 12
                    onVisibleChanged: if (visible) forceActiveFocus()
                    onAccepted: {
                        Network.connectTo(entry.modelData.ssid, text)
                        root.askingFor = ""
                    }
                    Keys.onEscapePressed: root.askingFor = ""
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: !password.text
                        text: qsTr("Password for %1").arg(entry.modelData.ssid)
                        color: Theme.secondaryForeground
                        font: password.font
                    }
                }
            }
        }
    }
}
