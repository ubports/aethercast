include_directories(
    ${GLIB_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/mcs)

AETHERCAST_ADD_TEST(wpasupplicantmessage_tests wpasupplicantmessage_test.cpp)
AETHERCAST_ADD_TEST(wpasupplicantcommandqueue_tests wpasupplicantcommandqueue_test.cpp)
AETHERCAST_ADD_TEST(wfddeviceinfo_tests wfddeviceinfo_test.cpp)
