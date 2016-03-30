set(SOURCES
    nl.c
    network.c
)

# We copied these code parts from LXC and don't want to fix any compile
# warnings in it.
set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "-Wall")

add_library(lxc-network ${SOURCES})
