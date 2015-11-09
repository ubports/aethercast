add_executable(dhclient-helper dhclient-helper.cpp)

qt5_use_modules(dhclient-helper Core Network)

target_link_libraries(dhclient-helper )

install(TARGETS dhclient-helper RUNTIME DESTINATION bin)
