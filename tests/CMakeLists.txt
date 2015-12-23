include_directories(
  ${Boost_INCLUDE_DIRS}
  ${GLIB_INCLUDE_DIRS}
  ${GIO_INCLUDE_DIRS}
  ${GIO-UNIX_INCLUDE_DIRS}
  ${GST_INCLUDE_DIRS}
  ${WDS_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/src/mcs
)

macro(AETHERCAST_ADD_TEST test_name src)
  add_executable(
    ${test_name}
    ${src})

  target_link_libraries(
    ${test_name}

    aethercast-core

    gmock
    gmock_main

    process-cpp

    ${ARGN}

    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
)

  add_test(${test_name} ${CMAKE_CURRENT_BINARY_DIR}/${test_name} --gtest_filter=*-*requires*)
endmacro(AETHERCAST_ADD_TEST)

add_subdirectory(3rd_party/process-cpp-minimal)

AETHERCAST_ADD_TEST(utilities_tests utilities_tests.cpp)
AETHERCAST_ADD_TEST(scoped_gobject_tests scoped_gobject_tests.cpp)

add_subdirectory(w11t)
add_subdirectory(mcs)
