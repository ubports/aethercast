set(SOURCES
  main.cpp
  unixsignalhandler.cpp
  networkutils.cpp
  mediamanagerfactory.cpp
  basesourcemediamanager.cpp
  gstsourcemediamanager.cpp
  testsourcemediamanager.cpp
  mirsourcemediamanager.cpp
  miracastservice.cpp
  miracastserviceadaptor.cpp
  miracastsource.cpp
  miracastsourceclient.cpp
  networkp2pmanager.cpp
  networkp2pdevice.cpp
  wpa/gdhcp/common.c
  wpa/gdhcp/ipv4ll.c
  wpa/gdhcp/server.c
  wpa/gdhcp/client.c
  wpa/dhcpclient.cpp
  wpa/dhcpserver.cpp
  wpa/networkp2pmanagerwpasupplicant.cpp
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
  ${CMAKE_CURRENT_SOURCE_DIR}/wpa
  ${CMAKE_CURRENT_SOURCE_DIR}/wpa/gdhcp
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
