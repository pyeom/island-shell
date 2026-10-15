import QtQuick

// Square icon from Icons.source(): theme name, path or URL.
//   IconImage { source: Icons.source("audio-volume-high-symbolic"); size: 18 }
Image {
    property int size: 20

    width: size
    height: size
    sourceSize: Qt.size(size * 2, size * 2)
    fillMode: Image.PreserveAspectFit
    asynchronous: true
    smooth: true
}
