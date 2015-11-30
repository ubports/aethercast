project(miracast-service C CXX)
cmake_minimum_required(VERSION 2.8.9)

include(CTest)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_C_FLAGS_DEBUG "-O0 -ggdb -g")
set(CMAKE_C_FLAGS_RELEASE "-O3")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_definitions(-DQT_NO_KEYWORDS=1)

find_package(Boost)
find_package(PkgConfig)
pkg_check_modules(GLIB glib-2.0)
pkg_check_modules(GIO gio-2.0)
pkg_check_modules(GIO-UNIX gio-unix-2.0)
pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_check_modules(WDS REQUIRED wds)

# Build with system gmock and embedded gtest
set (GMOCK_INCLUDE_DIR "/usr/include/gmock/include" CACHE PATH "gmock source include directory")
set (GMOCK_SOURCE_DIR "/usr/src/gmock" CACHE PATH "gmock source directory")
set (GTEST_INCLUDE_DIR "${GMOCK_SOURCE_DIR}/gtest/include" CACHE PATH "gtest source include directory")

add_subdirectory(${GMOCK_SOURCE_DIR} "${CMAKE_CURRENT_BINARY_DIR}/gmock")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -fPIC")

find_program(GDBUS_CODEGEN_EXECUTABLE NAMES gdbus-codegen)

add_subdirectory(src)
add_subdirectory(tests)
