cmake_minimum_required(VERSION 3.28)

project(island-shell
    VERSION 0.0.1
    DESCRIPTION "Dynamic Island style shell for Hyprland"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ISLAND_BUILD_TESTS "Build unit tests" ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick)
find_package(LayerShellQt REQUIRED)

include(GNUInstallDirs)

qt_standard_project_setup(REQUIRES 6.8)
qt_policy(SET QTP0004 NEW)

add_compile_options(-Wall -Wextra -Wpedantic)

add_subdirectory(qml/IslandShell)
add_subdirectory(src)

if(ISLAND_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
