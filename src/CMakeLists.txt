qt_add_executable(island-shell main.cpp)

target_link_libraries(island-shell PRIVATE Qt6::Gui Qt6::Quick islandshellplugin)

install(TARGETS island-shell RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
target_compile_definitions(island-shell PRIVATE ISLAND_VERSION="${PROJECT_VERSION}")
