# Writing activities

Everything the island shows is an **activity**: the clock, notifications, media,
the volume OSD, live activities, the launcher and the power menu. The built-in
ones (in [`activities/`](../activities/)) use the same API described here.

> Don't name an activity file like a singleton (`Privacy.qml`, `Calendar.qml`,
> ...): inside that file the name would refer to the file, not the service.

> The API is `import IslandShell 0.x` and may change between minor versions
> until 1.0. Breaking changes are listed in `CHANGELOG.md`.

## Where they live

Every `*.qml` file in `~/.config/island-shell/activities/` is loaded as an
activity. Saving a file reloads all user activities; no restart needed. Helper
components go in subdirectories and are imported from the activity file.

A file that fails to load, or a binding that throws at runtime, is reported in
the island (the `errors` activity) and the rest keep working.

> Activities are code. They run with the same permissions as the shell. Only
> install activities you trust.

Start from [`examples/activities/Pomodoro.qml`](../examples/activities/Pomodoro.qml).

## Anatomy

```qml
import QtQuick
import IslandShell

Activity {
    name: "pomodoro"        // used by `islandctl expand pomodoro`
    priority: 40            // higher wins the pill
    active: running         // only competes for the island while true
    scope: Activity.FocusedScreen   // or Activity.AllScreens
    modal: false            // see "Modal activities"
    ambient: false          // background info, hidden over fullscreen windows

    compactLeading:  Component { ... }  // left side of the pill
    compactTrailing: Component { ... }  // right side of the pill
    minimal:         Component { ... }  // the detached bubble
    expanded:        Component { ... }  // the expanded view
}
```

Presentations are `Component`s because each monitor's island creates its own
instance. Any of them may be omitted:

- No `expanded`: clicking the pill does nothing for this activity.
- No `minimal`: the activity is never shown as the secondary bubble.

### Choosing what is shown

For each island, among activities that are `active` and allowed on that monitor
(`AllScreens`, or `FocusedScreen` on the focused monitor):

1. The highest `priority` is the **primary**: it owns the pill.
2. The next highest that has a `minimal` presentation is the **secondary**,
   shown as a bubble next to the pill. Clicking it expands it.
3. Ties go to the activity registered first.

Built-in priorities:

| Activity | Priority |
|---|---|
| `launcher`, `power`, `quicksettings` (modal) | 1000 |
| `notification` (critical) | 100 |
| `errors` | 90 |
| `osd` (volume, brightness) | 80 |
| `notification` | 70 |
| `privacy` (mic, camera, screen) | 60 |
| `event` (calendar event starting soon) | 55 |
| `timer` (ringing) | 95 |
| `timer` | 52 |
| `live:<id>` | 50 |
| `media` | 40 |
| `idle` (workspaces, clock, notification history) | 0 |

### Sizes

The compact pill grows to fit `compactLeading` + `compactTrailing`. The expanded
pill takes the `implicitWidth`/`implicitHeight` of the `expanded` item, capped by
`island.surface_width`/`surface_height`. Items are laid out at their implicit
size and revealed by the pill as it grows, so give them a fixed implicit size.

### Alerts

`alert(ms)` briefly expands the island to show this activity, for example when a
timer ends. It collapses after `ms` unless the pointer is over it.

### Modal activities

`modal: true` activities open expanded as soon as they become active, take the
keyboard, and stay until they turn inactive. Only one is open at a time.
Clicking outside the island, `islandctl collapse` or another modal opening
calls their `dismissRequested` signal:

```qml
Activity {
    name: "picker"; priority: 1000; modal: true
    property bool open: false
    active: open
    onDismissRequested: open = false
    expanded: Component { MyPicker { onDone: root.open = false } }
}
```

## Available API

