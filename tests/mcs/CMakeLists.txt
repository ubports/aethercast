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
  # We have to pull in generated header files.
  ${CMAKE_BINARY_DIR}/src/mcs
)

add_executable(mediamanagerfactory_tests mediamanagerfactory_tests.cpp)
target_link_libraries(mediamanagerfactory_tests aethercast-core gtest gtest_main)
add_test(mediamanagerfactory_tests mediamanagerfactory_tests)

add_executable(forwardingmiracastcontroller_tests forwardingmiracastcontroller_tests.cpp)
target_link_libraries(forwardingmiracastcontroller_tests aethercast-core gmock gmock_main)
add_test(forwardingmiracastcontroller_tests forwardingmiracastcontroller_tests)

add_executable(miracastcontrollerskeleton_tests miracastcontrollerskeleton_tests.cpp)
target_link_libraries(miracastcontrollerskeleton_tests aethercast-core gmock gmock_main)
add_test(miracastcontrollerskeleton_tests miracastcontrollerskeleton_tests)

add_subdirectory(acceptance_tests)
