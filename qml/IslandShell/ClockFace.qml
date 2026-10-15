import QtQuick

// Idle content: time in compact, time + date in peek, large clock expanded.
Item {
    id: root

    property string mode: "compact"
    property date now: new Date()

    Timer {
        interval: 1000
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: root.now = new Date()
    }

    component Face: Item {
        id: face
        required property string forMode
        anchors.fill: parent
        opacity: root.mode === forMode ? 1 : 0
        scale: root.mode === forMode ? 1 : 0.92
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
        Behavior on scale { NumberAnimation { duration: 260; easing.type: Easing.OutCubic } }
    }

    Face {
        forMode: "compact"
        Text {
            anchors.centerIn: parent
            text: Qt.formatTime(root.now, "HH:mm")
            color: "white"
            font.pixelSize: 14
            font.weight: Font.DemiBold
            font.features: { "tnum": 1 }
        }
    }

    Face {
        forMode: "peek"
        Text {
            anchors.left: parent.left
            anchors.leftMargin: 18
            anchors.verticalCenter: parent.verticalCenter
            text: Qt.formatTime(root.now, "HH:mm")
            color: "white"
            font.pixelSize: 15
            font.weight: Font.DemiBold
            font.features: { "tnum": 1 }
        }
        Text {
            anchors.right: parent.right
            anchors.rightMargin: 18
            anchors.verticalCenter: parent.verticalCenter
            text: root.now.toLocaleDateString(Qt.locale(), "ddd d MMM")
            color: "#a0a0a0"
            font.pixelSize: 13
        }
    }

    Face {
        forMode: "expanded"
        Column {
            anchors.centerIn: parent
            spacing: 2
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: Qt.formatTime(root.now, "HH:mm")
                color: "white"
                font.pixelSize: 56
                font.weight: Font.Bold
                font.features: { "tnum": 1 }
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.now.toLocaleDateString(Qt.locale(), "dddd d MMMM yyyy")
                color: "#a0a0a0"
                font.pixelSize: 15
            }
        }
    }
}
