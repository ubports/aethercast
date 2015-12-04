set(SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/../mcs/miracastinterface.c
    main.cpp
)

link_directories(
  ${GLIB_LIBRARY_DIRS}
  ${GIO_LIBRARY_DIRS}
  ${GIO-UNIX_LIBRARY_DIRS}
)

include_directories(
  ${Boost_INCLUDE_DIRS}
  ${GLIB_INCLUDE_DIRS}
  ${GIO_INCLUDE_DIRS}
  ${GIO-UNIX_INCLUDE_DIRS}
  ${READLINE_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}/src
)

add_executable(miracastctl ${SOURCES})
target_link_libraries(miracastctl
  ${Boost_LIBRARIES}
  ${GLIB_LIBRARIES}
  ${GIO_LIBRARIES}
  ${GIO-UNIX_LIBRARIES}
  ${READLINE_LIBRARIES}
  -ldl
)
