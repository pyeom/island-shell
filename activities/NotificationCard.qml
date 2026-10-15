pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// A full notification: icon, text, actions and inline reply.
Item {
    id: card

    required property var notification
    // As the live popup: hovering pauses its timeout.
    property bool popup: false

    readonly property var n: notification

    implicitWidth: 420
    implicitHeight: content.implicitHeight + 36

    HoverHandler {
        onHoveredChanged: if (card.popup) Notifications.paused = hovered
    }
    Component.onDestruction: if (popup) Notifications.paused = false

    // Behind the content: a click on the card is the default action.
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            if (card.n.hasDefault)
                Notifications.invokeAction(card.n.id, "default")
            else if (card.popup)
                Notifications.dismissPopup()
        }
    }

    Column {
        id: content
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 18
        spacing: 10

        Row {
            width: parent.width
            spacing: 12

            Rectangle {
                width: 44
                height: 44
                radius: 12
                color: Qt.rgba(1, 1, 1, 0.08)
                clip: true
                Image {
                    anchors.fill: parent
                    visible: !!card.n.image
                    source: card.n.image || ""
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                }
                IconImage {
                    anchors.centerIn: parent
                    visible: !card.n.image
                    source: card.n.icon
                    size: 28
                }
            }

            Column {
                width: parent.width - 56 - close.width
                spacing: 2

                Text {
                    width: parent.width
                    text: card.n.appName + "  ·  " + Qt.formatTime(card.n.time, Config.clockFormat)
                    elide: Text.ElideRight
                    color: Theme.secondaryForeground
                    font.pixelSize: 11
                }
                Text {
                    width: parent.width
                    text: card.n.summary
                    elide: Text.ElideRight
                    color: Theme.foreground
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                }
                Text {
                    width: parent.width
                    visible: text !== ""
                    text: card.n.body
                    textFormat: Text.StyledText
                    wrapMode: Text.Wrap
                    maximumLineCount: 4
                    elide: Text.ElideRight
                    color: Theme.secondaryForeground
                    linkColor: Theme.accent
                    font.pixelSize: 13
                    onLinkActivated: link => Qt.openUrlExternally(link)
                }
            }

            IconButton {
                id: close
                width: 26
                height: 26
                icon: "window-close-symbolic"
                iconSize: 12
                onClicked: Notifications.dismiss(card.n.id)
            }
        }

        Row {
            visible: card.n.actions.length > 0
            spacing: 8
            Repeater {
                model: card.n.actions.filter(a => a.key !== "default")
                delegate: IconButton {
                    required property var modelData
                    height: 30
                    text: modelData.label
                    onClicked: Notifications.invokeAction(card.n.id, modelData.key)
                }
            }
        }

        Rectangle {
            visible: card.n.hasReply
            width: parent.width
            height: 34
            radius: 17
            color: Qt.rgba(1, 1, 1, 0.1)

            TextInput {
                id: reply
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                verticalAlignment: TextInput.AlignVCenter
                color: Theme.foreground
                font.pixelSize: 13
                clip: true
                onAccepted: Notifications.reply(card.n.id, text)

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: !reply.text && !reply.activeFocus
                    text: card.n.replyPlaceholder || qsTr("Reply…")
                    color: Theme.secondaryForeground
                    font: reply.font
                }
            }
        }
    }
}
