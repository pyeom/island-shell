# Writing activities

Everything the island shows is an **activity**: the clock, the error badge, and
(from F2) notifications, media and the launcher. The built-in ones use the same
API described here.

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
| `errors` | 90 |
| `idle` (workspaces + clock) | 0 |

Planned in F2: urgent notification > OSD > privacy > live activities > media.

### Sizes

The compact pill grows to fit `compactLeading` + `compactTrailing`. The expanded
pill takes the `implicitWidth`/`implicitHeight` of the `expanded` item, capped by
`island.surface_width`/`surface_height`. Items are laid out at their implicit
size and revealed by the pill as it grows, so give them a fixed implicit size.

### Alerts

`alert(ms)` briefly expands the island to show this activity, for example when a
timer ends. It collapses after `ms` unless the pointer is over it.

## Available API

| Name | What it is |
|---|---|
| `Activity` | The type above. |
| `Config` | Settings. `Config.get("activities.pomodoro.minutes", 25)` reads any key; also `Config.clockFormat`, sizes. |
| `Theme` | `background`, `foreground`, `secondaryForeground`, `accent`, `danger`, `blur`. |
| `Clock.now` | A `date` that updates every second. |
| `Hyprland` | `monitors`, `workspaces`, `focusedMonitor`, `dispatch("workspace 3")`, `event(name, data)` signal. |
| `IslandScreen` | Attached: `IslandScreen.name`, `IslandScreen.focused` for the monitor a presentation is drawn on. |
| `ShellControl` | `dnd` and the requests coming from `islandctl`. |
| `ActivityRegistry` | `activities`, `find(name)`. |
| `Spring` | Spring animation: `Spring { id: s; target: x }` then use `s.value`. |

## islandctl

```
islandctl expand [activity]   expand (the current activity if omitted)
islandctl collapse
islandctl activities          list names
islandctl reload              reload config, theme and activities
islandctl dnd on|off|toggle
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
