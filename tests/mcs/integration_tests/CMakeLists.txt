set(INTEGRATION_TESTS_SOURCE
  test_hybris_media_api.cpp
  test_stream_performance.cpp
)

add_executable(aethercast_integration_tests
    ${INTEGRATION_TESTS_SOURCE}
)

target_link_libraries(
  aethercast_integration_tests
  aethercast-core
  aethercast-test-common
  gmock
  gmock_main
  ${Boost_LIBRARIES}
)
