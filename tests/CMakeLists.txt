include_directories(
  ${Boost_INCLUDE_DIRS}
  ${GLIB_INCLUDE_DIRS}
  ${GIO_INCLUDE_DIRS}
  ${GIO-UNIX_INCLUDE_DIRS}
  ${GST_INCLUDE_DIRS}
  ${WDS_INCLUDE_DIRS}
  ${MIRCLIENT_INCLUDE_DIRS}
  ${HYBRIS_MEDIA_INCLDUE_DIRS}
  ${ANDROID_HEADERS_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/tests
  ${CMAKE_SOURCE_DIR}/tests/3rd_party/process-cpp-minimal/include
  # We have to pull in generated header files.
  ${CMAKE_BINARY_DIR}/src/gdbus
)

set(TEST_COMMON_SOURCE
  ${CMAKE_SOURCE_DIR}/tests/common/glibhelpers.cpp
  ${CMAKE_SOURCE_DIR}/tests/common/dbusnameowner.cpp
  ${CMAKE_SOURCE_DIR}/tests/common/dbusfixture.cpp
  ${CMAKE_SOURCE_DIR}/tests/common/virtualnetwork.cpp
  ${CMAKE_SOURCE_DIR}/tests/common/statistics.cpp
  ${CMAKE_SOURCE_DIR}/tests/common/benchmark.cpp
)

add_library(aethercast-test-common
  ${TEST_COMMON_SOURCE}
)

macro(AETHERCAST_ADD_TEST test_name src)
  add_executable(
    ${test_name}
    ${src}
  )

  target_link_libraries(
    ${test_name}

    aethercast-core
    aethercast-test-common

    ${GMOCK_LIBRARIES}
    ${GTEST_BOTH_LIBRARIES}

    process-cpp
    lxc-network

    ${ARGN}

    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
  )

  add_test(${test_name} ${CMAKE_CURRENT_BINARY_DIR}/${test_name} --gtest_filter=*-*requires*)
endmacro(AETHERCAST_ADD_TEST)

add_subdirectory(3rd_party/process-cpp-minimal)
add_subdirectory(3rd_party/lxc-nl)

AETHERCAST_ADD_TEST(utilities_tests utilities_tests.cpp)
AETHERCAST_ADD_TEST(scoped_gobject_tests scoped_gobject_tests.cpp)

add_subdirectory(w11tng)
add_subdirectory(ac)
#add_subdirectory(wds)
