pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// The screenshot just taken: thumbnail, then open / copy / edit / delete.
Activity {
    id: root

    readonly property string path: Screenshots.lastPath
    readonly property string source: path ? "file://" + path : ""

    name: "screenshot"
    priority: 75
    active: path !== ""

    onDismissRequested: Screenshots.dismiss()

    property Connections _shots: Connections {
        target: Screenshots
        function onTaken() {
            root.alert(6000)
            root._expire.restart()
        }
    }
    // Leaves the pill a while after the screenshot (it stays in the folder).
    property Timer _expire: Timer {
        interval: 60 * 1000
        onTriggered: Screenshots.dismiss()
    }

    component Thumb: Rectangle {
        property int size: 22
        width: size * 1.6
        height: size
        radius: 5
        color: Qt.rgba(1, 1, 1, 0.1)
        clip: true
        RoundedImage {
            anchors.fill: parent
            radius: parent.radius
            source: root.source
            sourceSize: Qt.size(parent.width * 2, parent.height * 2)
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            cache: false
        }
    }

    compactLeading: Component {
        Row {
            spacing: 8
            Thumb { anchors.verticalCenter: parent.verticalCenter }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Screenshot")
                color: Theme.foreground
                font.pixelSize: 12
            }
        }
    }
    compactTrailing: Component { ClockText {} }
    minimal: Component { Thumb { size: 18 } }

    expanded: Component {
        Item {
            implicitWidth: 440
            implicitHeight: column.implicitHeight + 36

            Column {
                id: column
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 18
                spacing: 12

                Rectangle {
                    width: parent.width
                    height: Math.min(220, width * (preview.implicitHeight / Math.max(1, preview.implicitWidth)))
                    radius: 14
                    color: Qt.rgba(1, 1, 1, 0.06)
                    clip: true
                    RoundedImage {
                        id: preview
                        radius: parent.radius
                        anchors.fill: parent
                        source: root.source
                        sourceSize.width: 800
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        cache: false
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Screenshots.open()
                    }
                }

                Text {
                    width: parent.width
                    text: root.path.split("/").pop() + "  ·  " + qsTr("copied to the clipboard")
                    elide: Text.ElideMiddle
                    color: Theme.secondaryForeground
                    font.pixelSize: 11
                }

                Row {
                    spacing: 8
                    IconButton {
                        icon: "document-open-symbolic"
                        text: qsTr("Open")
                        onClicked: Screenshots.open()
                    }
                    IconButton {
                        icon: "edit-copy-symbolic"
                        text: qsTr("Copy")
                        onClicked: Screenshots.copy()
                    }
                    IconButton {
                        visible: Screenshots.canEdit
                        icon: "document-edit-symbolic"
                        text: qsTr("Edit")
                        onClicked: Screenshots.edit()
                    }
                    IconButton {
                        icon: "edit-delete-symbolic"
                        text: qsTr("Delete")
                        onClicked: Screenshots.remove()
                    }
                }
            }
        }
    }
}
