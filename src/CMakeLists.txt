# We have to make sure that this directory exists.
# Otherwise, GDBUS_CODEGEN_EXECUTABLE fails.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/mcs)

execute_process(COMMAND ${GDBUS_CODEGEN_EXECUTABLE}
                        --c-namespace AethercastInterface
                        --c-generate-object-manager
                        --generate-c-code ${CMAKE_CURRENT_BINARY_DIR}/mcs/aethercastinterface
                        --interface-prefix org.aethercast.
                        ${CMAKE_CURRENT_SOURCE_DIR}/../data/org.aethercast.xml)

execute_process(COMMAND ${GDBUS_CODEGEN_EXECUTABLE}
                        --c-namespace WpaSupplicant
                        --c-generate-object-manager
                        --generate-c-code ${CMAKE_CURRENT_BINARY_DIR}/mcs/wpasupplicantinterface
                        --interface-prefix fi.w1.wpa_supplicant1.
                        ${CMAKE_CURRENT_SOURCE_DIR}/../data/fi.w1.wpa_supplicant1.xml)

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
  mcs/networkutils.cpp
  mcs/mediamanagerfactory.cpp
  mcs/basesourcemediamanager.cpp
  mcs/gstsourcemediamanager.cpp
  mcs/testsourcemediamanager.cpp
  mcs/logger.cpp
  mcs/mirsourcemediamanager.cpp
  mcs/forwardingmiracastcontroller.cpp
  mcs/forwardingnetworkdevice.cpp
  mcs/miracastcontroller.cpp
  mcs/miracastcontrollerskeleton.cpp
  mcs/miracastservice.cpp
  mcs/miracastsourcemanager.cpp
  mcs/miracastsourceclient.cpp
  mcs/networkmanager.cpp
  mcs/networkmanagerfactory.cpp
  mcs/networkdevice.cpp

  ${CMAKE_CURRENT_BINARY_DIR}/mcs/aethercastinterface.c
  mcs/dbushelpers.cpp
  mcs/networkdeviceskeleton.cpp

  w11t/gdhcp/common.c
  w11t/gdhcp/ipv4ll.c
  w11t/gdhcp/server.c
  w11t/gdhcp/client.c
  w11t/dhcpclient.cpp
  w11t/dhcpserver.cpp
  w11t/wififirmwareloader.cpp
  w11t/wfddeviceinfo.cpp
  w11t/message.cpp
  w11t/command.cpp
  w11t/commandqueue.cpp
  w11t/networkdevice.cpp
  w11t/networkmanager.cpp

  ${CMAKE_CURRENT_BINARY_DIR}/mcs/wpasupplicantinterface.c
  w11tng/networkmanager.cpp
  w11tng/networkdevice.cpp
  w11tng/p2pdevicestub.cpp
  w11tng/peerstub.cpp
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
  ${CMAKE_CURRENT_SOURCE_DIR}/w11t/gdhcp
  ${CMAKE_CURRENT_BINARY_DIR}/src
)

add_library(aethercast-core ${SOURCES} ${HEADERS})

add_executable(aethercast mcs/main.cpp)

target_link_libraries(aethercast-core
  ${Boost_LIBRARIES}
  ${GLIB_LIBRARIES}
  ${GIO_LIBRARIES}
  ${GIO-UNIX_LIBRARIES}
  ${GST_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${WDS_LIBRARIES}
  -ldl
)

target_link_libraries(aethercast aethercast-core)

install(
  TARGETS aethercast
  RUNTIME DESTINATION sbin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib/static
)

add_subdirectory(client)
