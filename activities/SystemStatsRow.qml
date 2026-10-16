pragma ComponentBehavior: Bound

import QtQuick
import IslandShell

// CPU, GPU, RAM and temperatures as small gauges.
Row {
    id: root

    spacing: 10

    component Gauge: Column {
        id: gauge
        property string label
        property real value: -1   // 0..1, -1 hides the bar
        property real temp: -1    // °C, -1 hides it
        property int hotAt: 0
        spacing: 3
        width: 70
        visible: value >= 0 || temp >= 0

        Row {
            width: parent.width
            Text {
                width: parent.width / 2
                text: gauge.label
                color: Theme.secondaryForeground
                font.pixelSize: 10
                font.weight: Font.DemiBold
            }
            Text {
                width: parent.width / 2
                horizontalAlignment: Text.AlignRight
                visible: gauge.temp >= 0
                text: Math.round(gauge.temp) + "°"
                color: gauge.hotAt > 0 && gauge.temp >= gauge.hotAt ? Theme.danger : Theme.foreground
                font.pixelSize: 10
                font.features: { "tnum": 1 }
            }
        }
        Rectangle {
            visible: gauge.value >= 0
            width: parent.width
            height: 4
            radius: 2
            color: Qt.rgba(1, 1, 1, 0.15)
            Rectangle {
                width: parent.width * Math.max(0, Math.min(1, gauge.value))
                height: parent.height
                radius: parent.radius
                color: gauge.value > 0.9 ? "#ff9f0a" : Theme.foreground
                Behavior on width { NumberAnimation { duration: 300 } }
            }
        }
        Text {
            visible: gauge.value >= 0
            text: Math.round(gauge.value * 100) + "%"
            color: Theme.secondaryForeground
            font.pixelSize: 9
            font.features: { "tnum": 1 }
        }
    }

    Gauge {
        label: "CPU"
        value: SystemStats.cpuUsage
        temp: SystemStats.cpuTemp
        hotAt: Config.get("system.cpu_hot", 90)
    }
    Gauge {
        label: "GPU"
        value: SystemStats.gpuUsage
        temp: SystemStats.gpuTemp
        hotAt: Config.get("system.gpu_hot", 95)
    }
    Gauge {
        label: "VRAM"
        value: SystemStats.vramUsage
    }
    Gauge {
        label: "RAM"
        value: SystemStats.memoryUsage
    }
    Gauge {
        label: "SSD"
        temp: SystemStats.nvmeTemp
        hotAt: Config.get("system.nvme_hot", 70)
    }
}
