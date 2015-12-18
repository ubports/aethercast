include_directories(${CMAKE_SOURCE_DIR}/tests/3rd_party/process-cpp-minimal/include)

add_library(aethercast-test-fwk did_exit_cleanly.cpp)

AETHERCAST_ADD_TEST(aethercast_tests miracast_service_tests.cpp aethercast-test-fwk)