| Name | What it is |
|---|---|
| `Activity` | The type above. |
| `Config` | Settings. `Config.get("activities.pomodoro.minutes", 25)` reads any key; also `Config.clockFormat`, sizes. |
| `Theme` | `background`, `foreground`, `secondaryForeground`, `accent`, `danger`, `blur`. |
| `Clock.now`, `Clock.minute` | A `date` that updates every second; the same without seconds, changing once a minute (bind to it unless you show seconds). `Clock.formatHasSeconds`. |
| `Hyprland` | `monitors`, `workspaces`, `focusedMonitor`, `dispatch("workspace 3")`, `event(name, data)` signal. |
| `Keybinds` | `available`, `replaced`, `errors`, `recording`, `startRecording()`, `stopRecording()`, `combo(modifiers, nativeScanCode)`. |
| `IslandScreen` | Attached: `IslandScreen.name`, `IslandScreen.focused` for the monitor a presentation is drawn on. |
| `ShellControl` | `dnd`, the requests coming from `islandctl`, `showSettingsPage(page, args)` (`"calendar"`, `{ uid, start }` to show an event). |
| `ActivityRegistry` | `activities`, `find(name)`. |
| `Spring` | Spring animation: `Spring { id: s; target: x }` then use `s.value`. |
| `Notifications` | `current` popup, `history` model, `invokeAction(id, key)`, `reply(id, text)`, `dismiss(id)`, `clearHistory()`. |
| `Audio` | Output `volume` (1.0 = 100%), `muted`, `sinks`; microphone `micVolume`, `micMuted`, `sources`; apps playing audio `streams` (`AudioStream`: `id`, `name`, `title`, `iconName`, `volume`, `muted`); `setDefault(id)`, `setVolume`, `setMicMuted`, `setStreamVolume(id, v)`, `setStreamMuted(id, b)`...; `changed` / `micMuteChanged` signals. |
| `Brightness` | Backlight `level` 0..1, `available`, `changed` signal. |
| `Mpris` | `players`, `current` (title, artist, artUrl, position, length, playPause(), next(), previous(), seekTo(s)). |
| `LiveActivities` | Model of pushed live activities; `count`, `invoke(id, key)`. `Config.liveEnabled` says whether they show. |
| `Tray` | Model of tray items (`key`, `title`, `tooltip`, `status`, `shown`, `icon`, `badge`, `attention`, `hasMenu`, `service`, `menuPath`); `count`, `attentionCount`, `available`, `activate(key)`, `secondaryActivate(key)`, `contextMenu(key)`, `scroll(key, delta, horizontal)`, `item(key)`, `showMenu(key)`; `activated(key)`, `menuRequested(key)` and `openRequested(key)` signals. `Config.trayAlerts`. |
| `TrayMenu` | A tray item's menu: `TrayMenu { service: ...; path: ... }`, then `entries` (`id`, `label`, `enabled`, `separator`, `toggle`, `checked`, `icon`, `submenu`, `children`), `loading`, `aboutToShow(id)`, `activate(id)`. |
| `Apps` | `search(query)`, `launch(id, action)`, `run(command)`, `runInTerminal`, `openUrl`, `copyText`, `iconForClass(windowClass)`. |
| `LauncherSearch` | `query(text)` over apps, windows, calculator, commands, web, emoji and timers; `activate(item, action)`. |
| `Timers` | `timers` (`id`, `label`, `duration`, `endsAt`, `done`), `start(seconds, label)`, `extend(id, seconds)`, `remove(id)`, `sounds`, `playSound(path, volume)`, `finished(id, label)` signal. |
| `Power` | `run("shutdown" \| "reboot" \| "suspend" \| "logout" \| "lock")`. |
| `Icons` | `Icons.source(nameOrPath, fallback)` → an `Image` source from the icon theme. |
| `Network` | `connected`, `primaryType`, `ssid`, `strength`, `iconName`, `wifiEnabled`, `setWifiEnabled(b)`. |
| `Bluetooth` | `available`, `powered`, `connectedDevices`, `setPowered(b)`. |
| `PowerProfiles` | `active`, `profiles`, `setActive(name)`. |
| `Updates` | Pending pacman/AUR updates: `available`, `manager`, `count`, `aurCount`, `packages`, `checking`, `checked`, `failed`, `check()`, `upgrade()` (in the terminal). |
| `DefaultApps` | `categories` (`key`, `current`, `candidates`), `setDefault(key, desktopId)`, `refresh()`; through xdg-mime. |
| `Privacy` | `microphone`, `microphoneApps`, `camera`, `cameraApps`, `screen`, `any`. |
| `Calendar` | `eventsOn(date)`, `eventsBetween(from, to)`, `findEvent(uid, start)`, `upcoming(n)`, `busyDays(year, month)`, `soon`, `revision` (bind to it to refresh). |
| `IconImage`, `IconButton`, `ProgressRing`, `IslandSlider`, `RoundedImage` | Small components used by the built-in activities. |

## islandctl

```
islandctl expand [activity]   expand (the current activity if omitted)
islandctl collapse
islandctl launcher toggle
islandctl power
islandctl quick               quick settings (also: right click on the island)
islandctl settings [calendar] the settings window, optionally on a page
islandctl activities          list names
islandctl reload              reload config, theme and activities
islandctl dnd on|off|toggle
islandctl notifications [list|clear|dismiss ID]
islandctl push --id ID [--progress 0.4] [--state running|success|error] \
               [--subtitle S] [--icon I] [--ttl SEC] [--action "Label=cmd"] TITLE
islandctl update --id ID [same options]
islandctl end ID
islandctl live
```

## Live activities from other programs

Anything on the session bus can push live activities without `islandctl`; see
[`data/dbus/org.islandshell.LiveActivities.xml`](../data/dbus/org.islandshell.LiveActivities.xml).
They end when the caller disconnects unless pushed with `persist = true`, and
action clicks come back as the `ActionInvoked(id, key)` signal.

```sh
busctl --user call org.islandshell.Shell /org/islandshell/LiveActivities \
    org.islandshell.LiveActivities Push 'sa{sv}' upload 2 \
    title s "Uploading" progress d 0.25
```

## Blur

With `theme.style = "blur"`, add this to your Hyprland config so the compositor
blurs what is behind the island (Lua config):

```lua
hl.layer_rule({ name = "island-blur", match = { namespace = "island-shell" }, blur = true, ignore_alpha = 0.01 })
```

or in `hyprland.conf`:

```
layerrule = blur on, match:namespace island-shell
layerrule = ignore_alpha 0.01, match:namespace island-shell
```
