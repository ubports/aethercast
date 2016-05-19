/*
 * Copyright (C) 2015 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef NETWORKDEVICEADAPTER_H_
#define NETWORKDEVICEADAPTER_H_

extern "C" {
// Ignore all warnings coming from the external headers as we don't
// control them and also don't want to get any warnings from them
// which will only pollute our build output.
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-w"
#include "aethercastinterface.h"
#pragma GCC diagnostic pop
}

#include "ac/forwardingnetworkdevice.h"
#include "ac/controller.h"
#include "ac/scoped_gobject.h"
#include "ac/shared_gobject.h"

namespace ac {
namespace dbus {
class NetworkDeviceSkeleton : public std::enable_shared_from_this<NetworkDeviceSkeleton>,
                              public ForwardingNetworkDevice {
public:
    typedef std::shared_ptr<NetworkDeviceSkeleton> Ptr;

    static NetworkDeviceSkeleton::Ptr Create(const SharedGObject<GDBusConnection> &connection, const std::string &path, const NetworkDevice::Ptr &device, const Controller::Ptr &controller);

    GDBusObjectSkeleton* DBusObject() const;
    std::string Path() const;

    void SyncProperties();

private:
    static gboolean OnHandleConnect(AethercastInterfaceDevice *skeleton, GDBusMethodInvocation *invocation,
                                const gchar *role, gpointer user_data);
    static gboolean OnHandleDisconnect(AethercastInterfaceDevice *skeleton, GDBusMethodInvocation *invocation,
                                   gpointer user_data);

private:
    NetworkDeviceSkeleton(const SharedGObject<GDBusConnection> &connection, const std::string &path, const NetworkDevice::Ptr &device, const Controller::Ptr &service);

    std::shared_ptr<NetworkDeviceSkeleton> FinalizeConstruction();

private:
    SharedGObject<GDBusConnection> connection_;
    SharedGObject<AethercastInterfaceObjectSkeleton> object_;
    std::string path_;
    Controller::Ptr controller_;
    ScopedGObject<AethercastInterfaceDevice> device_iface_;
};
} // namespace dbus
} // namespace ac

#endif
