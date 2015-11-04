project(miracast-service C CXX)
cmake_minimum_required(VERSION 2.8.9)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_C_FLAGS_DEBUG "-O0 -ggdb -g")
set(CMAKE_C_FLAGS_RELEASE "-O3")

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_definitions(-DQT_NO_KEYWORDS=1)

find_package(PkgConfig)
pkg_check_modules(GLIB glib-2.0)
pkg_check_modules(GIO gio-2.0)
pkg_check_modules(GIO-UNIX gio-unix-2.0)

find_package(Qt5Core REQUIRED)
include_directories(${Qt5Core_INCLUDE_DIRS})

find_package(Qt5DBus REQUIRED)
include_directories(${Qt5DBus_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -fPIC")

add_subdirectory(src)
