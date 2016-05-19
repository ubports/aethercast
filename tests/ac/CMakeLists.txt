include_directories(
  ${Boost_INCLUDE_DIRS}
  ${GLIB_INCLUDE_DIRS}
  ${GIO_INCLUDE_DIRS}
  ${GIO-UNIX_INCLUDE_DIRS}
  ${GST_INCLUDE_DIRS}
  ${WDS_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/mcs
  ${CMAKE_SOURCE_DIR}/src/mcs/w11t/gdhcp
  ${CMAKE_BINARY_DIR}
)

AETHERCAST_ADD_TEST(mediamanagerfactory_tests mediamanagerfactory_tests.cpp)
AETHERCAST_ADD_TEST(forwardingcontroller_tests forwardingcontroller_tests.cpp)
AETHERCAST_ADD_TEST(forwardingnetworkdevice_tests forwardingnetworkdevice_tests.cpp)
AETHERCAST_ADD_TEST(networkdevice_tests networkdevice_tests.cpp)
AETHERCAST_ADD_TEST(networkmanagerfactory_tests networkmanagerfactory_tests.cpp)
AETHERCAST_ADD_TEST(networkutils_tests networkutils_tests.cpp)

add_subdirectory(acceptance_tests)
add_subdirectory(integration_tests)
add_subdirectory(dbus)
add_subdirectory(streaming)
add_subdirectory(video)
add_subdirectory(mir)
add_subdirectory(android)
add_subdirectory(common)
add_subdirectory(report)
