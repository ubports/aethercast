add_library(aethercast-test-w11tng
    interfaceskeleton.cpp
    peerskeleton.cpp)

AETHERCAST_ADD_TEST(peerstub_tests peerstub_tests.cpp aethercast-test-w11tng)
AETHERCAST_ADD_TEST(interfacestub_tests interfacestub_tests.cpp aethercast-test-w11tng)
AETHERCAST_ADD_TEST(interfaceselector_tests interfaceselector_tests.cpp aethercast-test-w11tng)

AETHERCAST_ADD_TEST(informationelement_tests informationelement_tests.cpp)
