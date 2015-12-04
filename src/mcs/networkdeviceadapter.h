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
#include "miracastinterface.h"
#ifdef __cplusplus
}
#endif

#include "networkdevice.h"
#include "miracastservice.h"

namespace mcs {

class NetworkDeviceAdapter {
public:
    typedef std::shared_ptr<NetworkDeviceAdapter> Ptr;

    static NetworkDeviceAdapter::Ptr Create(GDBusConnection *connection, const NetworkDevice::Ptr &device, const MiracastService::Ptr &service);

    ~NetworkDeviceAdapter();

    GDBusObjectSkeleton* DBusObject() const;
    std::string Path() const;

private:
    static void OnHandleConnect(MiracastInterfaceDevice *skeleton, GDBusMethodInvocation *invocation,
                                const gchar *role, gpointer user_data);
    static void OnHandleDisconnect(MiracastInterfaceDevice *skeleton, GDBusMethodInvocation *invocation,
                                   gpointer user_data);

private:
    NetworkDeviceAdapter(GDBusConnection *connection, const NetworkDevice::Ptr &device, const MiracastService::Ptr &service);

    std::string GeneratePath() const;

private:
    GDBusConnection *connection_;
    MiracastInterfaceObjectSkeleton *object_;
    NetworkDevice::Ptr device_;
    MiracastService::Ptr service_;
    MiracastInterfaceDevice *device_iface_;
    std::string path_;
};

} // namespace mcs

#endif
