set(SOURCES
  gdhcp/client.c
  gdhcp/common.c
  gdhcp/ipv4ll.c
  gdhcp/server.c
  main.cpp
  miracastservice.cpp
  miracastserviceadaptor.cpp
  networkp2pmanager.cpp
  networkp2pmanagerwpasupplicant.cpp
)

link_directories(
  ${GLIB_LIBRARY_DIRS}
  ${GIO_LIBRARY_DIRS}
  ${GIO-UNIX_LIBRARY_DIRS}
)

include_directories(
  ${GLIB_INCLUDE_DIRS}
  ${GIO_INCLUDE_DIRS}
  ${GIO-UNIX_INCLUDE_DIRS}
  ${Qt5Core_INCLUDE_DIRS}
  ${Qt5DBus_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/gdhcp
)

add_executable(miracast-service ${SOURCES})

qt5_use_modules(miracast-service Core DBus Network)

target_link_libraries(miracast-service
  ${GLIB_LIBRARIES}
  ${GIO_LIBRARIES}
  ${GIO-UNIX_LIBRARIES}
  -ldl
)

install(
  TARGETS miracast-service
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib/static
)
