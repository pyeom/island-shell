qt_add_dbus_interface(dbus_interface
    ${PROJECT_SOURCE_DIR}/data/dbus/org.islandshell.Control.xml
    controlinterface)

qt_add_executable(islandctl main.cpp ${dbus_interface})
target_include_directories(islandctl PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(islandctl PRIVATE Qt6::DBus)

install(TARGETS islandctl RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
