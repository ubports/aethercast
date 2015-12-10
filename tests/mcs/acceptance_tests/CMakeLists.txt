include_directories(${CMAKE_SOURCE_DIR}/tests/3rd_party/process-cpp-minimal/include)

add_executable(aethercast_tests miracast_service_tests.cpp did_exit_cleanly.cpp)
target_link_libraries(aethercast_tests aethercast-core gtest gtest_main process-cpp)
add_test(aethercast_tests aethercast_tests)
