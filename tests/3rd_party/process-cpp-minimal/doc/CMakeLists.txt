# Copyright © 2013 Canonical Ltd.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Authored by: Thomas Voss <thomas.voss@canonical.com>

option(
  PROCESS_CPP_ENABLE_DOC_GENERATION
  "Generate package documentation with doxygen"
  ON
)

string(TOLOWER "${CMAKE_BUILD_TYPE}" cmake_build_type_lower)

if("${cmake_build_type_lower}" STREQUAL "debug")
  option(
    PROCESS_CPP_ENABLE_DOC_GENERATION_BY_DEFAULT
    "Generate package by default"
    OFF
  )
else()
  option(
    PROCESS_CPP_ENABLE_DOC_GENERATION_BY_DEFAULT
    "Generate package by default"
    ON
  )
endif()

if (PROCESS_CPP_ENABLE_DOC_GENERATION)

if (${PROCESS_CPP_ENABLE_DOC_GENERATION_BY_DEFAULT})
  set (IS_ALL ALL)
endif ()

find_package(Doxygen)

if (DOXYGEN_FOUND)
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in 
    ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc ${IS_ALL}
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen" VERBATIM)
  install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
    DESTINATION ${CMAKE_INSTALL_DOCDIR})
endif (DOXYGEN_FOUND)
endif (PROCESS_CPP_ENABLE_DOC_GENERATION)
