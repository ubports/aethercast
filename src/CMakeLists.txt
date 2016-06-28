add_subdirectory(gdbus)

# Make sure that generated gdbus header files can be found
include_directories(${CMAKE_CURRENT_BINARY_DIR}/gdbus)

set(AETHERCAST_DHCP_HELPER "/usr/sbin/aethercast-dhcp-helper")
set(DHCP_SERVER_PATH "/usr/sbin/dhcpd")
set(DHCP_CLIENT_PATH "/sbin/dhclient")

configure_file(ac/config.h.in ac/config.h @ONLY)
configure_file(w11tng/config.h.in w11tng/config.h @ONLY)

set(HEADERS
  ac/glib_wrapper.h

  ac/ip_v4_address.h
  ac/keep_alive.h
  ac/mac_address.h
  ac/shared_gobject.h
  ac/keep_alive.h
  ac/config.h

  ac/common/executable.h
  ac/common/executor.h
  ac/common/executorfactory.h

  ac/network/types.h

  ac/report/lttng/utils.h
  ac/report/lttng/encoderreport_tp.h
  ac/report/lttng/rendererreport_tp.h
  ac/report/lttng/packetizerreport_tp.h
  ac/report/lttng/senderreport_tp.h

  ac/video/encoderreport.h
  ac/video/rendererreport.h
  ac/video/packetizerreport.h
  ac/video/senderreport.h
  ac/video/bufferproducer.h

  ac/streaming/packetizer.h

  w11tng/config.h
)

set(SOURCES
  ac/types.cpp
  ac/utils.cpp
  ac/networkutils.cpp
  ac/mediamanagerfactory.cpp
  ac/basesourcemediamanager.cpp
  ac/logger.cpp
  ac/forwardingcontroller.cpp
  ac/forwardingnetworkdevice.cpp
  ac/controller.cpp
  ac/service.cpp
  ac/sourcemanager.cpp
  ac/sourceclient.cpp
  ac/networkmanager.cpp
  ac/networkmanagerfactory.cpp
  ac/networkdevice.cpp

  ac/dbus/helpers.cpp
  ac/dbus/errors.cpp
  ac/dbus/controllerskeleton.cpp
  ac/dbus/networkdeviceskeleton.cpp

  ac/common/executorpool.cpp
  ac/common/threadedexecutor.cpp
  ac/common/threadedexecutorfactory.cpp

  ac/network/stream.cpp
  ac/network/udpstream.cpp

  ac/report/reportfactory.cpp
  ac/report/reportfactory.h

  ac/report/null/nullreportfactory.cpp
  ac/report/null/encoderreport.cpp
  ac/report/null/rendererreport.cpp
  ac/report/null/packetizerreport.cpp
  ac/report/null/senderreport.cpp
  ac/report/logging/loggingreportfactory.cpp
  ac/report/logging/encoderreport.cpp
  ac/report/logging/rendererreport.cpp
  ac/report/logging/packetizerreport.cpp
  ac/report/logging/senderreport.cpp
  ac/report/lttng/lttngreportfactory.cpp
  ac/report/lttng/tracepointprovider.cpp
  ac/report/lttng/encoderreport.cpp
  ac/report/lttng/rendererreport.cpp
  ac/report/lttng/packetizerreport.cpp
  ac/report/lttng/senderreport.cpp

  ac/video/videoformat.cpp
  ac/video/buffer.cpp
  ac/video/bufferqueue.cpp
  ac/video/utils.cpp
  ac/video/utils_from_android.cpp
  ac/video/baseencoder.cpp
  ac/video/h264analyzer.cpp
  ac/video/displayoutput.cpp

  ac/streaming/transportsender.cpp
  ac/streaming/mpegtspacketizer.cpp
  ac/streaming/rtpsender.cpp
  ac/streaming/mediasender.cpp

  ac/mir/sourcemediamanager.cpp
  ac/mir/screencast.cpp
  ac/mir/streamrenderer.cpp

  ac/android/h264encoder.cpp

  ac/systemcontroller.cpp

  ac/ubuntu/unity.cpp
  ac/ubuntu/systemcontroller.cpp
  ac/ubuntu/unitydisplaylock.cpp

  w11tng/networkmanager.cpp
  w11tng/rfkillmanager.cpp
  w11tng/kernelrfkillmanager.cpp
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

add_library(aethercast-lttng SHARED ac/report/lttng/tracepoints.c)
target_link_libraries(aethercast-lttng
    ${LTTNG_UST_LIBRARIES}
)

install(TARGETS aethercast-lttng
  LIBRARY DESTINATION ${AETHERCAST_TRACEPOINT_LIB_INSTALL_DIR}
)

add_executable(aethercast ac/main.cpp)
target_link_libraries(aethercast aethercast-core)

install(
  TARGETS aethercast
  RUNTIME DESTINATION sbin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib/static
)

add_subdirectory(client)
