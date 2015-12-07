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
#include <boost/concept_check.hpp>

#include "miracastserviceadapter.h"
#include "networkdeviceadapter.h"
#include "utils.h"
#include "keep_alive.h"
#include "logging.h"
#include "dbushelpers.h"

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

    SyncProperties();

    object_ = miracast_interface_object_skeleton_new(path_.c_str());
    if (!object_) {
        mcs::Error("Failed to create object for device %s", device->Address().c_str());
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

void NetworkDeviceAdapter::SyncProperties() {
    miracast_interface_device_set_address(device_iface_, device_->Address().c_str());
    miracast_interface_device_set_name(device_iface_, device_->Name().c_str());
    miracast_interface_device_set_state(device_iface_, NetworkDevice::StateToStr(device_->State()).c_str());

    auto capabilities = DBusHelpers::GenerateCapabilities(device_->SupportedRoles());
    miracast_interface_device_set_capabilities(device_iface_, capabilities);
    g_strfreev(capabilities);
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
    boost::ignore_unused_variable_warning(skeleton);
    boost::ignore_unused_variable_warning(role);

    auto inst = static_cast<NetworkDeviceAdapter*>(user_data);

    if (not inst)
        return;

    g_object_ref(invocation);

    inst->service_->Connect(inst->device_, [invocation](mcs::Error error) {
        if (error != kErrorNone) {
            g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                                                  "%s", mcs::ErrorToString(error).c_str());
            g_object_unref(invocation);
            return;
        }

        g_dbus_method_invocation_return_value(invocation, nullptr);
        g_object_unref(invocation);
    });
}

void NetworkDeviceAdapter::OnHandleDisconnect(MiracastInterfaceDevice *skeleton, GDBusMethodInvocation *invocation,
                                              gpointer user_data)
{
    boost::ignore_unused_variable_warning(skeleton);

    auto inst = static_cast<NetworkDeviceAdapter*>(user_data);

    if (not inst)
        return;

    g_object_ref(invocation);

    inst->service_->Disconnect(inst->device_, [invocation](mcs::Error error) {
        if (error != kErrorNone) {
            g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED,
                                                  "%s", mcs::ErrorToString(error).c_str());
            g_object_unref(invocation);
            return;
        }

        g_dbus_method_invocation_return_value(invocation, nullptr);
        g_object_unref(invocation);
    });
}

} // namespace mcs
