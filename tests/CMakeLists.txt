include_directories(
  ${Boost_INCLUDE_DIRS}
  ${GLIB_INCLUDE_DIRS}
  ${GIO_INCLUDE_DIRS}
  ${GIO-UNIX_INCLUDE_DIRS}
  ${GST_INCLUDE_DIRS}
  ${WDS_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/wpa
  ${CMAKE_SOURCE_DIR}/wpa/gdhcp
  ${CMAKE_SOURCE_DIR}/src
)

add_executable(utilities_tests utilities_tests.cpp)
target_link_libraries(utilities_tests miracast gtest gtest_main)
add_test(utilities_tests utilities_tests)

add_executable(scoped_gobject_tests scoped_gobject_tests.cpp)
target_link_libraries(scoped_gobject_tests miracast gtest gtest_main)
add_test(scoped_gobject_tests scoped_gobject_tests)

add_subdirectory(mcs)
