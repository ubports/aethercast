configure_file(miracast-service.conf.in miracast-service.conf @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/miracast-service.conf
        DESTINATION /etc/init/)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/miracast-service-dbus.conf
        DESTINATION /etc/dbus-1/system.d/)
