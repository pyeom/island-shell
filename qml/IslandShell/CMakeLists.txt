set_source_files_properties(Metrics.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(islandshell
    URI IslandShell
    VERSION 0.1
    STATIC
    SOURCES
        ${PROJECT_SOURCE_DIR}/src/anim/springmodel.h
        ${PROJECT_SOURCE_DIR}/src/anim/spring.h
        ${PROJECT_SOURCE_DIR}/src/anim/spring.cpp
        ${PROJECT_SOURCE_DIR}/src/wayland/islandwindow.h
        ${PROJECT_SOURCE_DIR}/src/wayland/islandwindow.cpp
    QML_FILES
        Shell.qml
        Island.qml
        ClockFace.qml
        Metrics.qml
)

target_include_directories(islandshell PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/anim
    ${PROJECT_SOURCE_DIR}/src/wayland)

target_link_libraries(islandshell PUBLIC Qt6::Quick LayerShellQt::Interface)
