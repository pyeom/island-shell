import QtQuick
import IslandShell

// One island. All motion happens inside the fixed-size layer surface; only
// `inputRects` (pill and bubble) follow it.
Item {
    id: root

    property bool demo: false
    property bool focused: false

    // compact | peek | expanded
    property string mode: "compact"
    // Activity shown when expanded; null means the primary one.
    property Activity expandTarget: null

    readonly property Activity primary: arbiter.primary
    readonly property Activity secondary: arbiter.secondary
    readonly property Activity expandedActivity: expandTarget ?? primary
    readonly property bool expanded: mode === "expanded"

    readonly property list<rect> inputRects: bubble.shown ? [pillRect, bubbleRect] : [pillRect]
    readonly property rect pillRect: Qt.rect(pill.x, pill.y, pill.width, pill.height)
    readonly property rect bubbleRect: Qt.rect(bubble.x, bubble.y, bubble.width, bubble.height)

    readonly property bool animate: Config.animations !== "off"
    readonly property bool reduced: Config.animations === "reduced"

    readonly property int padding: 14
    readonly property int bubbleGap: 8

    readonly property size compactSize: Qt.size(
        Math.max(Config.compactWidth, leading.implicitWidth + trailing.implicitWidth + padding * 2 + 24),
        Config.compactHeight)

    readonly property size targetSize: {
        if (mode === "expanded") {
            const item = expandedLoader.item as Item
            const w = item && item.implicitWidth > 0 ? item.implicitWidth : Config.expandedWidth
            const h = item && item.implicitHeight > 0 ? item.implicitHeight : Config.expandedHeight
            return Qt.size(Math.min(w, Config.surfaceWidth),
                           Math.min(h, Config.surfaceHeight - Config.topMargin))
        }
        if (mode === "peek")
            return Qt.size(compactSize.width + Config.peekExtraWidth,
                           compactSize.height + Config.peekExtraHeight)
        return compactSize
    }

    function expand(activity: Activity): bool {
        const target = activity ?? root.primary
        if (!target || !target.expanded)
            return false
        root.expandTarget = activity
        root.mode = "expanded"
        return true
    }

    function collapse() {
        root.mode = "compact"
        holdTimer.stop()
    }

    Arbiter {
        id: arbiter
        focused: root.focused
    }

    Spring {
        id: widthSpring
        target: root.targetSize.width
        animated: root.animate
        response: 0.42
        dampingFraction: root.reduced ? 1.0 : 0.72
    }
    Spring {
        id: heightSpring
        target: root.targetSize.height
        animated: root.animate
        response: 0.42
        dampingFraction: root.reduced ? 1.0 : 0.78
    }
    Spring {
        id: radiusSpring
        target: root.expanded ? Config.expandedRadius : root.targetSize.height / 2
        animated: root.animate
        response: 0.42
        dampingFraction: 0.9
    }
    // Shifts the pill left so pill + bubble stay centered together.
    Spring {
        id: offsetSpring
        target: bubble.shown ? (bubble.width + root.bubbleGap) / 2 : 0
        animated: root.animate
        response: 0.4
        dampingFraction: root.reduced ? 1.0 : 0.8
    }

    Rectangle {
        id: pill

        width: widthSpring.value
        height: heightSpring.value
        radius: Math.min(radiusSpring.value, height / 2)
        x: Math.round((root.width - width) / 2 - offsetSpring.value)
        y: Config.topMargin
        color: Theme.background
        clip: true
        antialiasing: true

        Behavior on color { ColorAnimation { duration: 300 } }

        // Compact and peek: leading and trailing presentations.
        Item {
            anchors.fill: parent
            opacity: root.expanded ? 0 : 1
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: root.animate ? 150 : 0 } }

            Loader {
                id: leading
                anchors.left: parent.left
                anchors.leftMargin: root.padding
                anchors.verticalCenter: parent.verticalCenter
                sourceComponent: root.primary?.compactLeading ?? null
            }
            Loader {
                id: trailing
                anchors.right: parent.right
                anchors.rightMargin: root.padding
                anchors.verticalCenter: parent.verticalCenter
                sourceComponent: root.primary?.compactTrailing ?? null
            }
        }

        // Expanded presentation, at its own implicit size; the pill reveals it.
        Loader {
            id: expandedLoader
            anchors.centerIn: parent
            active: root.expanded || opacity > 0
            sourceComponent: root.expandedActivity?.expanded ?? null
            opacity: root.expanded ? 1 : 0
            scale: root.expanded || root.reduced ? 1 : 0.94
            Behavior on opacity {
                NumberAnimation { duration: root.animate ? 200 : 0; easing.type: Easing.OutCubic }
            }
            Behavior on scale {
                NumberAnimation { duration: root.animate ? 280 : 0; easing.type: Easing.OutCubic }
            }
            // Keep the expanded activity until its content has faded out.
            onActiveChanged: if (!active) root.expandTarget = null
        }

        HoverHandler {
            id: hover
            onHoveredChanged: {
                if (root.demo)
                    return
                if (hovered && root.mode === "compact")
                    root.mode = "peek"
                collapseTimer.restart()
            }
        }

        // Expanded content may contain buttons, so taps there do not collapse.
        TapHandler {
            enabled: !root.expanded && !root.demo
            onTapped: root.expand(null)
        }
    }

    // Secondary activity, iOS-style detached bubble.
    Rectangle {
        id: bubble

        readonly property bool shown: !!root.secondary && !root.expanded
        property Activity activity: null

        width: Config.compactHeight
        height: width
        radius: width / 2
        x: pill.x + pill.width + root.bubbleGap
        y: Config.topMargin
        color: Theme.background
        opacity: shown ? 1 : 0
        scale: shown || root.reduced ? 1 : 0.4
        visible: opacity > 0

        Behavior on opacity { NumberAnimation { duration: root.animate ? 180 : 0 } }
        Behavior on scale {
            NumberAnimation { duration: root.animate ? 320 : 0; easing.type: Easing.OutBack }
        }

        // Keep showing the last activity while fading out.
        onShownChanged: if (shown) activity = root.secondary
        Connections {
            target: root
            function onSecondaryChanged() {
                if (root.secondary)
                    bubble.activity = root.secondary
            }
        }

        Loader {
            anchors.centerIn: parent
            sourceComponent: bubble.activity?.minimal ?? null
        }

        TapHandler {
            enabled: bubble.shown && !root.demo
            onTapped: root.expand(bubble.activity)
        }
    }

    // Leaving the pill collapses it; expanded waits a bit longer so a pointer
    // that slips off the edge does not slam it shut.
    Timer {
        id: collapseTimer
        interval: root.expanded ? 700 : 150
        onTriggered: {
            if (!hover.hovered && !holdTimer.running && !root.demo)
                root.collapse()
        }
    }

    // Expansions not started by the pointer (alerts, islandctl) close on their
    // own unless the pointer is over the island.
    Timer {
        id: holdTimer
        onTriggered: {
            if (!hover.hovered && !root.demo)
                root.collapse()
        }
    }

    function hold(ms: int) {
        holdTimer.interval = ms
        holdTimer.restart()
    }

    Connections {
        target: ActivityRegistry
        function onAlertRequested(activity: Activity, durationMs: int) {
            if (arbiter.eligible(activity) && root.expand(activity))
                root.hold(durationMs)
        }
    }

    Connections {
        target: ShellControl
        function onExpandRequested(name: string) {
            if (!root.focused)
                return
            if (root.expand(name ? ActivityRegistry.find(name) : null))
                root.hold(6000)
        }
        function onCollapseRequested() {
            root.collapse()
        }
    }

    Timer {
        running: root.demo
        repeat: true
        interval: 1500
        property int step: 0
        onTriggered: {
            step = (step + 1) % 4
            if (step === 2)
                root.expand(null)
            else
                root.mode = step === 0 ? "compact" : "peek"
        }
    }
}
