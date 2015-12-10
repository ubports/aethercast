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

#include "networkdevice.h"
#include "miracastservice.h"
#include "shared_gobject.h"

namespace mcs {

class NetworkDeviceAdapter : public std::enable_shared_from_this<NetworkDeviceAdapter> {
public:
    typedef std::shared_ptr<NetworkDeviceAdapter> Ptr;

    static NetworkDeviceAdapter::Ptr Create(const SharedGObject<GDBusConnection> &connection, const std::string &path, const NetworkDevice::Ptr &device, const MiracastService::Ptr &service);

    ~NetworkDeviceAdapter();

    GDBusObjectSkeleton* DBusObject() const;
    std::string Path() const;

    void SyncProperties();

private:
    static void OnHandleConnect(AethercastInterfaceDevice *skeleton, GDBusMethodInvocation *invocation,
                                const gchar *role, gpointer user_data);
    static void OnHandleDisconnect(AethercastInterfaceDevice *skeleton, GDBusMethodInvocation *invocation,
                                   gpointer user_data);

private:
    NetworkDeviceAdapter(const SharedGObject<GDBusConnection> &connection, const std::string &path, const NetworkDevice::Ptr &device, const MiracastService::Ptr &service);

    std::shared_ptr<NetworkDeviceAdapter> FinalizeConstruction();

private:
    SharedGObject<GDBusConnection> connection_;
    AethercastInterfaceObjectSkeleton *object_;
    std::string path_;
    NetworkDevice::Ptr device_;
    MiracastService::Ptr service_;
    AethercastInterfaceDevice *device_iface_;
};

} // namespace mcs

#endif
