# Pure logic shared with the unit tests (no QML, no Wayland).
add_library(islandlogic STATIC
    core/configdata.h core/configdata.cpp
    core/pywal.h core/pywal.cpp
    core/tomledit.h core/tomledit.cpp
    services/hyprland/hyprlandparse.h services/hyprland/hyprlandparse.cpp
    services/hyprland/keybind.h services/hyprland/keybind.cpp
    services/notifications/notificationrules.h services/notifications/notificationrules.cpp
    services/apps/desktopentry.h services/apps/desktopentry.cpp
    services/calendar/calendarparse.h services/calendar/calendarparse.cpp
    services/calendar/quickevent.h services/calendar/quickevent.cpp
    services/power/nesteddetect.h services/power/nesteddetect.cpp
    services/privacy/camerascan.h services/privacy/camerascan.cpp
    services/live/progressparse.h services/live/progressparse.cpp
    services/launcher/calc.h services/launcher/calc.cpp
    services/system/sysparse.h services/system/sysparse.cpp
    services/weather/weatherparse.h services/weather/weatherparse.cpp
    services/wallpaper/wallpaperbackend.h services/wallpaper/wallpaperbackend.cpp
    activity/selection.h
)
target_include_directories(islandlogic PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/activity
    ${CMAKE_CURRENT_SOURCE_DIR}/services/hyprland
    ${CMAKE_CURRENT_SOURCE_DIR}/services/notifications
    ${CMAKE_CURRENT_SOURCE_DIR}/services/apps
    ${CMAKE_CURRENT_SOURCE_DIR}/services/calendar
    ${CMAKE_CURRENT_SOURCE_DIR}/services/power
    ${CMAKE_CURRENT_SOURCE_DIR}/services/privacy
    ${CMAKE_CURRENT_SOURCE_DIR}/services/live
    ${CMAKE_CURRENT_SOURCE_DIR}/services/launcher
    ${CMAKE_CURRENT_SOURCE_DIR}/services/wallpaper)
target_link_libraries(islandlogic PUBLIC Qt6::Core Qt6::Gui Qt6::DBus tomlplusplus::tomlplusplus
    PkgConfig::LIBICAL PkgConfig::XKBCOMMON)

# Internal module: the shell root and the island itself.
qt_add_qml_module(islandapp
    URI IslandShell.App
    VERSION 0.1
    STATIC
    OUTPUT_DIRECTORY ${ISLAND_QML_OUTPUT}/IslandShell/App
    IMPORTS IslandShell IslandShell.Activities
    QML_FILES
        app/Shell.qml
        app/Island.qml
        app/Settings.qml
)
target_link_libraries(islandapp PUBLIC islandshell islandactivities)

qt_add_executable(island-shell main.cpp)
target_compile_definitions(island-shell PRIVATE ISLAND_VERSION="${PROJECT_VERSION}")
target_link_libraries(island-shell PRIVATE
    Qt6::Gui Qt6::Quick
    islandshell
    islandshellplugin
    islandactivitiesplugin
    islandappplugin)

install(TARGETS island-shell RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
