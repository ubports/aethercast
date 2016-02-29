link_directories(
  ${MIRCLIENT_LIBRARY_DIRS}
  ${HYBRIS_MEDIA_LIBRARY_DIRS}
  ${EGL_LIBRARY_DIRS}
  ${GLESV2_LIBRARY_DIRS}
  ${GLIB_LIBRARY_DIRS}
  ${GIO_LIBRARY_DIRS}
  ${GIO-UNIX_LIBRARY_DIRS}
)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
  ${MIRCLIENT_INCLUDE_DIRS}
  ${HYBRIS_MEDIA_INCLDUE_DIRS}
  ${ANDROID_HEADERS_INCLUDE_DIRS}
  ${EGL_INCLUDE_DIRS}
  ${GLESV2_INCLUDE_DIRS}
  ${GLIB_INCLUDE_DIRS}
  ${GIO_INCLUDE_DIRS}
  ${GIO-UNIX_INCLUDE_DIRS}
  ${GST_INCLUDE_DIRS}
)

add_executable(mirscreencast_to_android_hw_enc
    mirscreencast_to_android_hw_enc.cpp)
target_link_libraries(mirscreencast_to_android_hw_enc
  ${MIRCLIENT_LIBRARIES}
  ${HYBRIS_MEDIA_LIBRARIES}
  ${EGL_LIBRARIES}
  ${GLESV2_LIBRARIES}
  ${GLIB_LIBRARIES}
  ${GIO_LIBRARIES}
  ${GIO-UNIX_LIBRARIES}
  ${GST_LIBRARIES}
  aethercast-core
)

set(MIRSCREENCAST_TO_STREAM_SOURCES
    simplesource.cpp
    mirscreencast_to_stream.cpp)

add_executable(mirscreencast_to_stream
    ${MIRSCREENCAST_TO_STREAM_SOURCES})
target_link_libraries(mirscreencast_to_stream
  ${MIRCLIENT_LIBRARIES}
  ${HYBRIS_MEDIA_LIBRARIES}
  ${EGL_LIBRARIES}
  ${GLESV2_LIBRARIES}
  ${GLIB_LIBRARIES}
  ${GIO_LIBRARIES}
  ${GIO-UNIX_LIBRARIES}
  ${GST_LIBRARIES}
  aethercast-core
)

install(
  TARGETS mirscreencast_to_stream
  RUNTIME DESTINATION bin
)

add_executable(mpegts_muxer mpegts_muxer.cpp)
target_link_libraries(mpegts_muxer aethercast-core)

add_executable(raw_streamer raw_streamer.cpp)
target_link_libraries(raw_streamer aethercast-core)
