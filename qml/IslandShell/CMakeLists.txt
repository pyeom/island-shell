# Public module: everything activities can use via `import IslandShell`.

set(src ${PROJECT_SOURCE_DIR}/src)

set_source_files_properties(Clock.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(${PROJECT_SOURCE_DIR}/data/emoji.tsv PROPERTIES QT_RESOURCE_ALIAS emoji.tsv)

qt_add_dbus_adaptor(dbus_adaptor
    ${PROJECT_SOURCE_DIR}/data/dbus/org.islandshell.Control.xml
    shellcontrol.h
    island::ShellControl
    controladaptor
    ControlAdaptor)

qt_add_dbus_adaptor(dbus_adaptor
    ${PROJECT_SOURCE_DIR}/data/dbus/org.freedesktop.Notifications.xml
    notifications.h
    island::Notifications
    notificationsadaptor
    NotificationsAdaptor)

qt_add_dbus_adaptor(dbus_adaptor
    ${PROJECT_SOURCE_DIR}/data/dbus/org.islandshell.Notifications.xml
    notifications.h
    island::Notifications
    historyadaptor
    HistoryAdaptor)

qt_add_dbus_adaptor(dbus_adaptor
    ${PROJECT_SOURCE_DIR}/data/dbus/org.islandshell.LiveActivities.xml
    liveactivities.h
    island::LiveActivities
    liveactivitiesadaptor
    LiveActivitiesAdaptor)

# PipeWire's C headers are not -Wpedantic clean.
set_source_files_properties(${src}/services/audio/audio.cpp ${src}/services/privacy/privacy.cpp PROPERTIES
    COMPILE_OPTIONS "-Wno-pedantic;-Wno-missing-field-initializers")

qt_add_qml_module(islandshell
    URI IslandShell
    VERSION 0.1
    STATIC
    OUTPUT_DIRECTORY ${ISLAND_QML_OUTPUT}/IslandShell
    SOURCES
        ${src}/anim/springmodel.h
        ${src}/anim/spring.h ${src}/anim/spring.cpp
        ${src}/wayland/islandwindow.h ${src}/wayland/islandwindow.cpp
        ${src}/core/filewatch.h ${src}/core/filewatch.cpp
        ${src}/core/config.h ${src}/core/config.cpp
        ${src}/core/theme.h ${src}/core/theme.cpp
        ${src}/activity/activity.h ${src}/activity/activity.cpp
        ${src}/activity/activityregistry.h ${src}/activity/activityregistry.cpp
        ${src}/activity/arbiter.h ${src}/activity/arbiter.cpp
        ${src}/activity/islandscreen.h ${src}/activity/islandscreen.cpp
        ${src}/activity/useractivities.h ${src}/activity/useractivities.cpp
        ${src}/dbus/shellcontrol.h ${src}/dbus/shellcontrol.cpp
        ${src}/services/hyprland/hyprland.h ${src}/services/hyprland/hyprland.cpp
        ${src}/services/hyprland/hyprlandipc.h ${src}/services/hyprland/hyprlandipc.cpp
        ${src}/services/hyprland/keybinds.h ${src}/services/hyprland/keybinds.cpp
        ${src}/services/notifications/notifications.h ${src}/services/notifications/notifications.cpp
        ${src}/services/audio/audio.h ${src}/services/audio/audio.cpp
        ${src}/services/mpris/mpris.h ${src}/services/mpris/mpris.cpp
        ${src}/services/live/liveactivities.h ${src}/services/live/liveactivities.cpp
        ${src}/services/live/appprogress.h ${src}/services/live/appprogress.cpp
        ${src}/services/live/downloadwatch.h ${src}/services/live/downloadwatch.cpp
        ${src}/services/apps/apps.h ${src}/services/apps/apps.cpp
        ${src}/services/apps/defaultapps.h ${src}/services/apps/defaultapps.cpp
        ${src}/services/power/power.h ${src}/services/power/power.cpp
        ${src}/services/brightness/brightness.h ${src}/services/brightness/brightness.cpp
        ${src}/services/network/network.h ${src}/services/network/network.cpp
        ${src}/services/bluetooth/bluetooth.h ${src}/services/bluetooth/bluetooth.cpp
        ${src}/services/powerprofiles/powerprofiles.h ${src}/services/powerprofiles/powerprofiles.cpp
        ${src}/services/privacy/privacy.h ${src}/services/privacy/privacy.cpp
        ${src}/services/calendar/calendar.h ${src}/services/calendar/calendar.cpp
        ${src}/services/launcher/launchersearch.h ${src}/services/launcher/launchersearch.cpp
        ${src}/services/system/systemstats.h ${src}/services/system/systemstats.cpp
        ${src}/services/weather/weather.h ${src}/services/weather/weather.cpp
        ${src}/services/nightlight/nightlight.h ${src}/services/nightlight/nightlight.cpp
        ${src}/services/screenshots/screenshots.h ${src}/services/screenshots/screenshots.cpp
        ${src}/services/wallpaper/wallpapers.h ${src}/services/wallpaper/wallpapers.cpp
        ${src}/services/batteries/batteries.h ${src}/services/batteries/batteries.cpp
        ${src}/services/updates/updates.h ${src}/services/updates/updates.cpp
        ${src}/services/timers/timers.h ${src}/services/timers/timers.cpp
        ${src}/dbus/dbusutil.h
        ${src}/dbus/dbusproperties.h ${src}/dbus/dbusproperties.cpp
        ${src}/core/icons.h ${src}/core/icons.cpp
        ${src}/wayland/scrimwindow.h ${src}/wayland/scrimwindow.cpp
        ${dbus_adaptor}
    QML_FILES
        Clock.qml
        IconImage.qml
        IconButton.qml
        ProgressRing.qml
        IslandSlider.qml
        RoundedImage.qml
    RESOURCES
        ${PROJECT_SOURCE_DIR}/data/emoji.tsv
)

target_include_directories(islandshell PUBLIC
    ${src}/anim
    ${src}/wayland
    ${src}/core
    ${src}/activity
    ${src}/dbus
    ${src}/services/hyprland
    ${src}/services/notifications
    ${src}/services/audio
    ${src}/services/mpris
    ${src}/services/live
    ${src}/services/apps
    ${src}/services/power
    ${src}/services/brightness
    ${src}/services/network
    ${src}/services/bluetooth
    ${src}/services/powerprofiles
    ${src}/services/privacy
    ${src}/services/calendar
    ${src}/services/launcher
    ${src}/services/system
    ${src}/services/weather
    ${src}/services/nightlight
    ${src}/services/screenshots
    ${src}/services/wallpaper
    ${src}/services/batteries
    ${src}/services/updates
    ${src}/services/timers
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(islandshell PUBLIC
    islandlogic
    Qt6::Quick
    Qt6::DBus
    Qt6::Network
    LayerShellQt::Interface
    Qt6::GuiPrivate
    PkgConfig::WAYLAND_CLIENT
    PkgConfig::PIPEWIRE)
