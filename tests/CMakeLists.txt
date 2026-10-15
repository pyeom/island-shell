qt_add_executable(tst_springmodel tst_springmodel.cpp)
target_include_directories(tst_springmodel PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(tst_springmodel PRIVATE Qt6::Test)
add_test(NAME springmodel COMMAND tst_springmodel)
