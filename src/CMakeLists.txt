add_subdirectory(gdbus)

# Make sure that generated gdbus header files can be found
include_directories(${CMAKE_CURRENT_BINARY_DIR}/gdbus)

set(AETHERCAST_DHCP_HELPER "/usr/sbin/aethercast-dhcp-helper")
set(DHCP_SERVER_PATH "/usr/sbin/dhcpd")
set(DHCP_CLIENT_PATH "/sbin/dhclient")

configure_file(mcs/config.h.in mcs/config.h @ONLY)
configure_file(w11tng/config.h.in w11tng/config.h @ONLY)

set(HEADERS
  mcs/glib_wrapper.h

  mcs/ip_v4_address.h
  mcs/keep_alive.h
  mcs/mac_address.h
  mcs/shared_gobject.h
  mcs/keep_alive.h
  mcs/config.h

  mcs/common/executable.h
  mcs/common/executor.h
  mcs/common/executorfactory.h

  mcs/network/types.h

  mcs/report/lttng/utils.h
  mcs/report/lttng/encoderreport_tp.h
  mcs/report/lttng/rendererreport_tp.h
  mcs/report/lttng/packetizerreport_tp.h
  mcs/report/lttng/senderreport_tp.h

  mcs/video/encoderreport.h
  mcs/video/rendererreport.h
  mcs/video/packetizerreport.h
  mcs/video/senderreport.h
  mcs/video/bufferproducer.h

  mcs/streaming/packetizer.h

  w11tng/config.h
)

set(SOURCES
  mcs/types.cpp
  mcs/utils.cpp
  mcs/networkutils.cpp
  mcs/mediamanagerfactory.cpp
  mcs/basesourcemediamanager.cpp
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
  mcs/dbushelpers.cpp
  mcs/networkdeviceskeleton.cpp
  mcs/dbuserrors.cpp

  mcs/common/executorpool.cpp
  mcs/common/threadedexecutor.cpp
  mcs/common/threadedexecutorfactory.cpp

  mcs/network/stream.cpp
  mcs/network/udpstream.cpp

  mcs/report/reportfactory.cpp
  mcs/report/reportfactory.h

  mcs/report/null/nullreportfactory.cpp
  mcs/report/null/encoderreport.cpp
  mcs/report/null/rendererreport.cpp
  mcs/report/null/packetizerreport.cpp
  mcs/report/null/senderreport.cpp
  mcs/report/logging/loggingreportfactory.cpp
  mcs/report/logging/encoderreport.cpp
  mcs/report/logging/rendererreport.cpp
  mcs/report/logging/packetizerreport.cpp
  mcs/report/logging/senderreport.cpp
  mcs/report/lttng/lttngreportfactory.cpp
  mcs/report/lttng/tracepointprovider.cpp
  mcs/report/lttng/encoderreport.cpp
  mcs/report/lttng/rendererreport.cpp
  mcs/report/lttng/packetizerreport.cpp
  mcs/report/lttng/senderreport.cpp

  mcs/video/videoformat.cpp
  mcs/video/buffer.cpp
  mcs/video/bufferqueue.cpp
  mcs/video/utils.cpp
  mcs/video/utils_from_android.cpp
  mcs/video/baseencoder.cpp
  mcs/video/h264analyzer.cpp
  mcs/video/displayoutput.cpp

  mcs/streaming/transportsender.cpp
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

  w11tng/networkmanager.cpp
  w11tng/rfkillmanager.cpp
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
target_compile_definitions(aethercast-core PUBLIC
    "-DAETHERCAST_TRACEPOINT_LIB_INSTALL_PATH=\"${CMAKE_INSTALL_PREFIX}/${AETHERCAST_TRACEPOINT_LIB_INSTALL_DIR}\"")
target_link_libraries(aethercast-core
  aethercast-gdbus-wrapper
  ${Boost_LDFLAGS}
  ${Boost_LIBRARIES}
  ${GLIB_LDFLAGS}
  ${GLIB_LIBRARIES}
  ${GIO_LDFLAGS}
  ${GIO_LIBRARIES}
  ${GIO-UNIX_LDFLAGS}
  ${GIO-UNIX_LIBRARIES}
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

add_library(aethercast-lttng SHARED mcs/report/lttng/tracepoints.c)
target_link_libraries(aethercast-lttng
    ${LTTNG_UST_LIBRARIES}
)

install(TARGETS aethercast-lttng
  LIBRARY DESTINATION ${AETHERCAST_TRACEPOINT_LIB_INSTALL_DIR}
)

add_executable(aethercast mcs/main.cpp)
target_link_libraries(aethercast aethercast-core)

install(
  TARGETS aethercast
  RUNTIME DESTINATION sbin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib/static
)

add_subdirectory(client)
