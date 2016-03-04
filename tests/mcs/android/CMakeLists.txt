set(LIB_SOURCES
    mockmedia.cpp
    mockencoderreport.cpp
)

add_library(aethercast-test-android ${LIB_SOURCES})

AETHERCAST_ADD_TEST(h264encoder_tests h264encoder_tests.cpp aethercast-test-android)
