pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import IslandShell

// Settings window (`islandctl settings`). Every control writes one key of
// config.toml through Config.set, which edits that line only, so comments and
// formatting survive; the shell reloads the file as usual.
Window {
    id: win

    width: 900
    height: 620
    minimumWidth: 700
    minimumHeight: 460
    title: qsTr("island-shell settings")
    color: "#141414"

    Material.theme: Material.Dark
    Material.accent: Theme.accent

    // Each option: section.key, label, kind, default, extra.
    // kind: text | int | double | bool | choice | color | list
    readonly property var pages: [
        { title: qsTr("General"), icon: "preferences-system-symbolic", options: [
            { key: "island.fullscreen", label: qsTr("Over fullscreen windows"), kind: "choice", def: "minimal",
              choices: [["minimal", qsTr("Hide clock and media")], ["hide", qsTr("Hide everything but critical")], ["show", qsTr("Always show")]] },
            { key: "animations.mode", label: qsTr("Animations"), kind: "choice", def: "full",
              choices: [["full", qsTr("Springs")], ["reduced", qsTr("Reduced")], ["off", qsTr("Off")]] },
            { key: "clock.format", label: qsTr("Clock format"), kind: "text", def: "HH:mm", hint: "HH:mm · h:mm AP · HH:mm:ss" },
            { key: "island.compact_width", label: qsTr("Pill width"), kind: "int", def: 190, from: 40, to: 2000 },
            { key: "island.compact_height", label: qsTr("Pill height"), kind: "int", def: 36, from: 16, to: 200 },
            { key: "island.top_margin", label: qsTr("Gap above the pill"), kind: "int", def: 6, from: 0, to: 200 },
            { key: "island.exclusive_zone", label: qsTr("Space reserved at the top"), kind: "int", def: 44, from: 0, to: 400 },
        ]},
        { title: qsTr("Appearance"), icon: "preferences-desktop-color", options: [
            { key: "theme.style", label: qsTr("Style"), kind: "choice", def: "black",
              choices: [["black", qsTr("Black")], ["blur", qsTr("Blur")]] },
            { key: "theme.source", label: qsTr("Colors"), kind: "choice", def: "static",
              choices: [["static", qsTr("Fixed accent")], ["pywal", qsTr("From pywal (wallpaper)")]] },
            { key: "theme.accent", label: qsTr("Accent color"), kind: "color", def: "#0a84ff" },
            { key: "theme.pywal_accent", label: qsTr("pywal accent"), kind: "text", def: "color4", hint: "color0 … color15" },
            { key: "theme.blur_opacity", label: qsTr("Blur opacity"), kind: "double", def: 0.6 },
            { key: "theme.icon_theme", label: qsTr("Icon theme"), kind: "text", def: "", hint: qsTr("empty: system") },
        ]},
        { title: qsTr("Notifications"), icon: "notifications-symbolic", options: [
            { key: "notifications.timeout", label: qsTr("Popup duration (ms)"), kind: "int", def: 5000, from: 500, to: 600000, step: 500 },
            { key: "notifications.history_limit", label: qsTr("History size"), kind: "int", def: 100, from: 0, to: 10000 },
        ], note: qsTr("Per-app rules (mute, history only, urgency) are edited in config.toml.") },
        { title: qsTr("Launcher"), icon: "system-search-symbolic", options: [
            { key: "launcher.terminal", label: qsTr("Terminal"), kind: "text", def: "", hint: qsTr("empty: $TERMINAL, kitty, alacritty…") },
            { key: "launcher.launch_prefix", label: qsTr("Launch prefix"), kind: "text", def: "", hint: "uwsm app --" },
            { key: "launcher.search_url", label: qsTr("Web search"), kind: "text", def: "https://duckduckgo.com/?q=%s", hint: "%s = query" },
        ]},
        { title: qsTr("Calendar"), icon: "x-office-calendar-symbolic", options: [
            { key: "calendar.paths", label: qsTr("Calendar folders"), kind: "list", def: ["~/.calendars", "~/.local/share/calendars"] },
            { key: "calendar.default", label: qsTr("New events go to"), kind: "text", def: "", hint: qsTr("empty: first calendar") },
            { key: "calendar.reminder_minutes", label: qsTr("Remind before (minutes)"), kind: "int", def: 10, from: 0, to: 1440 },
            { key: "calendar.days", label: qsTr("Days ahead"), kind: "int", def: 60, from: 1, to: 730 },
        ]},
        { title: qsTr("Weather"), icon: "weather-few-clouds-symbolic", options: [
            { key: "weather.city", label: qsTr("City"), kind: "text", def: "", hint: qsTr("empty: no weather") },
            { key: "weather.units", label: qsTr("Units"), kind: "choice", def: "metric",
              choices: [["metric", "°C"], ["imperial", "°F"]] },
            { key: "weather.rain_alert", label: qsTr("Rain alert (%)"), kind: "int", def: 60, from: 0, to: 100, step: 5 },
        ]},
        { title: qsTr("System"), icon: "cpu", options: [
            { key: "system.cpu_hot", label: qsTr("CPU alert (°C)"), kind: "int", def: 90, from: 0, to: 150 },
            { key: "system.gpu_hot", label: qsTr("GPU alert (°C)"), kind: "int", def: 95, from: 0, to: 150 },
            { key: "system.nvme_hot", label: qsTr("SSD alert (°C)"), kind: "int", def: 70, from: 0, to: 150 },
        ]},
        { title: qsTr("Screenshots && wallpaper"), icon: "camera-photo-symbolic", options: [
            { key: "screenshots.directory", label: qsTr("Screenshot folder"), kind: "text", def: "~/Pictures/Screenshots" },
            { key: "screenshots.editor", label: qsTr("Editor"), kind: "text", def: "", hint: "satty --filename" },
            { key: "wallpaper.directory", label: qsTr("Wallpaper folder"), kind: "text", def: "~/.config/wallpapers" },
            { key: "wallpaper.apply", label: qsTr("Apply command"), kind: "text", def: "", hint: qsTr("%f = file; empty: waypaper + wal") },
            { key: "night_light.command", label: qsTr("Night light command"), kind: "text", def: "", hint: qsTr("empty: reuse wlsunset's") },
        ]},
        { title: qsTr("Power"), icon: "system-shutdown-symbolic", options: [
            { key: "power.lock_command", label: qsTr("Lock command"), kind: "text", def: "hyprlock" },
            { key: "power.hold_ms", label: qsTr("Hold to confirm (ms)"), kind: "int", def: 800, from: 0, to: 5000, step: 100 },
        ]},
    ]
    property int page: 0

    function value(option: var): var {
        const v = Config.get(option.key, option.def)
        return v === undefined ? option.def : v
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Sections.
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 260
            color: "#0e0e0e"

            ListView {
                anchors.fill: parent
                anchors.topMargin: 16
                model: win.pages
                spacing: 2
                delegate: ItemDelegate {
                    id: entry
                    required property var modelData
                    required property int index
                    width: ListView.view.width
                    highlighted: index === win.page
                    icon.source: Icons.source(modelData.icon, "preferences-system")
                    icon.color: "transparent" // keep the theme's own icon colors
                    text: modelData.title
                    onClicked: win.page = index
                }
            }
        }

        // Options of the selected section.
        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: content.implicitHeight + 48
            clip: true

            ColumnLayout {
                id: content
                x: 32
                y: 24
                width: parent.width - 64
                spacing: 18

                Label {
                    text: win.pages[win.page].title
                    font.pixelSize: 22
                    font.weight: Font.DemiBold
                }

                Label {
                    Layout.fillWidth: true
                    visible: Config.errors.length > 0
                    wrapMode: Text.Wrap
                    color: Theme.danger
                    text: Config.errors.join("\n")
                }

                Repeater {
                    model: win.pages[win.page].options
                    delegate: RowLayout {
                        id: row
                        required property var modelData
                        readonly property var current: Config.revision >= 0 ? win.value(modelData) : undefined
                        Layout.fillWidth: true
                        spacing: 16

                        ColumnLayout {
                            Layout.preferredWidth: 240
                            Layout.maximumWidth: 240
                            spacing: 2
                            Label {
                                Layout.fillWidth: true
                                text: row.modelData.label
                                wrapMode: Text.Wrap
                                font.pixelSize: 14
                            }
                            Label {
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                                visible: !!row.modelData.hint
                                text: row.modelData.hint ?? ""
                                opacity: 0.6
                                font.pixelSize: 11
                            }
                        }

                        Loader {
                            Layout.fillWidth: true
                            sourceComponent: row.modelData.kind === "int" ? intEditor
                                           : row.modelData.kind === "double" ? doubleEditor
                                           : row.modelData.kind === "choice" ? choiceEditor
                                           : row.modelData.kind === "color" ? colorEditor
                                           : row.modelData.kind === "list" ? listEditor
                                           : textEditor

                            Component {
                                id: textEditor
                                TextField {
                                    text: row.current
                                    placeholderText: row.modelData.hint ?? ""
                                    selectByMouse: true
                                    onEditingFinished: if (text !== row.current) Config.set(row.modelData.key, text)
                                }
                            }
                            Component {
                                id: intEditor
                                SpinBox {
                                    from: row.modelData.from ?? 0
                                    to: row.modelData.to ?? 100000
                                    stepSize: row.modelData.step ?? 1
                                    editable: true
                                    value: row.current
                                    onValueModified: Config.set(row.modelData.key, value)
                                }
                            }
                            Component {
                                id: doubleEditor
                                RowLayout {
                                    Slider {
                                        id: slider
                                        Layout.fillWidth: true
                                        from: 0
                                        to: 1
                                        value: row.current
                                        onMoved: Config.set(row.modelData.key, Math.round(value * 100) / 100)
                                    }
                                    Label { text: Math.round(slider.value * 100) + "%" }
                                }
                            }
                            Component {
                                id: choiceEditor
                                ComboBox {
                                    model: row.modelData.choices.map(c => c[1])
                                    currentIndex: Math.max(0, row.modelData.choices.findIndex(c => c[0] === row.current))
                                    onActivated: index => Config.set(row.modelData.key, row.modelData.choices[index][0])
                                }
                            }
                            Component {
                                id: colorEditor
                                RowLayout {
                                    Rectangle {
                                        Layout.preferredWidth: 28
                                        Layout.preferredHeight: 28
                                        radius: 14
                                        color: row.current
                                    }
                                    TextField {
                                        Layout.fillWidth: true
                                        text: row.current
                                        validator: RegularExpressionValidator { regularExpression: /#[0-9a-fA-F]{6}/ }
                                        onEditingFinished: Config.set(row.modelData.key, text)
                                    }
                                }
                            }
                            Component {
                                id: listEditor
                                TextField {
                                    text: (row.current || []).join(", ")
                                    placeholderText: qsTr("comma separated")
                                    selectByMouse: true
                                    onEditingFinished: Config.set(row.modelData.key,
                                        text.split(",").map(s => s.trim()).filter(s => s.length > 0))
                                }
                            }
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    visible: !!win.pages[win.page].note
                    wrapMode: Text.Wrap
                    opacity: 0.7
                    text: win.pages[win.page].note ?? ""
                }

                RowLayout {
                    spacing: 10
                    Button {
                        text: qsTr("Open config.toml")
                        onClicked: Apps.openUrl("file://" + Config.path)
                    }
                    Label {
                        opacity: 0.6
                        font.pixelSize: 11
                        text: Config.path
                    }
                }
            }
        }
    }
}
