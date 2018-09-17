configure_file(aethercast.conf.in aethercast.conf @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/aethercast.conf
        DESTINATION /etc/init/)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/aethercast-dbus.conf
        DESTINATION /etc/dbus-1/system.d/)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/dhcpd.conf
        DESTINATION /etc/aethercast/)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/dhclient-hook-p2p
        RENAME aethercast
        DESTINATION /etc/dhcp/dhclient-enter-hooks.d/)
