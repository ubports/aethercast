set(SOURCES
  gdhcp/common.c
  gdhcp/ipv4ll.c
  gdhcp/server.c
  gdhcp/client.c
  main.cpp
  unixsignalhandler.cpp
  networkutils.cpp
  dhcpclient.cpp
  dhcpserver.cpp
  mediamanagerfactory.cpp
  basesourcemediamanager.cpp
  testsourcemediamanager.cpp
  mirsourcemediamanager.cpp
  miracastservice.cpp
  miracastserviceadaptor.cpp
  miracastsource.cpp
  miracastsourceclient.cpp
  networkp2pmanager.cpp
  networkp2pdevice.cpp
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
  ${GST_INCLUDE_DIRS}
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
  ${GST_LIBRARIES}
  -ldl
  -lwds
)

install(
  TARGETS miracast-service
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib/static
)
