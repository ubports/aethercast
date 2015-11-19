miracast-service
================

This is a service using the wds project (https://github.com/01org/wds) to
implement a service management layer for Miracast / WiFi Display. It is
build agnostic to the actual network management layer so it can use
whatever the system it runs in provides. This could be wpa-supplicant,
NetworkManager or connman. Its purpose is to provide a common DBus API
the UI layer can use to provide screen sharing functionality based on
the Miracast streaming protocol.
