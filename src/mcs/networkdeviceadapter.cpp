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

#include <algorithm>

#include "miracastserviceadapter.h"
#include "networkdeviceadapter.h"
#include "utils.h"

namespace mcs {

NetworkDeviceAdapter::Ptr NetworkDeviceAdapter::Create(GDBusConnection *connection, const std::string &path, const NetworkDevice::Ptr &device, const MiracastService::Ptr &service) {
    return std::shared_ptr<NetworkDeviceAdapter>(new NetworkDeviceAdapter(connection, path, device, service));
}

NetworkDeviceAdapter::NetworkDeviceAdapter(GDBusConnection *connection, const std::string &path, const NetworkDevice::Ptr &device, const MiracastService::Ptr &service) :
    connection_(connection),
    object_(nullptr),
    path_(path),
    device_(device),
    service_(service) {

    device_iface_ = miracast_interface_device_skeleton_new();

    g_signal_connect(device_iface_, "handle-connect",
                     G_CALLBACK(&NetworkDeviceAdapter::OnHandleConnect), this);
    g_signal_connect(device_iface_, "handle-disconnect",
                     G_CALLBACK(&NetworkDeviceAdapter::OnHandleDisconnect), this);

    object_ = miracast_interface_object_skeleton_new(path_.c_str());
    if (!object_) {
        g_warning("Failed to create object for device %s", device->Address().c_str());
        return;
    }

    miracast_interface_object_skeleton_set_device(object_, device_iface_);
}

NetworkDeviceAdapter::~NetworkDeviceAdapter() {
    if (device_iface_)
        g_object_unref(device_iface_);

    if (object_)
        g_object_unref(object_);
}

GDBusObjectSkeleton* NetworkDeviceAdapter::DBusObject() const {
    return G_DBUS_OBJECT_SKELETON(object_);
}

std::string NetworkDeviceAdapter::Path() const {
    return path_;
}

void NetworkDeviceAdapter::OnHandleConnect(MiracastInterfaceDevice *skeleton, GDBusMethodInvocation *invocation,
                                           const gchar *role, gpointer user_data)
{
    g_dbus_method_invocation_return_value(invocation, nullptr);
}

void NetworkDeviceAdapter::OnHandleDisconnect(MiracastInterfaceDevice *skeleton, GDBusMethodInvocation *invocation,
                                              gpointer user_data)
{
    g_dbus_method_invocation_return_value(invocation, nullptr);
}

} // namespace mcs
