# API review before 1.0

What `import IslandShell` exposes today (0.1), what is wrong with it, and
what to change before freezing it as 1.0. After 1.0, breaking changes need a
major version; additions go in minor versions.

## What is public

The QML module `IslandShell` and the D-Bus interfaces in `data/dbus/` are the
two contracts other people's code depends on.

QML module, 0.1:

| Kind | Names |
|---|---|
| Types | `Activity`, `Spring`, `IslandScreen` (attached), `Arbiter`, `IslandWindow`, `ScrimWindow` |
| Components | `Clock`, `IconImage`, `IconButton`, `IslandSlider`, `ProgressRing` |
| Singletons (28) | `Config`, `Theme`, `Icons`, `ActivityRegistry`, `UserActivities`, `ShellControl`, `Hyprland`, `Notifications`, `Audio`, `Brightness`, `Mpris`, `LiveActivities`, `Apps`, `LauncherSearch`, `Power`, `Network`, `Bluetooth`, `PowerProfiles`, `Privacy`, `Calendar`, `SystemStats`, `Weather`, `NightLight`, `Screenshots`, `Wallpapers`, `Batteries` |

D-Bus: `org.islandshell.Control`, `org.islandshell.LiveActivities`,
`org.islandshell.Notifications.History`.

## Findings

### 1. Internal types are public

`IslandWindow`, `ScrimWindow` and `Arbiter` are how the shell itself is
built; no activity should create them. Once public they cannot change.
**Proposal:** move them to the internal `IslandShell.App` module.

### 2. QML calls D-Bus-style methods

`ShellControl.ShowQuickSettings()`, `ShellControl.ShowPower()`,
`ShellControl.Screenshot("region")` are the D-Bus slots, PascalCase, called
straight from QML. Every other singleton uses camelCase.
**Proposal:** camelCase QML methods (`showQuickSettings()`, `screenshot()`,
`toggleLauncher()`...) that the D-Bus slots forward to; keep the D-Bus names.

### 3. Untyped maps everywhere

`Notifications.current`, `Weather.hours`, `Calendar.eventsOn()`,
`Bluetooth.devices`, `Network.networks`, `Audio.sinks`, `Batteries.peripherals`
return `var` maps whose keys exist only in comments. A typo is `undefined` at
runtime, and qmllint cannot help.
**Options:** (a) document every map's keys as part of the 1.0 contract
(cheap; keys become frozen); (b) typed value types (`Q_GADGET`) or list
models with roles (safer, more work, better tooling).
**Proposal:** (b) for lists that change often and are shown in views
(networks, devices, sinks, events, hours) as models; (a) for the rest.

### 4. "revision" properties

`Config.revision`, `Calendar.revision`, `LauncherSearch.revision` exist only
so bindings that call methods re-evaluate. Every caller must know the trick.
**Proposal:** keep `Config.get()` + `Config.revision` (generic lookups need
it), replace the calendar's methods with models (`Calendar.upcoming` as a
property, `Calendar.month(year, month)` returning a model) and make launcher
results a model driven by a `query` property.

### 5. Inconsistent units and "missing" values

| Value | Today |
|---|---|
| `Audio.volume`, `micVolume` | 0..1.5 |
| `Brightness.level` | 0..1 |
| `Network.strength`, battery `percentage` | 0..100 |
| `Bluetooth` device `battery` | 0..100, -1 unknown |
| `SystemStats` temperatures | °C, -1 missing |
| `SystemStats` usage | 0..1, -1 missing (GPU) |
| `Mpris` position/length | seconds |

**Proposal:** fractions 0..1 for every level and percentage (volume may still
exceed 1), `NaN` for "missing" (so arithmetic does not silently use -1), and
an `available` flag on each service. Temperatures stay °C, times seconds.

### 6. Priorities are magic numbers

Activities pick numbers relative to the built-ins (idle 0, media 40, live 50,
privacy 60, notification 70, OSD 80, errors 90, critical 100, modal 1000).
**Proposal:** named constants on `Activity` (`Activity.PriorityMedia`, ...)
documented as the scale; numbers keep working.

### 7. Activities cannot declare settings

User activities read `Config.get("activities.x.key", default)` but the
settings window cannot show them. A gallery would need this.
**Proposal (1.x, additive):** `Activity.settings: [{ key, label, kind, default }]`
rendered by the settings window under the activity's name.

### 8. Silent failures

Most actions return nothing (`Bluetooth.connectDevice`, `Wallpapers.apply`,
`Audio.setDefault`); failures only reach the log.
**Proposal:** a common `error` property + `errorOccurred(message)` signal on
services with actions (Network already has `lastError`).

### 9. D-Bus versioning

The D-Bus interfaces have no version.
**Proposal:** freeze the current names as version 1; future incompatible
changes get new interface names (`org.islandshell.Control2`); additions
(new methods) are allowed.

### 10. What stays experimental after 1.0

Singletons whose shape depends on features still moving: `LauncherSearch`
(modes may grow), `Weather` (provider), `SystemStats` (sensors per vendor).
**Proposal:** ship them in a separate `IslandShell.Experimental` import, or
mark them experimental in the docs and allow minor-version changes.

## Suggested order

1. Items 1, 2, 6, 9: small, mechanical, unblock everything else.
2. Item 5 (units) and 8 (errors): touch every service once.
3. Items 3 and 4 (models): the biggest change; do it per service.
4. Item 10: decide what is experimental, write `docs/activities.md` as the
   1.0 reference, tag `v1.0.0`.
5. After 1.0: item 7 (activity settings), then the activity gallery.
