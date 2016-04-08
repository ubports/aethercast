configure_file(config.h.in config.h @ONLY)

set(INTEGRATION_TESTS_SOURCE
  config.h
  test_hybris_media_api.cpp
  test_stream_performance.cpp
)

add_executable(aethercast-integration-tests
    ${INTEGRATION_TESTS_SOURCE}
)

target_link_libraries(
  aethercast-integration-tests
  aethercast-core
  aethercast-test-common
  gmock
  gmock_main
  ${Boost_LIBRARIES}
)

install(
  TARGETS aethercast-integration-tests
  RUNTIME DESTINATION sbin
)

install(
  FILES stream-performance-reference.xml
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/aethercast/tests/integration/)
