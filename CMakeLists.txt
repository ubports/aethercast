project(aethercast C CXX)
cmake_minimum_required(VERSION 2.8.2)

include(CTest)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to release")
    set(CMAKE_BUILD_TYPE "release")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -Wno-variadic-macros -Wextra -fPIC")

# By default, all symbols are visible in the library. We strip out things we don't
# want at link time, by running a version script (see unity-scopes.map and the
# setting of LINK_FLAGS below).
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=default")

set(C_AND_CXX_WARNINGS "-pedantic -Wall -Wextra")

# Some additional warnings not included by the general flags set above.
set(EXTRA_C_WARNINGS "-Wcast-align -Wcast-qual -Wformat -Wredundant-decls -Wswitch-default")
set(EXTRA_CXX_WARNINGS "-Wnon-virtual-dtor -Wctor-dtor-privacy -Wold-style-cast")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_AND_CXX_WARNINGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_C_WARNINGS}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ${C_AND_CXX_WARNINGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_C_WARNINGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CXX_WARNINGS}")

# -fno-permissive causes warnings with clang, so we only enable it for gcc
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-permissive")
endif()

string(TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type_lower)

if ("${cmake_build_type_lower}" STREQUAL "release" OR "${cmake_build_type_lower}" STREQUAL "relwithdebinfo")
  option(Werror "Treat warnings as errors" ON)
else()
  option(Werror "Treat warnings as errors" OFF)
endif()

if (${Werror})
    message(STATUS "Treat warnings as errors")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
  if ("${cmake_build_type_lower}" STREQUAL "release" OR "${cmake_build_type_lower}" STREQUAL "relwithdebinfo")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error=deprecated-declarations")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=deprecated-declarations")
  endif()
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(cmake/FindReadline.cmake)

find_package(Boost COMPONENTS filesystem log serialization system thread program_options)
find_package(PkgConfig)
find_package(Threads)
find_package(LTTngUST REQUIRED)

pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)
pkg_check_modules(GIO-UNIX REQUIRED gio-unix-2.0)
pkg_check_modules(WDS REQUIRED wds)
pkg_check_modules(MIRCLIENT REQUIRED mirclient)
pkg_check_modules(HYBRIS_MEDIA REQUIRED libmedia)
pkg_check_modules(ANDROID_HEADERS REQUIRED android-headers)

include(cmake/EnableCoverageReport.cmake)

#####################################################################
# Enable code coverage calculation with gcov/gcovr/lcov
# Usage:
#  * Switch build type to coverage (use ccmake or cmake-gui)
#  * Invoke make, make test, make coverage
#  * Find html report in subdir coveragereport
#  * Find xml report feasible for jenkins in coverage.xml
#####################################################################
IF(CMAKE_BUILD_TYPE MATCHES [cC][oO][vV][eE][rR][aA][gG][eE])
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -ftest-coverage -fprofile-arcs" )
  SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -ftest-coverage -fprofile-arcs" )
ENDIF(CMAKE_BUILD_TYPE MATCHES [cC][oO][vV][eE][rR][aA][gG][eE])

# Build with system gmock and embedded gtest
find_package(GMock)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fPIC")

find_program(GDBUS_CODEGEN_EXECUTABLE NAMES gdbus-codegen)

set(AETHERCAST_TRACEPOINT_LIB_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/aethercast/tools)

add_subdirectory(conf)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(tools)

set(COVERAGE_IGNORE_FILES
  ${CMAKE_CURRENT_BINARY_DIR}/*
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/*
)

enable_coverage_report(service FILTER ${COVERAGE_IGNORE_FILES})
