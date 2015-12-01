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

set(GMOCK_BOTH_LIBRARIES gmock gmock_main)

include_directories(
  ${CMAKE_SOURCE_DIR}/src
  ${GTEST_INCLUDE_DIR}
)

add_executable(
  posix_process_test
  posix_process_test.cpp
)

add_executable(
  linux_process_test
  linux_process_test.cpp
)

add_executable(
  fork_and_run_test
  fork_and_run_test.cpp
)

add_executable(
  cross_process_sync_test
  cross_process_sync_test.cpp
)

add_executable(
  death_observer_test
  death_observer_test.cpp
)

target_link_libraries(
  posix_process_test

  process-cpp

  ${CMAKE_THREAD_LIBS_INIT}
  ${GMOCK_BOTH_LIBRARIES}
)

target_link_libraries(
  linux_process_test

  process-cpp

  ${CMAKE_THREAD_LIBS_INIT}
  ${GMOCK_BOTH_LIBRARIES}
)

target_link_libraries(
  fork_and_run_test

  process-cpp

  ${CMAKE_THREAD_LIBS_INIT}
  ${GMOCK_BOTH_LIBRARIES}
)

target_link_libraries(
  cross_process_sync_test

  process-cpp

  ${CMAKE_THREAD_LIBS_INIT}
  ${GMOCK_BOTH_LIBRARIES}
)

target_link_libraries(
  death_observer_test

  process-cpp

  ${CMAKE_THREAD_LIBS_INIT}
  ${GMOCK_BOTH_LIBRARIES}
)

add_test(posix_process_test ${CMAKE_CURRENT_BINARY_DIR}/posix_process_test)
add_test(linux_process_test ${CMAKE_CURRENT_BINARY_DIR}/linux_process_test)
add_test(fork_and_run_test ${CMAKE_CURRENT_BINARY_DIR}/fork_and_run_test)
add_test(cross_process_sync_test ${CMAKE_CURRENT_BINARY_DIR}/cross_process_sync_test)
add_test(death_observer_test ${CMAKE_CURRENT_BINARY_DIR}/death_observer_test)
