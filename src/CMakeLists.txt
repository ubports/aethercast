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

set(AETHERCAST_DHCP_HELPER "/usr/sbin/aethercast-dhcp-helper")
set(DHCP_SERVER_PATH "/usr/sbin/dhcpd")
set(DHCP_CLIENT_PATH "/sbin/dhclient")

configure_file(mcs/config.h.in mcs/config.h @ONLY)
configure_file(w11tng/config.h.in w11tng/config.h @ONLY)

set(HEADERS
  mcs/ip_v4_address.h
  mcs/keep_alive.h
  mcs/mac_address.h
  mcs/types.h
  mcs/shared_gobject.h
  mcs/keep_alive.h
  mcs/config.h

  mcs/common/executable.h
  mcs/common/executor.h

  mcs/streaming/packetizer.h
  mcs/streaming/transportsender.h

  w11tng/config.h
)

set(SOURCES
  mcs/utils.cpp
  mcs/networkutils.cpp
  mcs/mediamanagerfactory.cpp
  mcs/basesourcemediamanager.cpp
  mcs/initgstreameronce.cpp
  mcs/gstsourcemediamanager.cpp
  mcs/testsourcemediamanager.cpp
  mcs/x11sourcemediamanager.cpp
  mcs/logger.cpp
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

  mcs/common/threadedexecutor.cpp

  mcs/video/videoformat.cpp
  mcs/video/buffer.cpp
  mcs/video/bufferqueue.cpp
  mcs/video/utils.cpp
  mcs/video/utils_from_android.cpp
  mcs/video/baseencoder.cpp
  mcs/video/h264analyzer.cpp
  mcs/video/statistics.cpp

  mcs/streaming/mpegtspacketizer.cpp
  mcs/streaming/rtpsender.cpp
  mcs/streaming/mediasender.cpp

  mcs/mir/sourcemediamanager.cpp
  mcs/mir/screencast.cpp
  mcs/mir/streamrenderer.cpp

  mcs/android/h264encoder.cpp

  mcs/systemcontroller.cpp

  mcs/ubuntu/unity.cpp
  mcs/ubuntu/systemcontroller.cpp
  mcs/ubuntu/unitydisplaylock.cpp

  ${CMAKE_CURRENT_BINARY_DIR}/mcs/wpasupplicantinterface.c
  w11tng/networkmanager.cpp
  w11tng/networkdevice.cpp
  w11tng/interfaceselector.cpp
  w11tng/p2pdevicestub.cpp
  w11tng/managerstub.cpp
  w11tng/peerstub.cpp
  w11tng/interfacestub.cpp
  w11tng/groupstub.cpp
  w11tng/informationelement.cpp
  w11tng/dhcpleaseparser.cpp
  w11tng/dhcpclient.cpp
  w11tng/dhcpserver.cpp
  w11tng/processexecutor.cpp
  w11tng/filemonitor.cpp
  w11tng/wififirmwareloader.cpp
  w11tng/hostname1stub.cpp
)

include_directories(
  ${Boost_INCLUDE_DIRS}
  ${GLIB_INCLUDE_DIRS}
  ${GIO_INCLUDE_DIRS}
  ${GIO-UNIX_INCLUDE_DIRS}
  ${GST_INCLUDE_DIRS}
  ${WDS_INCLUDE_DIRS}
  ${HYBRIS_MEDIA_INCLDUE_DIRS}
  ${ANDROID_HEADERS_INCLUDE_DIRS}
  ${MIRCLIENT_INCLUDE_DIRS}
  ${EGL_INCLUDE_DIRS}
  ${GLESV2_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}/src
  ${CMAKE_CURRENT_BINARY_DIR}/src/w11tng
)

add_library(aethercast-core ${SOURCES} ${HEADERS})

add_executable(aethercast mcs/main.cpp)

target_link_libraries(aethercast-core
  ${Boost_LDFLAGS}
  ${Boost_LIBRARIES}
  ${GLIB_LDFLAGS}
  ${GLIB_LIBRARIES}
  ${GIO_LDFLAGS}
  ${GIO_LIBRARIES}
  ${GIO-UNIX_LDFLAGS}
  ${GIO-UNIX_LIBRARIES}
  ${GST_LDFLAGS}
  ${GST_LIBRARIES}
  ${GST_APP_LDFLAGS}
  ${GST_APP_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${WDS_LDFLAGS}
  ${WDS_LIBRARIES}
  ${HYBRIS_MEDIA_LDFLAGS}
  ${HYBRIS_MEDIA_LIBRARIES}
  ${MIRCLIENT_LDFLAGS}
  ${MIRCLIENT_LIBRARIES}
  ${EGL_LDFLAGS}
  ${EGL_LIBRARIES}
  ${GLESV2_LDFLAGS}
  ${GLESV2_LIBRARIES}
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
