cmake_minimum_required(VERSION 3.28)

project(island-shell
    VERSION 0.1.0
    DESCRIPTION "Dynamic Island style shell for Hyprland"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ISLAND_BUILD_TESTS "Build unit tests" ON)

# The empty input region (src/wayland) uses one QPA header; rebuild after Qt
# minor upgrades, as with other Qt-based shells.
set(QT_NO_PRIVATE_MODULE_WARNING ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui GuiPrivate Quick DBus Network LinguistTools)
find_package(tomlplusplus REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3)
pkg_check_modules(LIBICAL REQUIRED IMPORTED_TARGET libical)
pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)
pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
find_package(LayerShellQt REQUIRED)

include(GNUInstallDirs)

qt_standard_project_setup(REQUIRES 6.8)
qt_policy(SET QTP0004 NEW)

# qmlls picks up the build tree's module layout.
set(QT_QML_GENERATE_QMLLS_INI ON)
set(ISLAND_QML_OUTPUT ${PROJECT_BINARY_DIR}/qml)

add_compile_options(-Wall -Wextra -Wpedantic)

add_subdirectory(src)
add_subdirectory(qml/IslandShell)
add_subdirectory(activities)
add_subdirectory(tools/islandctl)

# Translations: `cmake --build --preset dev --target update_translations`
# refreshes i18n/*.ts from the sources; .qm files are embedded in the binary.
qt_add_translations(TARGETS island-shell
    TS_FILES i18n/island-shell_es.ts
    SOURCE_TARGETS islandshell islandactivities islandapp
    RESOURCE_PREFIX /i18n
    LUPDATE_OPTIONS -no-obsolete)

if(ISLAND_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()

# Data, docs and helpers.
install(PROGRAMS data/island-shell-run data/island-dl data/island-run DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/island-shell.zsh DESTINATION ${CMAKE_INSTALL_DATADIR}/island-shell)
install(FILES data/config.toml DESTINATION ${CMAKE_INSTALL_DATADIR}/island-shell)
install(DIRECTORY examples/activities DESTINATION ${CMAKE_INSTALL_DATADIR}/island-shell/examples)
install(FILES
    data/dbus/org.islandshell.Control.xml
    data/dbus/org.islandshell.LiveActivities.xml
    data/dbus/org.islandshell.Notifications.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/interfaces)
install(FILES data/systemd/island-shell.service DESTINATION lib/systemd/user)
install(FILES README.md CHANGELOG.md docs/activities.md docs/migration.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR})
