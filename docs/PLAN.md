# island-shell — Plan

Shell para Hyprland (Arch Linux) con una isla estilo Dynamic Island de iOS. Framework propio en C++23 sobre Qt6 Quick: el core en C++ expone servicios, QML solo pinta.

Este documento recoge las decisiones de diseño acordadas y el plan por fases. Si una decisión cambia, se actualiza aquí.

## Requisito central

**La expansión y retracción de la isla nunca afecta a las ventanas.**

Cómo se garantiza:

1. Una superficie `wlr-layer-shell` por monitor, capa `overlay`, anclada solo arriba.
2. La superficie se crea una vez con el tamaño máximo (600×420 por defecto) y nunca cambia de tamaño. Es transparente salvo la píldora.
3. `exclusive_zone` fijo (44 px por defecto): reserva el espacio de la isla compacta. Hyprland calcula el layout una sola vez.
4. La animación ocurre dentro de la superficie. La input region (`QWindow::setMask`) sigue el rectángulo de la píldora en cada frame, así los clics fuera de la píldora llegan a las ventanas de abajo.

Ningún cambio de estado de la isla genera un `configure` nuevo ni toca la reserva de espacio.

## Decisiones

### Base

| Tema | Decisión |
|---|---|
| Lenguaje | C++23, gcc (principal), clang-tidy, clang-format |
| Build | CMake con presets, Ninja |
| UI | Qt6 Quick. Core C++ (servicios, D-Bus, modelos); vistas en QML |
| Layer shell | LayerShellQt |
| Licencia | GPL-3.0-or-later |
| Distribución | PKGBUILD `island-shell-git` desde la primera versión que funcione |
| Idioma UI | Inglés con `qsTr()`, traducción `es` incluida |
| Config | `~/.config/island-shell/config.toml` (tomlplusplus), recarga en caliente |

### Isla

- Compacta ~190×36 px, centrada, 6 px del borde superior. Expandida hasta ~420×200. Launcher ~600×420. Todo configurable.
- Solo isla, sin barra. Los workspaces se muestran como puntos en la píldora compacta; la información de barra aparece al expandir.
- Interacción: hover hace un "peek", clic expande, los eventos auto-expanden con timeout. Clic derecho abre quick settings.
- Multi-monitor: una isla por monitor. Workspaces y reloj en todas; notificaciones y actividades efímeras solo en el monitor con foco.
- Animación: springs propios en C++ (masa, rigidez, amortiguación) sobre ancho, alto y radio. El contenido entra con fade, escala y blur ligero. Ticks sincronizados con el render loop de cada ventana (144 Hz y 60 Hz). `animations = "full" | "reduced" | "off"`.
- Estilo: negro puro por defecto. Blur opcional (layerrule de Hyprland). Acento desde pywal (`~/.cache/wal/colors.json`) con recarga, `theme.source = "pywal" | "static"`.

### Framework de actividades

- Todo lo que se muestra en la isla es una `Activity`, incluidas las que trae el shell (dogfooding de la API pública).
- Modelo inspirado en ActivityKit:

  ```qml
  Activity {
      name: "media"; priority: 50
      active: Mpris.current?.playing ?? false
      compactLeading:  Component { AlbumArt {} }
      compactTrailing: Component { Waveform {} }
      minimal:         Component { AlbumArt {} }     // burbuja secundaria
      expanded:        Component { MediaPanel {} }
  }
  ```

- El árbitro elige la actividad principal y una secundaria (burbuja a la derecha, estilo iOS). El resto se ve al expandir.
- Prioridad por defecto: notificación urgente > OSD > privacidad > live activity externa > media > reposo (reloj y workspaces).
- Módulo QML `import IslandShell 0.x`. Inestable hasta 1.0; los breaking changes se anotan en `CHANGELOG.md`.
- Actividades de usuario en `~/.config/island-shell/activities/`. Sin sandbox (el QML es código con permisos del shell). Cada actividad se carga en un `Loader` aislado: si falla, se desactiva solo esa y se avisa.

### IPC: D-Bus

Nombre de bus `org.islandshell.Shell`, objeto `/org/islandshell/Shell`.

| Interfaz | Métodos y señales |
|---|---|
| `org.islandshell.Control` | `ToggleLauncher`, `Expand(activity)`, `Collapse`, `Reload`, `SetDnd(bool)`, `ShowPower` |
| `org.islandshell.LiveActivities` | `Push(id, props) → handle`, `Update(id, props)`, `End(id)`, señal `ActionInvoked(id, action)` |
| `org.islandshell.Notifications.History` | `List`, `Clear`, `Dismiss(id)` |

