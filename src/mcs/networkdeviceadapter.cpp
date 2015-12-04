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

#include "networkdeviceadapter.h"
#include "utils.h"

namespace mcs {

NetworkDeviceAdapter::Ptr NetworkDeviceAdapter::Create(GDBusConnection *connection, const NetworkDevice::Ptr &device, const MiracastService::Ptr &service) {
    return std::shared_ptr<NetworkDeviceAdapter>(new NetworkDeviceAdapter(connection, device, service));
}

NetworkDeviceAdapter::NetworkDeviceAdapter(GDBusConnection *connection, const NetworkDevice::Ptr &device, const MiracastService::Ptr &service) :
    connection_(connection),
    device_(device),
    service_(service) {

    device_obj_ = miracast_interface_device_skeleton_new();
    path_ = GeneratePath();

    g_signal_connect(device_obj_, "handle-connect",
                     G_CALLBACK(&NetworkDeviceAdapter::OnHandleConnect), this);
    g_signal_connect(device_obj_, "handle-disconnect",
                     G_CALLBACK(&NetworkDeviceAdapter::OnHandleDisconnect), this);

    GError *error = nullptr;
    if (!g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(device_obj_),
                                     connection_, path_.c_str(), &error)) {
        g_warning("Failed to export object for device: %s", error->message);
        g_error_free(error);
        return;
    }
}

NetworkDeviceAdapter::~NetworkDeviceAdapter() {
    if (device_obj_) {
        g_dbus_interface_skeleton_unexport(G_DBUS_INTERFACE_SKELETON(device_obj_));
        g_object_unref(device_obj_);
    }
}

std::string NetworkDeviceAdapter::GeneratePath() const {
    std::string address = device_->Address();
    std::replace(address.begin(), address.end(), ':', '_');
    return mcs::Utils::Sprintf("/dev_%s", address.c_str());
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
