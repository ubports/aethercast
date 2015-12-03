include_directories(
    ${GLIB_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src)

add_executable(wpasupplicantmessage_tests
    wpasupplicantmessage_test.cpp)
target_link_libraries(wpasupplicantmessage_tests
    miracast
    gtest
    gtest_main)
add_test(wpasupplicantmessage_tests wpasupplicantmessage_tests)

add_executable(wpasupplicantcommandqueue_tests
    wpasupplicantcommandqueue_test.cpp)
target_link_libraries(wpasupplicantcommandqueue_tests
    miracast
    gtest
    gtest_main
    gmock)
add_test(wpasupplicantcommandqueue_tests wpasupplicantcommandqueue_tests)