- `islandctl` es un cliente D-Bus delgado. Shell y CLI usan QtDBus (integrado con el event loop de Qt); el adaptor y el proxy se generan del mismo XML (`data/dbus/`). Ejemplos: `islandctl launcher toggle`, `islandctl expand media`, `islandctl power`, `islandctl push --id build --progress 0.4 "Compilando"`.
- Props de una live activity: `title`, `subtitle`, `icon`, `progress` (0–1 o vacío), `state` (`running|success|error`), `ttl`, `actions` (label → id o comando).
- Ciclo de vida:
  - Clientes D-Bus: sus actividades terminan cuando se desconectan (`NameOwnerChanged`), salvo `persist=true`.
  - CLI: duran hasta `ttl` (30 min por defecto) o `end`. Las acciones ejecutan el comando guardado.

### Funcionalidades del MVP (v0.1)

1. **Notificaciones**: servidor `org.freedesktop.Notifications` dentro del proceso. Toast en la isla ~5 s (fijo si es `critical`), hover pausa el timeout, acciones, inline reply, historial agrupado por app guardado en disco, DND (las críticas pasan), reglas por app en el TOML (`mute`, `no-popup`, `urgency`).
2. **Launcher**: la isla se transforma en buscador con teclado exclusivo. Fuzzy por nombre, keywords y frecuencia de uso. Respeta `NoDisplay` y acciones de los `.desktop`. Lanza con `hyprctl dispatch exec`; fallback double-fork + `setsid`; `uwsm app` si detecta uwsm activo. Esc, clic fuera o lanzar cierran.
3. **Calendario**: vista mensual, eventos de hoy y próximos. Lee un vdir o `.ics` con libical; la sincronización con Google o CalDAV la hace vdirsyncer por fuera.
4. **Media**: MPRIS con carátula, controles, progreso, visualizador y selector de players.
5. **OSD**: volumen (Pipewire nativo) y brillo dentro de la isla.
6. **Privacidad**: micrófono, cámara y captura de pantalla en uso (Pipewire).
7. **Live Activities** por D-Bus y CLI.
8. **Quick settings**: NetworkManager, BlueZ, power-profiles-daemon, DND.
9. **Workspaces**.
10. **Power menu**: Shutdown, Reboot, Logout, Suspend y Lock vía logind. Shutdown y Reboot se confirman manteniendo presionado (~800 ms con anillo de progreso). Lock configurable (por defecto `hyprlock`). Navegación con flechas, Enter y Esc.

Batería: el indicador solo aparece si UPower detecta una.

### Supuestos por defecto

- Fuente e iconos del sistema (fontconfig, icon theme de Qt); configurables.
- Notificaciones sin sonido; hook opcional `sound_command`.
- Calendario en la zona horaria local.

### Desarrollo

- **Hyprland anidado** (`scripts/dev-nested.sh`): Hyprland como ventana dentro de la sesión, con `dbus-run-session` para tener un bus propio y no chocar con swaync o hyprpanel. Multi-monitor con `hyprctl output create headless`.
- **Validación de paquete**: `scripts/test-pkg.sh` instala el PKGBUILD en un contenedor Arch limpio (docker o podman, sin root).
- **Tests**: Qt Test para la lógica pura (springs, árbitro, parser de config, reglas de notificaciones, ranking del launcher).
- **CI**: GitHub Actions en `archlinux:latest`, compila y corre tests en cada push.
- **Arranque**: `hl.exec_cmd("island-shell")` en el autostart, con un watchdog que lo relanza si cae. Unidad systemd `--user` opcional para quien use uwsm.

### Migración de la sesión actual

Al llegar al MVP, la isla reemplaza a waybar, hyprpanel, swaync, rofi (SUPER+R), `power-menu.sh` (ALT+TAB) y `music-player.sh` (ALT+p). No se toca la config de Hyprland automáticamente: la documentación incluye un snippet en Lua listo para pegar.

## Notas de implementación

