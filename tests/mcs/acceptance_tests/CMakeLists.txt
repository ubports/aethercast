include_directories(${CMAKE_SOURCE_DIR}/tests/3rd_party/process-cpp-minimal/include)

add_executable(miracast_service_tests miracast_service_tests.cpp did_exit_cleanly.cpp)
target_link_libraries(miracast_service_tests miracast gtest gtest_main process-cpp)
add_test(miracast_service_tests miracast_service_tests)
