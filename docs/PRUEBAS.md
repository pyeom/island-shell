# Guía de pruebas

Qué probar a mano y cómo. Lo que ya está cubierto por tests automáticos
(springs, árbitro, config, reglas de notificaciones, `.desktop`, calendario)
no se repite aquí: esto es lo que necesita mouse, teclado, hardware real o tu
sesión real.

Orden recomendado: **1 → 2 → 3**. Cada etapa es reversible.

## 0. Preparación

```sh
cd ~/.t3/projects/projects-island-shell
cmake --preset dev && cmake --build --preset dev
ctest --preset dev                 # 8 suites, deben pasar
```

Logs de la isla (en cualquier etapa):

```sh
journalctl -f _COMM=island-shell                       # normal
QT_LOGGING_RULES='island.*=true' ...                   # más detalle al lanzarla
```

## 1. Entorno anidado (no toca tu sesión)

Hyprland corre como ventana, con su propio D-Bus. Nada de lo que pase ahí
llega a tu sesión real.

```sh
scripts/testing/run-nested.sh
```

Usa datos de prueba en `/tmp/ish-test`: un calendario con un evento en ~8
minutos, un backlight falso, reglas de notificaciones y el power menu en
**modo de prueba** (solo registra, no apaga). Dentro de la ventana:
`SUPER+Return` abre una terminal, `SUPER+R` reinicia la isla,
`SUPER+SHIFT+E` cierra todo. Los avisos rojos/amarillos de Hyprland sobre
`.conf` y `start-hyprland` son del Hyprland anidado; ignóralos.

Para mandar comandos desde tu terminal normal al anidado:

```sh
source scripts/testing/lib.sh       # en bash; deja $CTL = islandctl del anidado
$CTL activities
```

Al terminar: `scripts/testing/close-nested.sh` (nunca cierra tu sesión real).

### Checklist

| # | Qué hacer | Qué esperar |
|---|---|---|
| 1 | Pasa el mouse sobre la píldora | Crece un poco (peek). Al salir vuelve a su tamaño |
| 2 | Clic en la píldora | Se expande: reloj grande, fecha, workspaces, calendario del mes, próximos eventos |
| 3 | Con la isla expandida, saca el mouse | Se contrae ~0.7 s después |
| 4 | Haz clic en una ventana **debajo** de la zona transparente de la isla (al lado de la píldora) | El clic llega a la ventana. **Las ventanas nunca se mueven** al expandir/contraer |
| 5 | En la vista expandida, clic en un punto de workspace | Cambia de workspace |
| 6 | Flechas del mes en el calendario; clic en un día | Cambia de mes; la lista muestra los eventos de ese día |
| 7 | Clic derecho en la píldora | Quick settings |
| 8 | En quick settings: arrastra el slider de volumen, rueda del mouse encima, clic en el icono | Cambia el volumen **real** del sistema; el icono silencia/activa |
| 9 | Slider de brillo | Cambia `/tmp/ish-test/backlight/actual_brightness` (backlight falso) |
| 10 | Perfil de energía: clic en "Ahorro de energía" y vuelve a "Equilibrado" | Cambia el perfil **real** (`powerprofilesctl get` para confirmar) |
| 11 | Tile "No molestar" | Se pone azul; las notificaciones normales dejan de aparecer (las críticas sí) |
| 12 | Clic fuera de la isla con quick settings abierto, o `Esc` | Se cierra |
| 13 | `SUPER+R` no; usa `$CTL launcher toggle` (o crea el bind en el `hyprland.conf` anidado) y escribe "fire", flechas, `Enter` | Busca, mueve la selección y abre la app dentro del anidado. `→` sobre una app con acciones (Firefox) elige "Nueva ventana" |
| 14 | `Esc` en el launcher, y clic fuera | Se cierra |
| 15 | `$CTL power`, flechas, `Enter` sobre "Bloquear" | Se cierra y el log dice `dry run: lock` |
| 16 | En "Apagar": un clic corto, luego **mantener** clic (o `Enter`) | Clic corto no hace nada; mantener llena el anillo rojo y el log dice `dry run: shutdown` |
| 17 | Notificación con acciones y respuesta: `python3 -I scripts/testing/notify.py --app Telegram --icon org.telegram.desktop --action default=Abrir --action archive=Archivar --reply --wait "Ana" "¿Vamos?"` | La isla se expande. Pasar el mouse pausa el tiempo. Clic en "Archivar" → la terminal imprime `ActionInvoked ... archive`. Escribir en "Responder…" + `Enter` → `NotificationReplied ... texto` |
| 18 | Clic en la ✕ de una notificación del historial y en "Borrar" | Desaparecen; `$CTL notifications` lo confirma |
| 19 | Media: `python3 -I scripts/testing/fakeplayer.py 60` | Portada + barras a la izquierda, reloj a la derecha. Expandida: botones ⏮ ⏯ ⏭ imprimen `Previous`/`PlayPause`/`Next` en la terminal del player; clic en la barra de progreso salta |
| 20 | Live activity: `$CTL push --id build --progress 0.3 --action "Cancelar=notify-send hola" "Compilando"`, luego `$CTL update --id build --progress 0.9`, `--state success`, `$CTL end build` | Anillo de progreso, porcentaje, check verde al terminar. Clic en "Cancelar" ejecuta el comando |
| 21 | Espera a que llegue el evento del calendario de prueba (~8 min) | Aparece "Standup del equipo · en N min", luego "ahora" |
| 22 | Edita `/tmp/ish-test/config.toml`: `[island] compact_width = 260` y guarda | La píldora cambia sin reiniciar. Pon `compact_width = 1`: aparece "1 error" en la isla |
| 23 | Copia `examples/activities/Pomodoro.qml` editado con un error de sintaxis a `/tmp/ish-test/activities/` | La isla muestra el error; al corregir y guardar desaparece |

