set(LIB_SOURCES
    mockmir.cpp
)

add_library(aethercast-test-mir ${LIB_SOURCES})

AETHERCAST_ADD_TEST(screencast_tests screencast_tests.cpp aethercast-test-mir)
AETHERCAST_ADD_TEST(streamrenderer_tests streamrenderer_tests.cpp)
