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

add_executable(mediamanagerfactory_tests mediamanagerfactory_tests.cpp)
target_link_libraries(mediamanagerfactory_tests miracast gtest gtest_main)
add_test(mediamanagerfactory_tests mediamanagerfactory_tests)
