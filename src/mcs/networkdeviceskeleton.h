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

#ifdef __cplusplus
extern "C" {
#endif
#include "aethercastinterface.h"
#ifdef __cplusplus
}
#endif

#include "forwardingnetworkdevice.h"
#include "miracastcontroller.h"
#include "scoped_gobject.h"
#include "shared_gobject.h"

namespace mcs {

class NetworkDeviceSkeleton : public std::enable_shared_from_this<NetworkDeviceSkeleton>,
                              public ForwardingNetworkDevice {
public:
    typedef std::shared_ptr<NetworkDeviceSkeleton> Ptr;

    static NetworkDeviceSkeleton::Ptr Create(const SharedGObject<GDBusConnection> &connection, const std::string &path, const NetworkDevice::Ptr &device, const MiracastController::Ptr &controller);

    GDBusObjectSkeleton* DBusObject() const;
    std::string Path() const;

    void SyncProperties();

private:
    static gboolean OnHandleConnect(AethercastInterfaceDevice *skeleton, GDBusMethodInvocation *invocation,
                                const gchar *role, gpointer user_data);
    static gboolean OnHandleDisconnect(AethercastInterfaceDevice *skeleton, GDBusMethodInvocation *invocation,
                                   gpointer user_data);

private:
    NetworkDeviceSkeleton(const SharedGObject<GDBusConnection> &connection, const std::string &path, const NetworkDevice::Ptr &device, const MiracastController::Ptr &service);

    std::shared_ptr<NetworkDeviceSkeleton> FinalizeConstruction();

private:
    SharedGObject<GDBusConnection> connection_;
    SharedGObject<AethercastInterfaceObjectSkeleton> object_;
    std::string path_;
    MiracastController::Ptr controller_;
    ScopedGObject<AethercastInterfaceDevice> device_iface_;
};

} // namespace mcs

#endif
