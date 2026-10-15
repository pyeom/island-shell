# Activities that ship with the shell. They only use the public IslandShell
# API, same as user activities.

qt_add_qml_module(islandactivities
    URI IslandShell.Activities
    VERSION 0.1
    STATIC
    OUTPUT_DIRECTORY ${ISLAND_QML_OUTPUT}/IslandShell/Activities
    IMPORTS IslandShell
    QML_FILES
        Idle.qml
        IdleExpanded.qml
        WorkspaceDots.qml
        ShellErrors.qml
)

target_link_libraries(islandactivities PUBLIC islandshell)