- `Activity.name` en vez de `id` (`id` está reservado en QML). Las presentaciones son `Component` porque cada isla (una por monitor) crea su propia instancia.
- Módulos QML: `IslandShell` (API pública: tipos C++, `Clock`), `IslandShell.Activities` (incluidas), `IslandShell.App` (Shell e Island, internos). Separados para evitar imports circulares.
- Los errores de `config.toml` y de actividades de usuario se muestran en la isla (actividad `errors`, prioridad 90).
- Variables de entorno para desarrollo: `ISLAND_SHELL_CONFIG`, `ISLAND_SHELL_ACTIVITIES`, `ISLAND_SHELL_DRY_RUN` (el power menu solo registra), `ISLAND_SHELL_BACKLIGHT` (directorio de backlight falso).
- Launcher y power menu son actividades `modal`: abren expandidas, toman el teclado (layer-shell exclusive) y una capa transparente a pantalla completa (scrim) las cierra al hacer clic fuera.
- uwsm: en vez de autodetectarlo, `launcher.launch_prefix = "uwsm app --"` en el TOML. Más simple y explícito.
- Brillo: lee `/sys/class/backlight` por polling (sysfs no notifica). Solo portátiles; monitores externos por DDC quedan fuera.
- La IPC con Hyprland corre en su propio hilo. Hyprland atiende `.socket.sock` de forma síncrona: si el request se escribiera desde el hilo de la UI mientras este espera al compositor (EGL hace un roundtrip de Wayland al crear una ventana), ambos se bloquean hasta el timeout de Hyprland (~10 s). Pasó en F3 y se corrigió así.
- Privacidad: micrófono = streams de captura de PipeWire en estado running, sin contar capturas de sinks (cava, grabadores de audio del escritorio) ni medidores de nivel; cámara = procesos con `/dev/video*` abierto (inotify + `/proc`), porque muchas apps usan V4L2 sin PipeWire; pantalla = eventos `screencast` de Hyprland.
- Calendario: libical expande RRULE/EXDATE/RECURRENCE-ID en una ventana de fechas; lee `displayname` y `color` de cada colección vdir.
- Con media, privacidad o un evento próximo, el reloj sigue visible a la derecha de la píldora.
- Herramientas de prueba en `scripts/testing/` (fixtures en `/tmp/ish-test`, emisor de notificaciones, player MPRIS falso).
- Validación del paquete en contenedor Arch (docker/podman) en vez de systemd-nspawn: no necesita root.
- `island-shell-run` (watchdog en bash): reinicia tras crashes con espera creciente, se rinde tras 5 en un minuto; el exit 3 ("ya hay otra instancia") no reinicia.
- Hyprland no pasa todo su entorno a `exec-once`: `dev-nested.sh` pasa explícitamente `ISLAND_SHELL_*`, `XDG_STATE_HOME`, `XDG_CACHE_HOME` y `QT_LOGGING_RULES`.
- La migración de la sesión real queda documentada en `docs/migration.md` y en la guía de pruebas; no se toca la config de Hyprland automáticamente.
- El entorno anidado usa un bus D-Bus sin directorios de servicio, para que nada se active por D-Bus y le quite `org.freedesktop.Notifications` a la isla.

## Layout del repo

```
island-shell/
├── CMakeLists.txt, CMakePresets.json
├── src/
│   ├── core/          # app, config (toml), theme, logging
│   ├── wayland/       # layer surfaces, input mask, monitores
│   ├── anim/          # springs
│   ├── activity/      # Activity, registro, árbitro
│   ├── dbus/          # adaptors org.islandshell.* (+ XML)
│   └── services/      # hyprland, notifications, mpris, audio, network,
│                      # bluetooth, power, calendar, apps, privacy, liveactivities
├── qml/IslandShell/   # módulo público: Island, Pill, componentes
├── activities/        # incluidas: clock, workspaces, notifications, media, osd,
│                      # launcher, power, calendar, quicksettings, privacy, live
├── tools/islandctl/
├── data/              # config.toml por defecto, XML D-Bus
├── i18n/              # island-shell_es.ts
├── packaging/arch/PKGBUILD
├── scripts/           # dev-nested.sh, test-pkg.sh, testing/
├── tests/
└── docs/
```

## Fases

| Fase | Contenido | Hito verificable |
|---|---|---|
| **F0 Fundación** ✅ | CMake y presets, CI, `dev-nested.sh`, superficie layer-shell por monitor, exclusive zone, input mask, springs, píldora con reloj | La isla se expande y contrae con hover y clic sin mover ninguna ventana, y los clics fuera de la píldora pasan |
| **F1 Framework** ✅ | `Activity` y árbitro, módulo `IslandShell 0.1`, Loader aislado, config TOML con recarga, tema y pywal, D-Bus `Control` e `islandctl`, servicio Hyprland, actividad de workspaces | Una actividad de usuario en `~/.config/island-shell/activities/` aparece y se recarga en caliente |
| **F2 Núcleo MVP** ✅ | Notificaciones, OSD de audio, Media, Live Activities, Launcher, Power menu | En el entorno anidado se puede prescindir de waybar, hyprpanel, swaync y rofi |
| **F3 Sistema** ✅ | Quick settings, privacidad, calendario (requiere `libical`) | MVP completo |
| **F4 Release v0.1** ✅ | Traducción `es`, docs, PKGBUILD, prueba en nspawn, migración de la sesión real con 3 monitores | `makepkg -si` en un Arch limpio funciona; la sesión diaria usa la isla |
| **Después** | Calculadora, emojis y portapapeles en el launcher, timer/pomodoro, clima, indicador de grabación, API 1.0 | — |

## Dependencias

Runtime: `qt6-base`, `qt6-declarative`, `qt6-wayland`, `qt6-svg`, `layer-shell-qt`, `tomlplusplus`, `libpipewire` (F2), `libical` (F3), `hyprland`.

Build: `cmake`, `ninja`, `gcc`, `qt6-shadertools`.
