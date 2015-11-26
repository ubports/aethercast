
execute_process(COMMAND ${GDBUS_CODEGEN_EXECUTABLE}
                        --c-namespace MiracastInterface
                        --c-generate-object-manager
                        --generate-c-code ${CMAKE_CURRENT_BINARY_DIR}/mcs/miracastinterface
                        --interface-prefix org.freedesktop.miracast.
                        ${CMAKE_CURRENT_SOURCE_DIR}/../data/org.freedesktop.miracast.xml)

set(SOURCES
  mcs/main.cpp
  mcs/utilities.cpp
  mcs/networkutils.cpp
  mcs/wfddeviceinfo.cpp
  mcs/mediamanagerfactory.cpp
  mcs/basesourcemediamanager.cpp
  mcs/gstsourcemediamanager.cpp
  mcs/testsourcemediamanager.cpp
  mcs/mirsourcemediamanager.cpp
  mcs/miracastservice.cpp
  mcs/miracastserviceadapter.cpp
  mcs/miracastsource.cpp
  mcs/miracastsourceclient.cpp
  mcs/networkmanager.cpp
  mcs/networkdevice.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/mcs/miracastinterface.c
  
  wpa/gdhcp/common.c
  wpa/gdhcp/ipv4ll.c
  wpa/gdhcp/server.c
  wpa/gdhcp/client.c
  wpa/dhcpclient.cpp
  wpa/dhcpserver.cpp
  wpa/wpasupplicantmessage.cpp
  wpa/wpasupplicantcommand.cpp
  wpa/wpasupplicantcommandqueue.cpp
  wpa/wpasupplicantnetworkmanager.cpp
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
  ${CMAKE_CURRENT_SOURCE_DIR}/wpa
  ${CMAKE_CURRENT_SOURCE_DIR}/wpa/gdhcp
  ${CMAKE_CURRENT_BINARY_DIR}/src
)

add_executable(miracast-service ${SOURCES})

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


add_executable(test-wpasupplicantmessage
    utilities.cpp
    wpa/wpasupplicantmessage.cpp
    wpa/wpasupplicantmessage_test.cpp)
target_link_libraries(test-wpasupplicantmessage ${GLIB_LIBRARIES})
target_link_libraries (test-wpasupplicantmessage)
add_test(WpaSupplicantMessage test-wpasupplicantmessage)
