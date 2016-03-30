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

set(MIRSCREENCAST_TO_STREAM_SOURCES
    simplesource.cpp
    mirscreencast_to_stream.cpp)

add_executable(mirscreencast_to_stream
    ${MIRSCREENCAST_TO_STREAM_SOURCES})
target_link_libraries(mirscreencast_to_stream aethercast-core)

install(
  TARGETS mirscreencast_to_stream
  RUNTIME DESTINATION bin
)

add_executable(mpegts_muxer mpegts_muxer.cpp)
target_link_libraries(mpegts_muxer aethercast-core)
