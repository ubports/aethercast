include_directories(
    ${GLIB_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src)

add_executable(wpasupplicantmessage_tests
    wpasupplicantmessage_test.cpp)
target_link_libraries(wpasupplicantmessage_tests
    aethercast-core
    gtest
    gtest_main)
add_test(wpasupplicantmessage_tests wpasupplicantmessage_tests)

add_executable(wpasupplicantcommandqueue_tests
    wpasupplicantcommandqueue_test.cpp)
target_link_libraries(wpasupplicantcommandqueue_tests
    aethercast-core
    gtest
    gtest_main
    gmock)
add_test(wpasupplicantcommandqueue_tests wpasupplicantcommandqueue_tests)

add_executable(wfddeviceinfo_tests
    wfddeviceinfo_test.cpp)
target_link_libraries(wfddeviceinfo_tests
    aethercast-core
    gtest
    gtest_main
    gmock)
add_test(wfddeviceinfo_tests wfddeviceinfo_tests)

