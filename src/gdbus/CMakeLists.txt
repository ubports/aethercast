# We can't control what gdbus-codegen produces so we have to live with the
# fact that it produces code which doesn't pass -Werror
set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "-Wall")

execute_process(COMMAND ${GDBUS_CODEGEN_EXECUTABLE}
                        --c-namespace AethercastInterface
                        --c-generate-object-manager
                        --generate-c-code ${CMAKE_CURRENT_BINARY_DIR}/aethercastinterface
                        --interface-prefix org.aethercast.
                        ${CMAKE_CURRENT_SOURCE_DIR}/../../data/org.aethercast.xml)
# gdbus-codegen generates GDBusInterfaceVTables with const as specifier
# but there are methods from both the generated code and gdbus itself
# to retrieve the vtable and modify it. We're changing the set_property
# function of the vtable inside our implementation to apply additional
# policy decisions when certain properties are changed and we want to
# return an error to the user instead of not applying the property
# change silently.
execute_process(COMMAND sed
    -i s:const\ GDBusInterfaceVTable:GDBusInterfaceVTable:g
    ${CMAKE_CURRENT_BINARY_DIR}/aethercastinterface.c)

execute_process(COMMAND ${GDBUS_CODEGEN_EXECUTABLE}
                        --c-namespace WpaSupplicant
                        --c-generate-object-manager
                        --generate-c-code ${CMAKE_CURRENT_BINARY_DIR}/wpasupplicantinterface
                        --interface-prefix fi.w1.wpa_supplicant1.
                        ${CMAKE_CURRENT_SOURCE_DIR}/../../data/fi.w1.wpa_supplicant1.xml)

include_directories(
  ${GLIB_INCLUDE_DIRS}
  ${GIO_INCLUDE_DIRS}
  ${GIO-UNIX_INCLUDE_DIRS}
)

set(GDBUS_WRAPPER_SOURCE
  ${CMAKE_CURRENT_BINARY_DIR}/aethercastinterface.c
  ${CMAKE_CURRENT_BINARY_DIR}/wpasupplicantinterface.c
)

add_library(aethercast-gdbus-wrapper ${GDBUS_WRAPPER_SOURCE})
