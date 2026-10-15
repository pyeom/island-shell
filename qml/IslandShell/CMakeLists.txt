# Public module: everything activities can use via `import IslandShell`.

set(src ${PROJECT_SOURCE_DIR}/src)

set_source_files_properties(Clock.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_dbus_adaptor(dbus_adaptor
    ${PROJECT_SOURCE_DIR}/data/dbus/org.islandshell.Control.xml
    shellcontrol.h
    island::ShellControl
    controladaptor
    ControlAdaptor)

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
        ${dbus_adaptor}
    QML_FILES
        Clock.qml
)

target_include_directories(islandshell PUBLIC
    ${src}/anim
    ${src}/wayland
    ${src}/core
    ${src}/activity
    ${src}/dbus
    ${src}/services/hyprland
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(islandshell PUBLIC
    islandlogic
    Qt6::Quick
    Qt6::DBus
    LayerShellQt::Interface)