Si algo falla: anota el número, qué hiciste y lo que viste, y adjunta
`journalctl _COMM=island-shell --since "-10min"`.

## 2. Sesión real, en paralelo (sin migrar)

La isla corre junto a waybar, en tus 3 monitores. Sirve para probar lo que el
anidado no puede: monitores reales a 144/60 Hz, Firefox/Spotify por MPRIS,
Discord y la cámara reales.

```sh
ISLAND_SHELL_DRY_RUN=1 build/dev/src/island-shell
```

(`Ctrl+C` en esa terminal la cierra; todo vuelve a como estaba.)

Ten en cuenta:

- Reserva 44 px arriba **además** de waybar: las ventanas bajan un poco
  mientras corre. Es esperado en esta etapa.
- swaync hoy solo se activa con la primera notificación. Si no está
  corriendo, la isla toma las notificaciones mientras esté abierta; si swaync
  ya corre, la isla muestra un error "notifications: ... owned by swaync" y no
  le quita nada. Ambos casos son correctos.
- Usa tu config real (`~/.config/island-shell/`, si existe) y guarda el
  historial en `~/.local/state/island-shell/`.

| # | Qué hacer | Qué esperar |
|---|---|---|
| 24 | Mira los 3 monitores | Una isla por monitor, cada una con sus workspaces |
| 25 | Mueve el foco entre monitores y lanza `notify-send hola` (desde una terminal fuera de T3 Code) | La notificación aparece solo en el monitor con foco |
| 26 | Expande/contrae en DP-2 (144 Hz) y en HDMI-A-1 (60 Hz) | Animación igual de fluida y con la misma duración en ambos |
| 27 | Reproduce algo en Firefox o Spotify | Aparece media con portada real; los botones controlan el player |
| 28 | Entra a un canal de voz en Discord | Punto naranja de micrófono, "Discord" al expandir |
| 29 | Abre la cámara (una videollamada en el navegador, o cualquier app de cámara) | Indicador verde de cámara |
| 30 | Comparte pantalla en una videollamada (portal) | Indicador morado de pantalla |
| 31 | Teclas de volumen (ALT+F2/F3) | OSD de volumen en la isla |
| 32 | `islandctl launcher toggle` y abre una app real | Se abre en tu sesión, en el workspace que dicten tus windowrules |
| 33 | Opcional: `[theme] source = "pywal"` y `style = "blur"` + la layer_rule de `docs/migration.md` | Píldora translúcida con colores de tu wallpaper |

## 3. Migración (cuando 1 y 2 estén bien)

Sigue `docs/migration.md`. En tu config concretamente:

- `~/.config/hypr/conf/autostart.lua`: comenta `hl.exec_cmd("waybar")` y
  `hl.exec_cmd("hyprpanel")` (hyprpanel ni siquiera está instalado) y agrega
  `hl.exec_cmd("island-shell-run")`.
- `~/.config/hypr/conf/binds.lua`: SUPER+R → `islandctl launcher toggle`,
  ALT+TAB → `islandctl power`, ALT+p → `islandctl expand media`; ALT+a y
  ALT+r (refresh de waybar/swaync) ya no hacen falta.
- Instala con el paquete (`packaging/arch`, `makepkg -si`) o usa los binarios
  de `build/release` en tu `PATH`.

Después de reiniciar sesión:

| # | Qué hacer | Qué esperar |
|---|---|---|
| 34 | `busctl --user status org.freedesktop.Notifications` | `Comm=island-shell` |
| 35 | `pkill -SEGV island-shell` (simula un crash) | `island-shell-run` la levanta en ~1 s |
| 36 | Usa el power menu **sin** modo de prueba: "Bloquear" | `hyprlock` |
| 37 | Un día normal de uso | Anota lo que moleste o falte |

Volver atrás: deshaz los cambios de autostart y binds (ver "Going back" en
`docs/migration.md`).
