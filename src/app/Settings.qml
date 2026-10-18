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

    // Each page holds groups, each group its options: section.key, label,
    // kind, default, extra.
    // kind: text | int | double | bool | choice | color | list | key
    readonly property var pages: [
        { title: qsTr("General"), icon: "preferences-system-symbolic", groups: [
            { title: qsTr("Behavior"), options: [
                { key: "island.fullscreen", label: qsTr("Over fullscreen windows"), kind: "choice", def: "minimal",
                  choices: [["minimal", qsTr("Hide clock and media")], ["hide", qsTr("Hide everything but critical")], ["show", qsTr("Always show")]] },
                { key: "animations.mode", label: qsTr("Animations"), kind: "choice", def: "full",
                  choices: [["full", qsTr("Springs")], ["reduced", qsTr("Reduced")], ["off", qsTr("Off")]] },
            ]},
            { title: qsTr("Clock"), options: [
                { key: "clock.format", label: qsTr("Clock format"), kind: "text", def: "HH:mm", hint: "HH:mm · h:mm AP · HH:mm:ss" },
            ]},
            { title: qsTr("Size and position"), options: [
                { key: "island.compact_width", label: qsTr("Pill width"), kind: "int", def: 190, from: 40, to: 2000 },
                { key: "island.compact_height", label: qsTr("Pill height"), kind: "int", def: 36, from: 16, to: 200 },
                { key: "island.top_margin", label: qsTr("Gap above the pill"), kind: "int", def: 6, from: 0, to: 200 },
                { key: "island.exclusive_zone", label: qsTr("Space reserved at the top"), kind: "int", def: 44, from: 0, to: 400 },
            ]},
        ]},
        { title: qsTr("Appearance"), icon: "preferences-desktop-color", groups: [
            { title: qsTr("Background"), options: [
                { key: "theme.style", label: qsTr("Style"), kind: "choice", def: "black",
                  choices: [["black", qsTr("Black")], ["blur", qsTr("Blur")]] },
                { key: "theme.blur_opacity", label: qsTr("Blur opacity"), kind: "double", def: 0.6 },
            ]},
            { title: qsTr("Colors"), options: [
                { key: "theme.source", label: qsTr("Colors"), kind: "choice", def: "static",
                  choices: [["static", qsTr("Fixed accent")], ["pywal", qsTr("From pywal (wallpaper)")]] },
                { key: "theme.accent", label: qsTr("Accent color"), kind: "color", def: "#0a84ff" },
                { key: "theme.pywal_accent", label: qsTr("pywal accent"), kind: "text", def: "color4", hint: "color0 … color15" },
            ]},
            { title: qsTr("Icons"), options: [
                { key: "theme.icon_theme", label: qsTr("Icon theme"), kind: "text", def: "", hint: qsTr("empty: system") },
            ]},
        ]},
        { title: qsTr("Notifications"), icon: "notifications-symbolic", groups: [
            { title: qsTr("Popups"), options: [
                { key: "notifications.timeout", label: qsTr("Popup duration (ms)"), kind: "int", def: 5000, from: 500, to: 600000, step: 500 },
            ]},
            { title: qsTr("History"), options: [
                { key: "notifications.history_limit", label: qsTr("History size"), kind: "int", def: 100, from: 0, to: 10000 },
            ]},
        ], note: qsTr("Per-app rules (mute, history only, urgency) are edited in config.toml.") },
        { title: qsTr("Launcher"), icon: "system-search-symbolic", groups: [
            { title: qsTr("Apps and commands"), options: [
                { key: "launcher.terminal", label: qsTr("Terminal"), kind: "text", def: "", hint: qsTr("empty: $TERMINAL, kitty, alacritty…") },
                { key: "launcher.launch_prefix", label: qsTr("Launch prefix"), kind: "text", def: "", hint: "uwsm app --" },
            ]},
            { title: qsTr("Web search"), options: [
                { key: "launcher.search_engine", label: qsTr("Search engine"), kind: "choice", def: "duckduckgo",
                  choices: [["duckduckgo", "DuckDuckGo"], ["google", "Google"], ["bing", "Bing"], ["brave", "Brave Search"],
                            ["startpage", "Startpage"], ["ecosia", "Ecosia"], ["kagi", "Kagi"], ["custom", qsTr("Custom URL")]] },
                { key: "launcher.search_url", label: qsTr("Custom search URL"), kind: "text", def: "",
                  hint: qsTr("%s = query; with Custom URL") },
            ]},
        ]},
        { title: qsTr("Calendar"), icon: "x-office-calendar-symbolic", groups: [
            { title: qsTr("Calendars"), options: [
                { key: "calendar.paths", label: qsTr("Calendar folders"), kind: "list", def: ["~/.calendars", "~/.local/share/calendars"] },
                { key: "calendar.default", label: qsTr("New events go to"), kind: "text", def: "", hint: qsTr("empty: first calendar") },
            ]},
            { title: qsTr("Events"), options: [
                { key: "calendar.reminder_minutes", label: qsTr("Remind before (minutes)"), kind: "int", def: 10, from: 0, to: 1440 },
                { key: "calendar.days", label: qsTr("Days ahead"), kind: "int", def: 60, from: 1, to: 730 },
            ]},
        ]},
        { title: qsTr("Weather"), icon: "weather-few-clouds-symbolic", groups: [
            { title: qsTr("Forecast"), options: [
                { key: "weather.city", label: qsTr("City"), kind: "text", def: "", hint: qsTr("empty: no weather") },
                { key: "weather.units", label: qsTr("Units"), kind: "choice", def: "metric",
                  choices: [["metric", "°C"], ["imperial", "°F"]] },
            ]},
            { title: qsTr("Alerts"), options: [
                { key: "weather.rain_alert", label: qsTr("Rain alert (%)"), kind: "int", def: 60, from: 0, to: 100, step: 5 },
            ]},
        ]},
        { title: qsTr("System"), icon: "cpu", groups: [
            { title: qsTr("Temperature alerts"), options: [
                { key: "system.cpu_hot", label: qsTr("CPU alert (°C)"), kind: "int", def: 90, from: 0, to: 150 },
                { key: "system.gpu_hot", label: qsTr("GPU alert (°C)"), kind: "int", def: 95, from: 0, to: 150 },
                { key: "system.nvme_hot", label: qsTr("SSD alert (°C)"), kind: "int", def: 70, from: 0, to: 150 },
            ]},
        ]},
        { title: qsTr("Screenshots && wallpaper"), icon: "camera-photo-symbolic", groups: [
            { title: qsTr("Screenshots"), options: [
                { key: "screenshots.directory", label: qsTr("Screenshot folder"), kind: "text", def: "~/Pictures/Screenshots" },
                { key: "screenshots.editor", label: qsTr("Editor"), kind: "text", def: "", hint: "satty --filename" },
            ]},
            { title: qsTr("Wallpaper"), options: [
                { key: "wallpaper.directory", label: qsTr("Wallpaper folder"), kind: "text", def: "~/.config/wallpapers" },
                { key: "wallpaper.backend", label: qsTr("Wallpaper backend"), kind: "choice", def: "auto",
                  hint: qsTr("In use: %1").arg(Wallpapers.backend),
                  choices: [["auto", qsTr("Automatic")], ["awww", "awww"], ["swww", "swww"], ["hyprpaper", "hyprpaper"],
                            ["swaybg", "swaybg"], ["waypaper", "waypaper"], ["command", qsTr("Custom command")]] },
                { key: "wallpaper.backend_args", label: qsTr("Backend arguments"), kind: "text", def: "",
                  hint: "--transition-type grow" },
                { key: "wallpaper.apply", label: qsTr("Custom command"), kind: "text", def: "", hint: qsTr("%f = file") },
                { key: "wallpaper.pywal", label: qsTr("Run pywal after changing it"), kind: "bool", def: true },
            ]},
            { title: qsTr("Night light"), options: [
                { key: "night_light.command", label: qsTr("Night light command"), kind: "text", def: "", hint: qsTr("empty: reuse wlsunset's") },
            ]},
        ]},
        { title: qsTr("Privacy"), icon: "security-high-symbolic", groups: [
            { title: qsTr("Indicator"), note: qsTr("The privacy indicator shows which apps use these devices."), options: [
                { key: "privacy.microphone", label: qsTr("Show microphone use"), kind: "bool", def: true },
                { key: "privacy.camera", label: qsTr("Show camera use"), kind: "bool", def: true },
                { key: "privacy.screen", label: qsTr("Show screen sharing"), kind: "bool", def: true },
            ]},
        ]},
        { title: qsTr("Live activities"), icon: "folder-download-symbolic", groups: [
            { title: qsTr("Downloads"), options: [
                { key: "live.downloads", label: qsTr("Show browser downloads"), kind: "bool", def: true },
                { key: "live.downloads_directory", label: qsTr("Downloads folder"), kind: "text", def: "",
                  hint: qsTr("empty: your downloads folder") },
            ]},
            { title: qsTr("Apps"), options: [
                { key: "live.app_progress", label: qsTr("Show progress apps report"), kind: "bool", def: true,
                  hint: qsTr("Chromium downloads, file copies…") },
            ]},
        ], note: qsTr("island-dl URL downloads a file with its progress in the island; scripts can show their own with islandctl push.") },
        { title: qsTr("Priorities"), icon: "view-sort-descending-symbolic", groups: [
            { title: qsTr("Ongoing"), options: [
                { key: "priorities.privacy", label: qsTr("Privacy indicator"), kind: "int", def: 60, from: 0, to: 99 },
                { key: "priorities.event", label: qsTr("Upcoming event"), kind: "int", def: 55, from: 0, to: 99 },
                { key: "priorities.live", label: qsTr("Live activities"), kind: "int", def: 50, from: 0, to: 99 },
                { key: "priorities.media", label: qsTr("Media"), kind: "int", def: 40, from: 0, to: 99 },
            ]},
            { title: qsTr("Alerts"), options: [
                { key: "priorities.errors", label: qsTr("Shell errors"), kind: "int", def: 90, from: 0, to: 99 },
                { key: "priorities.system-heat", label: qsTr("Overheating"), kind: "int", def: 85, from: 0, to: 99 },
                { key: "priorities.battery-low", label: qsTr("Low battery"), kind: "int", def: 82, from: 0, to: 99 },
                { key: "priorities.notification", label: qsTr("Notifications"), kind: "int", def: 70, from: 0, to: 99 },
                { key: "priorities.rain", label: qsTr("Rain alert"), kind: "int", def: 45, from: 0, to: 99 },
            ]},
            { title: qsTr("Feedback"), options: [
                { key: "priorities.osd", label: qsTr("Volume and brightness"), kind: "int", def: 80, from: 0, to: 99 },
                { key: "priorities.screenshot", label: qsTr("Screenshot"), kind: "int", def: 75, from: 0, to: 99 },
            ]},
        ], note: qsTr("The highest priority takes the pill and the next one the bubble beside it. Critical notifications and menus always come first.") },
        { title: qsTr("Keybindings"), icon: "preferences-desktop-keyboard-shortcuts", groups: [
            { title: qsTr("Menus"), options: [
                { key: "keybinds.launcher", label: qsTr("Launcher"), kind: "key" },
                { key: "keybinds.quick_settings", label: qsTr("Quick settings"), kind: "key" },
                { key: "keybinds.media", label: qsTr("Media player"), kind: "key" },
                { key: "keybinds.power", label: qsTr("Power menu"), kind: "key" },
                { key: "keybinds.settings", label: qsTr("Settings"), kind: "key" },
            ]},
            { title: qsTr("Island"), options: [
                { key: "keybinds.expand", label: qsTr("Expand the island"), kind: "key" },
                { key: "keybinds.collapse", label: qsTr("Collapse the island"), kind: "key" },
                { key: "keybinds.reload", label: qsTr("Reload the island"), kind: "key" },
            ]},
            { title: qsTr("Screenshots"), options: [
                { key: "keybinds.screenshot_region", label: qsTr("Screenshot of a region"), kind: "key" },
                { key: "keybinds.screenshot_window", label: qsTr("Screenshot of a window"), kind: "key" },
                { key: "keybinds.screenshot_output", label: qsTr("Screenshot of the screen"), kind: "key" },
            ]},
            { title: qsTr("Wallpaper"), options: [
                { key: "keybinds.wallpaper", label: qsTr("Wallpaper picker"), kind: "key" },
                { key: "keybinds.wallpaper_random", label: qsTr("Random wallpaper"), kind: "key" },
            ]},
            { title: qsTr("Media and volume"), options: [
                { key: "keybinds.media_play_pause", label: qsTr("Play or pause"), kind: "key", hint: "XF86AudioPlay" },
                { key: "keybinds.media_next", label: qsTr("Next track"), kind: "key", hint: "XF86AudioNext" },
                { key: "keybinds.media_previous", label: qsTr("Previous track"), kind: "key", hint: "XF86AudioPrev" },
                { key: "keybinds.volume_up", label: qsTr("Volume up"), kind: "key", hint: "XF86AudioRaiseVolume" },
                { key: "keybinds.volume_down", label: qsTr("Volume down"), kind: "key", hint: "XF86AudioLowerVolume" },
                { key: "keybinds.volume_mute", label: qsTr("Mute"), kind: "key", hint: "XF86AudioMute" },
                { key: "keybinds.mic_mute", label: qsTr("Mute the microphone"), kind: "key", hint: "XF86AudioMicMute" },
            ]},
            { title: qsTr("Notifications"), options: [
                { key: "keybinds.dnd", label: qsTr("Do not disturb"), kind: "key" },
                { key: "keybinds.notifications_clear", label: qsTr("Clear notifications"), kind: "key" },
            ]},
        ], note: qsTr("The island adds these binds to Hyprland while it runs; any other bind on the same keys is replaced. Recording pauses Hyprland's binds until you press a key, or Esc to cancel.") },
        { title: qsTr("Power"), icon: "system-shutdown-symbolic", groups: [
            { title: qsTr("Lock"), options: [
                { key: "power.lock_command", label: qsTr("Lock command"), kind: "text", def: "hyprlock" },
            ]},
            { title: qsTr("Power menu"), options: [
                { key: "power.hold_ms", label: qsTr("Hold to confirm (ms)"), kind: "int", def: 800, from: 0, to: 5000, step: 100 },
            ]},
        ]},
    ]
    property int page: 0
    // Config key of the keybinding being recorded, if any.
    property string recordingKey: ""

    onPageChanged: Keybinds.stopRecording()
    onVisibleChanged: if (!visible) Keybinds.stopRecording()

    function value(option: var): var {
        const v = Config.get(option.key, option.def)
        return v === undefined ? option.def : v
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Pages.
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

        // Groups of options of the selected page.
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
                spacing: 22

                Label {
                    text: win.pages[win.page].title.replace("&&", "&") // && is for the sidebar's mnemonics
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

                Label {
                    Layout.fillWidth: true
                    readonly property bool keysPage: win.pages[win.page].groups.some(g => g.options.some(o => o.kind === "key"))
                    visible: keysPage && (!Keybinds.available || Keybinds.errors.length > 0)
                    wrapMode: Text.Wrap
                    color: Theme.danger
                    text: !Keybinds.available ? qsTr("Hyprland is not running: these keys are not bound.")
                                              : qsTr("Hyprland refused some binds:") + "\n" + Keybinds.errors.join("\n")
                }

                // One card per group.
                Repeater {
                    model: win.pages[win.page].groups
                    delegate: ColumnLayout {
                        id: group
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 8

                        Label {
                            text: group.modelData.title
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            opacity: 0.7
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: rows.implicitHeight + 8
                            radius: 12
                            color: "#1c1c1c"

                            ColumnLayout {
                                id: rows
                                x: 16
                                y: 4
                                width: parent.width - 32
                                spacing: 0

                                Repeater {
                                    model: group.modelData.options
                                    delegate: ColumnLayout {
                                        id: row
                                        required property var modelData
                                        required property int index
                                        readonly property var current: Config.revision >= 0 ? win.value(modelData) : undefined
                                        readonly property string hint: modelData.kind === "key"
                                            && Keybinds.replaced.includes(modelData.key.split(".")[1])
                                            ? qsTr("Replaces a bind from your Hyprland config") : (modelData.hint ?? "")
                                        Layout.fillWidth: true
                                        spacing: 0

                                        Rectangle {
                                            Layout.fillWidth: true
                                            implicitHeight: 1
                                            visible: row.index > 0
                                            color: "#2a2a2a"
                                        }

                                        RowLayout {
                                            Layout.fillWidth: true
                                            Layout.topMargin: 10
                                            Layout.bottomMargin: 10
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
                                                    visible: row.hint !== ""
                                                    text: row.hint
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
                                                               : row.modelData.kind === "bool" ? boolEditor
                                                               : row.modelData.kind === "key" ? keyEditor
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
                                                    id: boolEditor
                                                    Item {
                                                        implicitHeight: toggle.implicitHeight
                                                        Switch {
                                                            id: toggle
                                                            checked: row.current
                                                            onToggled: Config.set(row.modelData.key, checked)
                                                        }
                                                    }
                                                }
                                                Component {
                                                    id: keyEditor
                                                    RowLayout {
                                                        readonly property bool recording: Keybinds.recording && win.recordingKey === row.modelData.key
                                                        spacing: 6
                                                        TextField {
                                                            id: keys
                                                            Layout.fillWidth: true
                                                            text: parent.recording ? qsTr("Press the keys…") : (row.current ?? "")
                                                            readOnly: parent.recording
                                                            placeholderText: text === "" ? qsTr("Not set") : ""
                                                            selectByMouse: true
                                                            onEditingFinished: if (!parent.recording && text.trim() !== (row.current ?? ""))
                                                                                   Config.set(row.modelData.key, text.trim())
                                                            Keys.onPressed: event => {
                                                                if (!parent.recording)
                                                                    return
                                                                event.accepted = true
                                                                const combo = Keybinds.combo(event.modifiers, event.nativeScanCode)
                                                                if (combo === "")
                                                                    return // a modifier on its own: wait for the key
                                                                Keybinds.stopRecording()
                                                                Config.set(row.modelData.key, combo)
                                                            }
                                                        }
                                                        Button {
                                                            text: parent.recording ? qsTr("Cancel") : qsTr("Record")
                                                            onClicked: {
                                                                if (parent.recording) {
                                                                    Keybinds.stopRecording()
                                                                    return
                                                                }
                                                                win.recordingKey = row.modelData.key
                                                                keys.forceActiveFocus()
                                                                Keybinds.startRecording()
                                                            }
                                                        }
                                                        ToolButton {
                                                            icon.source: Icons.source("edit-clear-symbolic", "edit-clear")
                                                            enabled: (row.current ?? "") !== ""
                                                            onClicked: Config.set(row.modelData.key, "")
                                                            ToolTip.visible: hovered
                                                            ToolTip.text: qsTr("Remove")
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
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: !!group.modelData.note
                            wrapMode: Text.Wrap
                            opacity: 0.6
                            font.pixelSize: 12
                            text: group.modelData.note ?? ""
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
