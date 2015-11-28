include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(utilities_tests utilities_tests.cpp)
target_link_libraries(utilities_tests miracast gtest gtest_main)
add_test(utilities_tests utilities_tests)

add_subdirectory(wpa)
