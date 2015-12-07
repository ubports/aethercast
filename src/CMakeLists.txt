# We have to make sure that this directory exists.
# Otherwise, GDBUS_CODEGEN_EXECUTABLE fails.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/mcs)

execute_process(COMMAND ${GDBUS_CODEGEN_EXECUTABLE}
                        --c-namespace MiracastInterface
                        --c-generate-object-manager
                        --generate-c-code ${CMAKE_CURRENT_BINARY_DIR}/mcs/miracastinterface
                        --interface-prefix org.wds.
                        ${CMAKE_CURRENT_SOURCE_DIR}/../data/org.wds.xml)

# Make sure that generated header files can be found
include_directories(${CMAKE_CURRENT_BINARY_DIR}/mcs)

set(HEADERS
    mcs/ip_v4_address.h
    mcs/keep_alive.h
    mcs/mac_address.h
    mcs/types.h
)

set(SOURCES
  mcs/utils.cpp
  mcs/logging.cpp
  mcs/networkutils.cpp
  mcs/mediamanagerfactory.cpp
  mcs/basesourcemediamanager.cpp
  mcs/gstsourcemediamanager.cpp
  mcs/testsourcemediamanager.cpp
  mcs/mirsourcemediamanager.cpp
  mcs/miracastservice.cpp
  mcs/miracastsourcemanager.cpp
  mcs/miracastsourceclient.cpp
  mcs/networkmanager.cpp
  mcs/networkdevice.cpp

  ${CMAKE_CURRENT_BINARY_DIR}/mcs/miracastinterface.c
  mcs/dbushelpers.cpp
  mcs/miracastserviceadapter.cpp
  mcs/networkdeviceadapter.cpp

  wpa/gdhcp/common.c
  wpa/gdhcp/ipv4ll.c
  wpa/gdhcp/server.c
  wpa/gdhcp/client.c
  wpa/dhcpclient.cpp
  wpa/dhcpserver.cpp
  wpa/wififirmwareloader.cpp
  wpa/wfddeviceinfo.cpp
  wpa/wpasupplicantmessage.cpp
  wpa/wpasupplicantcommand.cpp
  wpa/wpasupplicantcommandqueue.cpp
  wpa/wpasupplicantnetworkmanager.cpp
)

link_directories(
  ${GLIB_LIBRARY_DIRS}
  ${GIO_LIBRARY_DIRS}
  ${GIO-UNIX_LIBRARY_DIRS}
  ${WDS_LIBRARY_DIRS}
)

include_directories(
  ${Boost_INCLUDE_DIRS}
  ${GLIB_INCLUDE_DIRS}
  ${GIO_INCLUDE_DIRS}
  ${GIO-UNIX_INCLUDE_DIRS}
  ${GST_INCLUDE_DIRS}
  ${WDS_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/wpa
  ${CMAKE_CURRENT_SOURCE_DIR}/wpa/gdhcp
  ${CMAKE_CURRENT_BINARY_DIR}/src
)

add_library(miracast ${SOURCES} ${HEADERS})

add_executable(miracast-service mcs/main.cpp)

target_link_libraries(miracast
  ${Boost_LIBRARIES}
  ${GLIB_LIBRARIES}
  ${GIO_LIBRARIES}
  ${GIO-UNIX_LIBRARIES}
  ${GST_LIBRARIES}
  ${WDS_LIBRARIES}
  -ldl
)

target_link_libraries(miracast-service miracast)

install(
  TARGETS miracast-service
  RUNTIME DESTINATION sbin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib/static
)

add_subdirectory(client)
