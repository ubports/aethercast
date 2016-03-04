add_library(aethercast-test-android mockmedia.cpp)

AETHERCAST_ADD_TEST(h264encoder_tests h264encoder_tests.cpp aethercast-test-android)
